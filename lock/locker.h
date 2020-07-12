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
	//��С��semָ�����ź�����ֵ.����ź�����ֵ��0��,��ô���м�һ�Ĳ���,������������.����
	//����ź�����ǰΪ0ֵ,��ô���þͻ�һֱ����ֱ���ź�����ÿ��Խ��м�һ�Ĳ���
	bool wait()
	{
		return sem_wait(&m_sem) == 0;
	}
	//���������ź�����ֵ+1�������߳�����������ź�����ʱ���������������ʹ���е�һ���̲߳�������
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
		if (pthread_mutex_init(&m_mutex, NULL) != 0) //attrΪ��(NULL)��Ĭ������Ϊ���ٻ����� 
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
		//��m_mutex������Ȼ����Լ���������ȵ��ź����ˣ�Ҫ�ȳ��Լ������ټ���ִ�С�
		ret = pthread_cond_wait(&m_cond, m_mutex); //�����������ⲿwhileѭ���е��ж�
		return ret == 0;
	}
	bool time_wait(pthread_mutex_t &m_mutex, struct timespec t)
	{
		int ret = 0;
		ret = pthread_cond_timedwait(&m_cond, &m_mutex, &t); //��������ʽ�ȴ� time��������û��������
		return ret == 0;
	}
	bool signal()
	{
		//��һ�����ߣ�������һ������һ����Ʒ����������һ��������
		return pthread_cond_signal(&m_cond) == 0;
	}
	bool broadcast()
	{
		//һ�������߶������ߣ���������һ�β��������Ʒ������� �������߶����������
		return pthread_cond_broadcast(&m_cond) == 0;
	}

private:
	pthread_cond_t m_cond;
};
