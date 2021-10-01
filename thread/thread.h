/*
 * @version: 0.1 2021-5-23  init
 *			 0.2 2021-5-28	add "atomic.h"
 *			 0.3 2021-6-3   add move constructor & assignment
 * @author: Conzxy
 * Thread: pthread wrapper
 */

#ifndef _THREAD_H
#define _THREAD_H

#include "util/noncopyable.h"
#include "current-thread.h"
#include <pthread.h>
#include <functional>
#include <utility>
#include "atomic.h"

namespace kanon{

class Thread : public noncopyable {
public:
	//callback register
	using Threadfunc = std::function<void()>;

public:
	explicit Thread(Threadfunc func, std::string const& name = {});
	~Thread();

	//move constructor
	//so that thread can be placed in container by move construct
	Thread(Thread&& rhs) noexcept
		: func_{std::move(rhs.func_)},
		  is_started_{rhs.is_started_},
		  is_joined_{rhs.is_joined_},
		  pthreadId_{rhs.pthreadId_},
		  name_{std::move(rhs.name_)}
	{
		rhs.pthreadId_ = 0;
	}
	
	Thread& operator=(Thread&& rhs) noexcept
	{
		func_ = std::move(rhs.func_);
		is_started_ = rhs.is_started_;
		is_joined_ = rhs.is_joined_;
		pthreadId_ = rhs.pthreadId_;
		name_ = std::move(rhs.name_);

		rhs.pthreadId_ = 0;
		rhs.is_started_ = false;
		
		return *this;
	}

	void start();
	void join();

	std::string name() const noexcept
	{
		return name_;
	}

	pthread_t pthreadId() const noexcept
	{
		return pthreadId_;
	}

private:
	void setDefaultname();

private:
	Threadfunc	func_;
	bool		is_started_;
	bool		is_joined_;
	pthread_t	pthreadId_;
	std::string name_;

	static AtomicInt32 numCreated_;
};

}//namespace kanon

#endif //_THREAD_H
