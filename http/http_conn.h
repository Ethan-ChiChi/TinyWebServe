#include <fcntl.h>
#include <sys/epoll.h>
#include "../lock/locker.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>

using namespace std;

class http_conn
{
public:
	static const int BUFF_READ_SIZE = 2048;
	static const int BUFF_WRITE_SIZE = 2048;
	enum HTTP_CODE
	{
		NO_REQUEST,
		GET_REQUEST,
		POST_REQUEST,
		NO_RESOURCE,
		CLOSED_CONNECTION
	};
public:
	http_conn() {};
	~http_conn() {};

public:
	void init(int socketfd, const sockaddr_in &addr);
	void init();
	void close_conn(string msg = "");
	void process();
	bool read();
	bool write();

public:
	static int m_epollfd;
	static int m_user_count;

private:
	HTTP_CODE process_read();
	bool process_write(HTTP_CODE ret);
	void parser_requestline(const string &text, map<std::string, string> &m_map);
	void parser_header(const string& text, map<string, string>& m_map);
	void parser_postinfo(const string& text, map<string, string>& m_map);
	bool do_request();
	void unmap();

private:
	sockaddr_in m_addr;
	int m_socket;
	string filename;
	string postmsg;

	char read_buff[BUFF_READ_SIZE];
	char write_buff[BUFF_WRITE_SIZE];

	int read_for_row = 0;
	int write_for_row = 0;

	char* file_addr;
	char post_temp[];
	map<string, string> m_map;

	struct stat m_file_stat;
	struct iovec m_iovec[2];
	int m_iovec_length;
	
};