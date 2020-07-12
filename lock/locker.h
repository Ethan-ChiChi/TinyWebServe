#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
	sem() {
		if (sem_init(&m_sem, 0, 0) != 0)
		{
			throw std::exception();
		}
	}
	sem(int num)
	{
		if (sem_init(&m_sem, 0, num) != 0)
		{
			throw std::exception();
		}
	}
	~sem()
	{
		sem_destroy(&m_sem);
	}
	//减小由sem指定的信号量的值.如果信号量的值比0大,那么进行减一的操作,函数立即返回.　　
	//如果信号量当前为0值,那么调用就会一直阻塞直到信号量变得可以进行减一的操作
	bool wait()
	{
		return sem_wait(&m_sem) == 0;
	}
	//用来增加信号量的值+1。当有线程阻塞在这个信号量上时，调用这个函数会使其中的一个线程不在阻塞
	bool post()
	{
		return sem_post(&m_sem) == 0;
	}
private:
	sem_t m_sem;
};

class locker
{
public:
	locker()
	{
		if (pthread_mutex_init(&m_mutex, NULL) != 0) //attr为空(NULL)，默认属性为快速互斥锁 
		{
			throw std::exception();
		}
	}
	~locker()
	{
		pthread_mutex_destroy(&m_mutex);
	}
	bool lock()
	{
		return pthread_mutex_lock(&m_mutex);
	}
	bool unlock()
	{
		return pthread_mutex_unlock(&m_mutex);
	}
	pthread_mutex_t* get()
	{
		return &m_mutex;
	}
private:
	pthread_mutex_t m_mutex;
};

class cond
{
public:
	cond()
	{
		if (pthread_cond_init(&m_cond, NULL) != 0)
		{
			throw std::exception();
		}
	}
	~cond()
	{
		pthread_cond_destroy(&m_cond);
	}
	bool wait(pthread_mutex_t *m_mutex)
	{
		int ret = 0;
		//将m_mutex解锁，然后把自己挂起。如果等到信号来了，要先尝试加锁，再继续执行。
		ret = pthread_cond_wait(&m_cond, m_mutex); //锁用来保护外部while循环中的判断
		return ret == 0;
	}
	bool time_wait(pthread_mutex_t &m_mutex, struct timespec t)
	{
		int ret = 0;
		ret = pthread_cond_timedwait(&m_cond, &m_mutex, &t); //以阻塞方式等待 time到了条件没满足会结束
		return ret == 0;
	}
	bool signal()
	{
		//单一生产者，生产者一次生产一个产品的情况，最好一个消费者
		return pthread_cond_signal(&m_cond) == 0;
	}
	bool broadcast()
	{
		//一个生产者多消费者，生产者能一次产生多个产品的情况。 多生产者多消费者情况
		return pthread_cond_broadcast(&m_cond) == 0;
	}

private:
	pthread_cond_t m_cond;
};
