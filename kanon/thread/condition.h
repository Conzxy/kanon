#ifndef KANON_CONDVARIABLE_H
#define KANON_CONDVARIABLE_H

#include <pthread.h>
#include "pthreadM.h"
#include "kanon/util/noncopyable.h"
#include "mutexlock.h"

namespace kanon{

class Condition : noncopyable 
{
public:
	explicit Condition(MutexLock& mutex)
		: mutex_{mutex}
	{
		TCHECK(pthread_cond_init(&cond_, NULL));
	}

	~Condition(){
		TCHECK(pthread_cond_destroy(&cond_));
	}

	void wait(){
		MutexLock::UnassignHolder holder(mutex_);
		TCHECK(pthread_cond_wait(&cond_, &mutex_.mutex()));
	}

	bool waitForSeconds(double seconds);

	void notify(){
		TCHECK(pthread_cond_signal(&cond_));
	}

	void notifyAll(){
		TCHECK(pthread_cond_broadcast(&cond_));
	}

private:
	MutexLock& mutex_;	
	pthread_cond_t cond_;
};

}//namespace kanon

#endif // KANON_CONDITION_H
