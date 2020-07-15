#include "http_conn.h"

void setnonblocking(int socketfd)
{
	int old_opt = fcntl(socketfd, F_GETFL);
	int new_opt = old_opt | O_NONBLOCK;
	fcntl(socketfd, F_SETFL, new_opt);
}

void addfd(int epollfd, int socketfd)
{
	epoll_event m_event;                                    // create a event
	m_event.data.fd = socketfd;                             //ʹ��������Ϊ��������
	m_event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;        //ET
	epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &m_event);  //�����¼����
	setnonblocking(socketfd);
}

void removefd(int epollfd, int socketfd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, socketfd, 0);
}

void modfd(int epollfd, int socketfd, int ev)
{
	epoll_event m_event;
	m_event.data.fd = socketfd;
	m_event.events = ev | EPOLLET | EPOLLIN | EPOLLRDHUP;
	epoll_ctl(epollfd, socketfd, EPOLL_CTL_MOD, &m_event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

//�׽��ֵĳ�ʼ�������epoll�¼���
void http_conn::init(int socketfd, const sockaddr_in &addr)
{
	m_socket = socketfd;
	m_addr = addr;
	addfd(m_epollfd, m_socket);
}

void http_conn::init()
{
	filename = "";
	memset(read_buff, '\0', BUFF_READ_SIZE);
	memset(write_buff, '\0', BUFF_WRITE_SIZE);
	read_for_row = 0;
	write_for_row = 0;
}

void http_conn::close_conn(string msg)
{
	//m_socket != - 1��ʾ���Ѿ����ӵ��׽���
	if (m_socket != -1)
	{
		removefd(m_epollfd, m_socket);
		m_user_count--;
		m_socket = -1;
	}
}

void http_conn::process()
{
	HTTP_CODE read_ret = process_read();
	//NO_REQUEST����ʾ������������Ҫ����������������
	if (read_ret == NO_REQUEST) 
	{
		//ע�Ტ�������¼�
		modfd(m_epollfd, m_socket, EPOLLIN);
		return;
	}
	bool write_ret = process_write(read_ret);
	if (!write_ret)

	{
		close_conn();
	}

	//����EPOLLOUT�¼�
	modfd(m_epollfd, m_socket, EPOLLOUT);
}

//���������е�����������
void http_conn::parser_requestline(const string &text, map<string, string> &m_map)
{
	string m_method = text.substr(0, text.find(" "));
	string m_url = text.substr(text.find_first_of(" ") + 1, text.find_last_of(" ") - text.find_first_of(" ") - 1);
	string m_protocol = text.substr(text.find_last_of(" ") + 1);
	m_map["method"] = m_method;
	m_map["url"] = m_url;
	m_map["m_protocol"] = m_protocol;
}

//���������е�����ͷ����
void http_conn::parser_header(const string &text, map<string, string>& map)
{
	if (text.size() > 0)
	{
		if (text.find(": ") <= text.size())
		{
			string m_type = text.substr(0, text.find(": "));
			string m_content = text.substr(text.find(": ") + 2);
			m_map[m_type] = m_content;
		}
		else if (text.find("=") <= text.size())
		{
			string m_type = text.substr(0, text.find("="));
			string m_content = text.substr(text.find("=") + 1);
			m_map[m_type] = m_content;
		}
	}
}

void http_conn::parser_postinfo(const string& text, map<string, string>& m_map)
{
	string processd = "";
	string strleft = text;
	while (true)
	{
		processd = strleft.substr(0, strleft.find("&"));
		m_map[processd.substr(0, processd.find("="))] = processd.substr(processd.find("=") + 1);
		strleft = strleft.substr(strleft.find("&") + 1);
		if (strleft == processd) break;
	}
}

//��m_read_buf��ȡ��������������
http_conn::HTTP_CODE http_conn::process_read()
{
	string m_head = "";
	string m_left = read_buff; //����ת�ַ���

	int flag = 0;
	int do_post_flag = 0;
	while (true)
	{
		m_head = m_left.substr(0, m_left.find("\r\n"));
		m_left = m_left.substr(m_left.find("\r\n") + 2);
		if (flag == 0)
		{
			flag = 1;
			parser_requestline(m_head, m_map);
		}

		else if (do_post_flag)
		{
			parser_postinfo(m_head, m_map);
			break;
		}
		else
		{
			parser_header(m_head, m_map);
		}

		if (m_head == "") do_post_flag = 1;
		if (m_left == "") break;
	}

	if (m_map["method"] == "POST") return POST_REQUEST;
	else if (m_map["method"] == "GET") return GET_REQUEST;
	else return NO_REQUEST;
}

bool http_conn::do_request()
{
	if (m_map["method"] == "POST")
	{
		redis_clt* m_redis = redis_clt::getinstance();
		if (m_map["url"] == "/base.html" || m_map["url"] == "/") //������Ե�¼����
		{
			//��¼��ӭ����
			if (m_redis->getUserpasswd(m_map["username"]) == m_map["passwd"])
			{
				if(m_redis->getUserpasswd(m_map["username"] == "root"))
					filename = "./root/welcomeroot.html";
				else
					filename = "./root/welcome.html";
			}
			else 
				filename = "./root/error.html"; //��¼ʧ��ҳ��
		}
		else if (m_map["url"] == "/register.html") //ע��ҳ��
		{
			m_redis->setUserpasswd(m_map["username"], m_map["passwd"]);
			filename = "./root/register.html";
		}
		else if (m_map["url"] == "/welcome.http") //��¼��ҳ��
		{
			m_redis->vote(m_map["votename"]);
			postmsg = "";
			return false;
		}
		else if (m_map["url"] == "/getvote")
		{
			postmsg = m_redis->getvoteboard();
			return false;
		}
		else
		{
			filename = "./root/base.html";
		}
	}
	else if(m_map["method"] == "GET")
	{
		if (m_map["url"] == "/")
		{
			m_map["url"] = "base.html";
		}
		filename = "./root" + m_map["url"];
	}
	else
	{
		filename = "./root/error.html";
	}
	return true;
}

//���ڽ�һ���ļ�����������ӳ�䵽�ڴ棬����ļ��ķ����ٶ�
void http_conn::unmap()
{
	if (file_addr)
	{
		munmap(file_addr, m_file_stat.st_size);
		file_addr = 0;
	}
}

bool http_conn::process_write(HTTP_CODE ret)
/*
start��ӳ�����Ŀ�ʼ��ַ������Ϊ0ʱ��ʾ��ϵͳ����ӳ��������ʼ��ַ
length��ӳ�����ĳ���
prot���������ڴ汣����־���������ļ��Ĵ�ģʽ��ͻ
PROT_READ ��ʾҳ���ݿ��Ա���ȡ
flags��ָ��ӳ���������ͣ�ӳ��ѡ���ӳ��ҳ�Ƿ���Թ���
MAP_PRIVATE ����һ��д��ʱ������˽��ӳ�䣬�ڴ������д�벻��Ӱ�쵽ԭ�ļ�
fd����Ч���ļ���������һ������open()��������
off_toffset����ӳ��������ݵ����
*/
{
	if (do_request())
	{
		int fd = open(filename.c_str(), O_RDONLY);
		
		//��ȡ�ļ����ԣ��洢��m_file_stat��
		stat(filename.c_str(), &m_file_stat);
		file_addr = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		//I/O vector����readv��wirtev������صĽṹ�塣
		m_iovec[1].iov_base = file_addr;
		m_iovec[1].iov_len = m_file_stat.st_size;
		m_iovec_length = 2;
		close(fd);
	}
	else
	{
		if (postmsg != "")
		{
			if (postmsg.length() < 20)
				cout << "wrong pstmsg: " << postmsg << endl;
			strcpy(post_temp, postmsg.c_str());

			m_iovec[1].iov_base = post_temp;
			m_iovec[1].iov_len = (postmsg.size()) * sizeof(char);
			m_iovec_length = 2;
		}
		else
		{
			m_iovec_length = 1;
		}
	}
	return true;
}

bool http_conn::read()
{
	if (read_for_row > BUFF_READ_SIZE) // over buff
	{
		cout << "read error at begin" << endl;
		return false;
	}
	int bytes_read = 0;
	while (true)
	{
		bytes_read = recv(m_socket, read_buff + read_for_row, BUFF_READ_SIZE - read_for_row, 0);
		if (bytes_read == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) //����������
			{
				break;
			}
			cout << "bytes_read == -1" << endl;
			return false;
		}
		//���ӹر�
		else if (bytes_read == 0)
		{
			cout << "bytes_read == 0" << endl;
			return false;
			continue;
		}
		read_for_row += bytes_read;
	}
	return true;
}

bool http_conn::write()
{
	int bytes_write = 0;
	string response_head = "HTTP/1.1 200 OK \r\n\r\n";
	char head_temp[response_head.size()];
	strcpy(head_temp, response_head.c_str());
	m_iovec[0].iov_base = head_temp;
	m_iovec[0].iov_len = response_head.size() * sizeof(char);

	bytes_write = writev(m_socket, m_iovec, m_iovec_length);

	if (bytes_write <= 0)
	{
		return false;
	}
	unmap();
	if (m_map["Connection"] == "keep-alive")//������
	{
		return false;
	}
	else
	{
		return false;
	}
}




