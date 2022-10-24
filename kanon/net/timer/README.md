# 定时器设施
目前的定时器是基于`timerfd`和`有序集合`（STL底层实现多为红黑树）实现的：
* 添加定时器：$O(lgn)$
* 取消定时器：$O(lgn)$
* 获取所有过期定时器：$O(lgn)$
* 遍历所有过期定时器：$O(nlgn)$

**TODO**: 采用`时间轮`(time wheel)实现

## Timer
```cpp
struct Timer {
  using TimerCallback = std::function<void()>;
  
  TimeStamp expiration;
  uint64_t sequence;
  double interval;
  TimerCallback cb;
};
```
准备一个结构体封装`过期时间`、`回调`、`时间间隔`以及`序号`（下一节解释）。<br>
为了支持`重复性定时器(repeated timer)`，需要知道时间间隔。

## TimerQueue
> 这个Queue不是指数据结构中队列，即按照压入的元素顺序取出消费，而是过期时间最近的先取出消费，算是一种自定义的FIFO结构

因此通过有序结合管理这些Timer是很有必要的。<br>
首先假设采用`std::set<Timer, TimerCompare>`，那么会导致相同过期时间的定时器添加不进来，那么采用`std::multiset<Timer>`呢？那么取消定时器会出问题：对于相同过期时间的定时器会一起取消。<br>
那么我们绑定一个唯一整型ID，可以避免该现象，即`std::set<std::pair<Timer, uint64_t>>`。<br>
但是我们还是先来考虑一下`AddTimer()`该怎么设计：
```cpp
? AddTimer(Timer timer)
{
  loop_->RunInLoop([]() {
    timers_.insert(...);
  });
  return ?
}
```
由于需要保证线程安全，又因为定时器事件也融入了事件循环，所以利用第三阶段避免使用锁保证线程安全，所以利用事件循环的`RunInLoop()`方法即可。<br>
> 如果用锁的话，会导致明明在事件循环的处理中，却因为锁而阻塞（锁争用），这是很不划算的。

那么`AddTimer()`返回什么句柄给用户以便它能够传递给`CancelTimer()`取消定时器？<br>
如果选ID作为句柄，那么我们还得维护`ID -> Timer*`的映射，不然凑不成key。<br>
那么返回的句柄应当包含Timer对象：
* 拷贝Timer对象：显然不合适，回调可能是move-only的
* 移动Timer对象：显然不合适，被移动对象已经是空的了（或者被格式化了，依赖于实现）

利用`引用语义`的特点，先new一个Timer对象，然后用`std::set<Timer*, uint64_t>`表示容器，这样都是一致的，这样`AddTimer()`：
```cpp
std::pair<Timer*, uint64_t> AddTimer(Timer timer)
{
  auto t = new Timer(std::move(timer));
  auto id = ...;
  loop_->RunInLoop([]() {
    timers_.insert(t, id);
  });

  return std::make_pair(t, id);
}
```

但是实际上new出来的Timer对象也是唯一的，也就是说与ID重了，我们可以考虑用`std::set<Timer*>`来保存定时器，从而避免维护原子计数器（ID源）。<br>
可实际上，由于`Timer*`会返回给用户，因此它可能立即过期，然后`TimerQueue`释放了它，此时用户添加新的定时器会获取到相同的`Timer*`，然后用*旧*定时器去取消*新*定时器。
```cpp
auto p = new int(); delete p;
auto p2 = new int(); delete p2;
assert(p == p2); // Don't abort!
```
因此对于这种 **将动态分配指针作为容器的键并且提供容器的添加、删除接口并返回指针给用户（暴露键）** 的场景，是绝对不安全或易被误用的。<br>

起初，我是考虑用`TimeStamp`与`Timer*`绑在一起（前者是后者的字段），虽然冗余了，但是在一起是唯一的。<br>
然而只是返回`Timer*`恰当吗？<br>
为了作为键删除定时器，必须考虑`Timer*`是不是已经过期了，可是如果真的过期了，`Timer*`是非法的，因此无法作为键确认，即死锁了。
如果返回`<TimeStamp, Timer*>`也存在问题，因为重复定时器的过期时间是会变化的，因此不能采纳这种做法。<br>
最终我还是考虑用原子计数器作为序号（sequence）源来保证数据的唯一性，并用一个新容器以`<Sequence, Timer*>`作为元素类型来检测定时器是否活跃（active）。<br>
可是从结果来看，终究要用到原子计数器，因此直接用一个以`<Timer*, Sequence>`的容器保存就OK了，也不会出现之前提到的问题。

```cpp
using TimerEntry = std::pair<Timer*, uint64_t>;
std::set<TimerEntry, TimerEntryCompare> timers_;
```

## 自取消（Self-cancel）
> 自取消：在定时器回调中取消定时器

从需求角度分析：
* 对于一次性定时器，是不大需要自取消的，因为回调结束后，定时器就会被释放（尽管它也可以自取消并得到正确处理）<br>
* 对于重复性定时器，可能会有在满足一定条件后取消定时器的需求

那么自取消不特殊处理，会造成什么后果？
* 一次性定时器: `CancelTimer()`并无作用，因为它已经从`timers_`移除，之后也不会重置它（即无需特殊处理）。
* 重复性定时器: `CancelTimer()`不起作用，然后 *重置依然起作用* ，因此要考虑如何处理自取消场景

由于定时器处理事件的阶段如下：
```
获取所有过期定时器 ==> 调用定时器回调 ==> 重新设置重复性定时器
```
如果在回调中调用了`CancelTimer()`，只要让该函数意识到它在第二个阶段，标记这些自取消的定时器，然后第三阶段不处理它们即可。<br>
用一个布尔值变量和一个容器保存自取消的定时器来识别它们。
```cpp
bool calling_timer_;
std::set<TimerEntry, TimerEntryHash> canceling_timers_;
```
**第三阶段发现它在canceling_timers中，拒绝重置，** 正确处理重复性定时器的自取消问题。
