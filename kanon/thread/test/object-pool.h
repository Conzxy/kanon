#include "thread/mutexlock.h"
#include "util/noncopyable.h"
#include <memory>
#include <unordered_map>
#include <utility>
#include "thread/weak-callback.h"

namespace zxy {

#define HashMap std::unordered_map

// @note ObjectPool and Object refer to each other
// use weak_ptr always better choice
// 1) weak_ptr unlikely shared_ptr, it not increment reference count
// 2) avoid circular reference
// a better handler @see object-pool2.h
template<typename K, typename T>
class ObjectPool 
	: public std::enable_shared_from_this<ObjectPool<K, T>>	
	, noncopyable
	
{
	using WeakPtr = std::weak_ptr<T>;
	using iterator = typename HashMap<K, WeakPtr>::iterator;
public:
	using ObjectPtr = std::shared_ptr<T>;

	ObjectPool()
	{
		// shared_from_this() is prohibited called in constructor
		//T::pool_ = this->shared_from_this();
	}
	
	template<typename K2, typename... Args>
	ObjectPtr get(K2&& key, Args&&... args);
	
	void erase(K const& key)
	{ 
		MutexGuard guard(mutex_);
		auto x = pool_.find(key);
		if(contains(x) && x->second.expired())
		{
			pool_.erase(key);
			printf("erase ok\n");
		}
	}

private:
	
	bool contains(iterator const& x)
	{ return x != pool_.end(); }

	
private:
	// ver1: shared_ptr<T>
	// at least ref-count = 1, wrong
	// ver2: weak_ptr<T>
	// lock() atomically promote
	//
	HashMap<K, WeakPtr> pool_;	
	MutexLock mutex_;
}; 

template<typename K, typename T>
template<typename K2, typename ...Args>
auto ObjectPool<K, T>::get(K2&& key, Args&&... args) -> ObjectPtr
{
	MutexGuard guard(mutex_);	

	static int cnt = 1;
	if(cnt == 1){
		T::pool_ = this->shared_from_this();
		--cnt;
	}

	auto& weakPtr = pool_[key];
	auto promoted = weakPtr.lock();
	if(!promoted) {
		promoted.reset(new T(std::forward<K2>(key), std::forward<Args>(args)...));
		weakPtr = promoted;
	}
	
	return promoted;
}

template<typename K, typename Derived>
class Object
{
	using ObjectPoolPtr = std::weak_ptr<ObjectPool<K, Derived>>;
public:
	Object() = default;
	Object(K const& key) : key_(key) 
	{ }

	void setKey(K const& key)
	{ key_ = key; }

	K const& getKey() const noexcept
	{ return key_; }

	~Object()
	{
		makeWeakCallback(pool_.lock(), 
				&ObjectPool<K, Derived>::erase)
		(key_);
		//auto spool = pool_.lock();

		//if(spool) {
			// race condition
			// ==========================================
			// thread1          |   thread2
			// a = get("A");	|   
			// a destroied      |    
			//					|    b = get("A");[new]
			// MutexGuard		|
			// erase			|    
			//					|    c = get("A"):[new]
			//===========================================
			// violate semantic: b == c
			//
			
			// race condition
			// assert(x != end())
			// ===========================================
			// thread1			|   thread2
			// a = get("A")		| 
			// a destroied		|
			// (not locked)		|
			//					|   b = get("A")
			//					|   b destroied
			//					|   (not locked)
			// erase			|
			//					|   erase
			//					|   (trigger assertion)
			//============================================
			//auto x = spool->find(key_);
	
			//if (spool->contains(x) && 
					//x->second.expired()) {
				//spool->erase(key_);
				//printf("erase ok\n");
			//}
		//}
	}

private:
	friend ObjectPool<K, Derived>;
	
	static ObjectPoolPtr pool_;
	K key_;
};

template<typename K, typename Derived>
typename Object<K, Derived>::ObjectPoolPtr Object<K, Derived>::pool_ {};

} // namespace zxy

