#include "object_pool.h"

#include <memory>
#include <string>
#include <unistd.h>

#include "kanon/string/string_view.h"
#include "kanon/thread/thread.h"

using namespace kanon;

class Stock : public Object<std::string, Stock>
{
public:
  using base = Object<std::string, Stock>;

  Stock() = default;
  Stock(StringArg const& arg)
   : base((char const*)arg)
  { }

};

using StockPool = ObjectPool<std::string, Stock>;

auto pool = std::make_shared<StockPool>();

int main()
{

  Thread thr([]{
      auto stocka2 = pool->get("A");
      stocka2.reset();
      });
  
  thr.StartRun();
  
  sleep(1);
  auto stocka = pool->get("A");  
  
  sleep(1);
  auto stocka3 = pool->get("A");
  
  printf("stocka: %p\n", stocka.get());
  printf("stocka3: %p\n", stocka3.get());
  ASSERT(stocka == stocka3);
  thr.Join();

}


