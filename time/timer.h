//#include<sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <iostream>

using namespace std;
class t_client;

//������Դ
struct client_data
{
	sockaddr_in address; //�ͻ���socket��ַ
	int sockfd;
	t_client *timer; //��ʱ��
};

//��ʱ����
class t_client
{
public:
	t_client() : pre(nullptr), next(nullptr) {}

public:
	time_t livetime;

	void(*cb_func)(client_data*);
	client_data* user_data;
	t_client* pre;
	t_client* next;
};

class t_client_list
{
private:
	void add_timer(t_client* timer, t_client* lst_head);

public:
	t_client_list() : head(nullptr), tail(nullptr) {};
	~t_client_list();
	void add_timer(t_client* timer);
	void adjust_timer(t_client* timer);
	void del_timer(t_client* timer);
	void tick();
	t_client* remove_from_list(t_client* timer);

public:
	t_client* head;
	t_client* tail;
};

t_client_list::~t_client_list()
{
	t_client* temp = head;
	while (temp)
	{
		head = temp->next;
		delete temp;
		temp = head;
	}
}

void t_client_list::tick()
{
	if (!head)
		return;
	time_t cur = time(NULL);
	t_client* temp = head;
	while (temp)
	{
		if (cur < temp->livetime)
		{
			break;
		}
		else
		{
			temp->cb_func(temp->user_data);
		}
		head = temp->next;
		if (head)
			head->pre = NULL;
		delete temp;
		temp = head;
	}
}

void t_client_list::add_timer(t_client* timer)
{
	if (!timer)
	{
		cout << "not a timer" << endl;
		return;
	}
	if (!head)
	{
		head = timer;
		tail = timer;
	}

	//����µĶ�ʱ����ʱʱ��С�ڵ�ǰͷ�����
	//ֱ�ӽ���ǰ��ʱ�������Ϊͷ�����
	if (timer->livetime < head->livetime)
	{
		timer->next = head;
		head->pre = timer;
		head = timer;
	}
	// ������ǰ���֮����������ճ�ʱʱ���ҵ�Ŀ�궨ʱ����Ӧ��λ�ã�����˫������������
	else
	{
		t_client* temp = head;
		while (temp)
		{
			
			if (temp->livetime > timer->livetime)
			{
				t_client* temppre = timer->pre;
				timer->pre = temp->pre;
				timer->next = temp;
				temppre->next = timer;
				timer->pre = timer;
				return;
			}
			temp = temp->next;
		}
		tail->next = timer;
		timer->pre = tail;
		tail = timer;
	}
}

t_client* t_client_list::remove_from_list(t_client* timer)
{
	if (!timer)
		return timer;
	else if (timer == head && timer == tail)
		head = tail = nullptr;
	else if (timer == head)
	{
		head = head->next;
		head->pre = nullptr;
		timer->next = nullptr;
	}
	else if (timer == tail)
	{
		tail = tail->pre;
		tail->next = nullptr;
		timer->pre = nullptr;
	}
	else
	{
		timer->pre->next = timer->next;
		timer->next->pre = timer->pre;
		timer->next->pre = nullptr;
		timer->next = nullptr;
	}
	return timer;
}

void t_client_list::del_timer(t_client* timer)
{
	if (!timer)
		return;
	remove_from_list(timer);
	delete timer;
}

void t_client_list::adjust_timer(t_client* timer)
{
	if (timer)
	{
		t_client* temp = remove_from_list(timer);
		add_timer(temp);
	}
}

