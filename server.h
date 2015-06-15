#ifndef SERVER_H_
#define SERVER_H_
#include "common.h"
#include "db.h"
class server
{
	db m_database;
	int m_port;
	int m_socket;
	void client_proc(int sock);
public:
	server(int port);
	virtual ~server();
	void listen();
	void load();
};

#endif /* SERVER_H_ */
