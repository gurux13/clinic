#include "server.h"
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
server::server(int port) :
		m_database(0, FILE_SIZE, true), m_port(port), m_socket(-1)
{
	// TODO Auto-generated constructor stub

}

server::~server()
{
	// TODO Auto-generated destructor stub
}

void server::listen()
{
	cout << "Listening on port " << m_port << "\n";
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in binding;
	memset(&binding, 0, sizeof(binding));
	binding.sin_family = AF_INET;
	binding.sin_port = htons(m_port);
	binding.sin_addr.s_addr = INADDR_ANY;
	if (bind(m_socket, (sockaddr*) &binding, sizeof(binding)))
	{
		perror("bind");
		return;
	}
	if (::listen(m_socket, 5))
	{
		perror("listen");
		return;
	}
	int cli_sock;
	while ((cli_sock = accept(m_socket, NULL, NULL)) != -1)
	{
		thread t(&server::client_proc, this, cli_sock);
		t.detach();
	}
}
inline bool contains(const char * s, char c)
{
	while (*s && *s != c)
		++s;
	return *s == c;
}
void server::client_proc(int sock)
{
	cout << "New client connection\n";
	char term[MAX_TERM_LENGTH + 1];
	while (true)
	{
		int total_bytes = 0;
		memset(term, 0, sizeof(term));
		while (total_bytes < MAX_TERM_LENGTH && !contains(term, '\n'))
		{
			int bytes = recv(sock, term + total_bytes,
					MAX_TERM_LENGTH - total_bytes, 0);
			if (bytes < 0)
			{
				close(sock);
				return;
			}
			if (bytes == 0)
				break;
		}
		string term_s = term;
		cout << "Requested " << term_s << "\n";
		m_database.fancy_term(term_s);
		if (term_s.length() == 0)
		{
			cout << "Client left\n";
			close(sock);
			return;
		}
		auto docs = m_database.get(term_s);
		stringstream data;
		for (auto doc : docs)
		{
			data << doc.doc_id << " " << doc.freq << " ";
		}
		data << "\n";
		string data_s = data.str();
		send(sock, data_s.c_str(), data_s.length(), 0);
	}
}

void server::load()
{
	cout << "Loading database...\n";
	m_database.load();
	cout << "Load done.\n";
}
