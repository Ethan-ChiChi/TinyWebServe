#include "../lock/locker.h"
#include <exception>
#include <pthread.h>
#include <list>
#include <vector>
#include <iostream>

using namespace std;

/*
һ���̳߳���Ӧ��ӵ�еģ�
�̳߳����飬���ڳ�ʼ������߳�
������У����������
��������ӵ�append����
�߳�ִ�еĺ���worker��run���������߳���run���������о���������
�����̺߳͹����̶߳�������н��в���ʱ����Ҫ����
�ź�������¼�Ƿ�������Ҫ����û����ʱ����wait������
*/

template <typename T>
class threadpool
{
private:
	list<T*> request_list;
	vector<pthread_t> thread_list;
	locker m_lock;
	sem m_sem;
	int m_thread_num;
	int m_max_request;

private:
	static void* worker(void* arg);
	void run();

public:
	/*thread_number���̳߳����̵߳�������max_requests������������������ġ��ȴ���������������*/
	threadpool(int thread_num = 10, int max_request = 100);
	~threadpool();
	bool append(T* request);

private:
	/*�����߳����еĺ����������ϴӹ���������ȡ������ִ��֮*/
	static void* worker(void* arg);
	void run();
};

template<typename T>
threadpool<T>::threadpool(int thread_num, int max_request) : m_thread_num(thread_num), m_max_requests(max_request)
{
	thread_list = vector<pthread_t>(thread_num, 0);

	for (int i = 0; i < m_thread_num; i++)
	{
		if (pthread_create(&thread_list[i], NULL, worker, this) != 0)
		{
			cout << "some thing wrong!" << endl;
		}

		if (pthread_detach(thread_list[i]) != 0)
		{
			cout << "detach failed" << endl;
		}
	}
}

template <typename T>
threadpool<T>::~threadpool()
{
	;
}

template<typename T>
bool threadpool<T>::append(T* request)
{
	m_lock.lock();
	if (request_list.size() > m_max_request)
	{
		m_lock.unlock();
		return false;
	}
	request_list.push_back(request);
	m_lock.unlock();
	m_sem.post();
	return true;
}

template<typename T>
void* threadpool<T>::worker(void *arg)
{
	threadpool* pool = (threadpool*)arg;
	pool->run;
	return pool;

}

template <typename T>
void threadpool<T>::run()
{
	while (true)
	{
		m_sem.wait();
		m_lock.lock();
		if (request_list.size() <= 0)
		{
			m_lock.unlock();
		}
		else
		{
			T* request = request_list.front();
			request_list.pop_front();
			m_lock.unlock();

			if (!request)
			{
				continue;
			}
			request->process();
		}
	}
}


