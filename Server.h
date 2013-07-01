/**
 * \brief Server.h
 * Created on: Jun 21, 2013
 * \author Arvind A. de Menezes Pereira
 */
 /*!
 *
 * \subsection intro_sec Introduction to TCP_Server (base class for TCP/IP simulations)
 *
 * A very simple TCP/IP server which can be extended to allow for
 * fairly nice ways of talking to multiple clients.
 * It is upto the derived class to implement error-checking
 * that should also be added to make this more robust.
 * I have also included example code for how callbacks via CommandCallbackHandler.h, with a
 * demo to show how this can be extended for callback registrations as functors.
 *
 * Also included here are a couple of nice tools SignalTools.h which provide a singleton instance to capture
 * a few signals for quitting apps. The other nice tool is TimeTools.h which provides a set of easy to use
 * timer routines.
 *
 * Finally I have also written a Demo client DemoClient.cpp which also uses threaded reads for asynchronous read/write
 * capabilities from Client.h. This can also be made very powerful through the use of functors and so on.
 */
#ifndef SERVER_H_
#define SERVER_H_

#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>


namespace ArvindsTools {

using std::vector;
using std::map;
using std::string;
using std::invalid_argument;

#define MAX_BUFSIZE 4096

typedef struct
{
	int client_fd;
	int client_id;
	long int read_subscription_flags;
	long int write_subscription_flags;
	char client_name[40];
}ClientInfo;


class TCP_Server {
protected:
	const int MAX_DATA_LEN;
	int numClients;
	int server_sockfd, client_sockfd;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	socklen_t server_len, client_len;
	fd_set readfds, testfds;
	unsigned char buf[MAX_BUFSIZE];
	int numClientConnected;
	map<int,int> client_fds;
	map<int,ClientInfo> ClientTable;
	bool terminateListener;
	pthread_t listener;
	int server_port;
	TCP_Server() : MAX_DATA_LEN(1025) { }
public:
	explicit TCP_Server( int );
	virtual ~TCP_Server() {}

	int send_frame(int fd, const char *data, int len );
	void CreateSocket();
	void CloseSocket();
	void StartServer();
	void StopServer();
	map<int, ClientInfo> getClientTable();
	// These are meant to be over-ridden
	virtual void read_client(int fd);
	virtual void check_command(int, int, unsigned char *);
protected:
	int add_client();
	void remove_client(int fd);
	static void *listen_thread(void *arg);
	unsigned int calc_crc(unsigned char *ptr, int count);
 };

class PacketSizeException : public invalid_argument {
public:
	explicit PacketSizeException( const string &s ) : invalid_argument( s ) {}
};

}


#endif /* SERVER_H_ */
