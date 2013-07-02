/**
 * \brief Client.h
 * \author Arvind A. de Menezes Pereira
 *
 * \summary This is a multi-platform client which is designed to work across multiple operating systems.
 * Currently it relies only upon the standard library (I like Boost, but don't want my code to rely heavily on
 * such a large library to compile in a new place). The Windows socket API client has not been tested so far
 * but I should get around to that soon (hopefully). The client works on both OS-X and Linux.
 */
#ifndef __SIMPLE_CLIENT_H__
#define __SIMPLE_CLIENT_H__

#ifdef _WINDOWS_
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
	// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")
#else  // This is probably being compiled for Linux or OS-X
	#include <unistd.h>
	#include <cerrno>
	#include <string.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <cstdio>
	#include <fcntl.h> // non blocking socket
#endif
// Cross platform headers...
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "TimeTools.h"
#include "ScopedLock.h"

namespace ArvindsTools {
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// size of our buffer
#define MAX_CLIENT_BUFSIZE 4096
// port to connect sockets through
#define DEFAULT_PORT "10000"


class TCP_Client {
#ifdef _WINDOWS_
	WSADATA wsaData; // Windows Socket data object
	struct addrinfo *result, *ptr, hints;
	int iResult;
	SOCKET ClientSocket;

public:
	TCP_Client() {
		// Initialize Winsock
		iResult = WSAStartup( MAKEWORD(2,2), &wsaData ); // initiate use of WS2_32.dll
		if( iResult != 0 ) {
			std::cerr<<"WSA Startup failed "<<iResult<<std::endl;
			exit(EXIT_FAILURE);
		}
	}

	bool CreateSocket( string ip_addr, string port  ) {
		ZeroMemory( &hints, sizeof(hints) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		iResult = getaddrinfo(ip_addr.c_str(), DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
		    printf("getaddrinfo failed: %d\n", iResult);
		    WSACleanup();
		    return false;
		}
		ClientSocket = INVALID_SOCKET;
		// Attempt to connect to the first address returned by
		// the call to getaddrinfo
		ptr=result;

		// Create a SOCKET for connecting to server
		ClientSocket = socket(ptr->ai_family, ptr->ai_socktype,
		    ptr->ai_protocol);
		if (ClientSocket == INVALID_SOCKET) {
		    printf("Error at socket(): %ld\n", WSAGetLastError());
		    freeaddrinfo(result);
		    WSACleanup();
		    return false;
		}

		// Connect to server.
		iResult = connect( ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
		    closesocket(ClientSocket);
		    ClientSocket = INVALID_SOCKET;
		}

		// Should really try the next address returned by getaddrinfo
		// if the connect call failed
		freeaddrinfo(result);

		if (ClientSocket == INVALID_SOCKET) {
		    printf("Unable to connect to server!\n");
		    WSACleanup();
		    return false;
		}

		// Great, we're done opening a socket to the server!
		return true;
	}

	int send_frame( string data ){
		// Send an initial buffer
		iResult = send(ClientSocket, data.c_str(), data.length(), 0);
		if (iResult == SOCKET_ERROR) {
		    printf("send failed: %d\n", WSAGetLastError());
		    closesocket(ClientSocket);
		    WSACleanup();
		    return -1;
		}

		printf("Bytes Sent: %ld\n", iResult);
		return iResult;
	}

	int receive( string &receivedData ) {
		int recvbuflen = MAX_CLIENT_BUF_SIZE-1;
		char recvbuf[MAX_CLIENT_BUFSIZE];


		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		        cout<<"Bytes received: "<<iResult<<'\n';
		else if (iResult == 0)
		        cout<<"Connection Closed\n";
		else
		        cout<<"recv failed: "<<WSAGetLastError()<<'\n';
	}

	int CloseSocket() {
		// shutdown the send half of the connection since no more data will be sent
		iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
		    printf("shutdown failed: %d\n", WSAGetLastError());
		    closesocket(ConnectSocket);
		    WSACleanup();
		    return 1;
		}
		// cleanup
		closesocket(ConnectSocket);
		WSACleanup();

		return 0;
	}


#else // Linux and OS-X (taken straight out of beej.us)
	int sockfd, numbytes;
	char buf[MAX_CLIENT_BUFSIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	fd_set readfds, testfds;
	char s[INET6_ADDRSTRLEN];
	bool terminateReader;
	pthread_t reader;
	pthread_mutex_t readMutex;
	TimeTools readTimer;
private:
	// get sockaddr, IPv4 or IPv6:
	void *get_in_addr(struct sockaddr *sa)
	{
	    if (sa->sa_family == AF_INET) {
	        return &(((struct sockaddr_in*)sa)->sin_addr);
	    }

	    return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}

public:
	TCP_Client() {
	    memset(&hints, 0, sizeof hints);
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    terminateReader = false;
	    sockfd = -1;
	}

	bool CreateSocket( string ip_addr, string port ) {
		int res=pthread_create(&reader,NULL,read_thread, this );
		if(res!=0)
		{ perror("Read thread creation failed."); exit(EXIT_FAILURE); }
		res = pthread_mutex_init( &readMutex, NULL );
		if (res!=0) { perror("Mutex init failed."); exit(EXIT_FAILURE); }

		if ((rv = getaddrinfo((char*)ip_addr.c_str(), (char*)port.c_str(), &hints, &servinfo)) != 0) {
	        cerr<<"getaddrinfo: "<<gai_strerror(rv)<<endl;
	        return false;
	    }
	    // loop through all the results and connect to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
					p->ai_protocol)) == -1) {
				perror("client: socket");
				continue;
			}

			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("client: connect");
				continue;
			}

			break;
		}

		if ( p==NULL ) {
			cerr<<"client: failed to connect\n";
			return false;
		}
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		            s, sizeof s);
		cout<<"client: connecting to "<< s<<endl;

	    FD_ZERO(&readfds);	// Initialize readfds with no fds...
	    FD_SET( sockfd,&readfds );	// readfds has sockfd...

		freeaddrinfo(servinfo); // all done with this structure
		return true;
	}

	/** Listen thread - does most of the heavy lifting */
	static void *read_thread(void *arg)
	{
		TCP_Client *clientInstance=reinterpret_cast<TCP_Client*>(arg);

		clientInstance->terminateReader = false;
		while( !(clientInstance->terminateReader) )
		{
			usleep(10);
			clientInstance->testfds=clientInstance->readfds;
			struct timeval timeout; timeout.tv_sec=1; timeout.tv_usec=500000;
			int result=select(FD_SETSIZE,&(clientInstance->testfds),(fd_set*)0,(fd_set*)0,&timeout);
			if ( result<0 ) break;
			string data;
			clientInstance->receive(data);
			clientInstance->check_frame(data);
		}
		if(clientInstance->terminateReader)
		  cerr<<"Client read thread is exiting..."<<endl;
		pthread_exit(NULL);
	}


	int receive( string &rx_data ) {
		if ((numbytes = recv(sockfd, buf, MAX_CLIENT_BUFSIZE-1, 0)) == -1) {
		        perror("recv");
		        exit(1);
		}
		rx_data=string(buf);
		return rx_data.length();
	}

	virtual int send_frame( string tx_data ) {
		if ((numbytes = send(sockfd, (char*)tx_data.c_str(), tx_data.length(), 0)) == -1) {
			perror("send");
			exit(1);
		}
		return numbytes;
	}

	virtual int check_frame( string data ) {
		cout<<readTimer.timeSinceStart()<<" Received:"<<data<<'\n'; cout.flush();
		return data.length();
	}

	int CloseSocket() {
		if ( sockfd > 0 ) {
			cout<<"Closing socket...";
			// shutdown the send half of the connection since no more data will be sent
			terminateReader = true;
			void *thread_result;
			pthread_join( reader, &thread_result );
			return close(sockfd);
		}
		else
			return 0;
	}



#endif


};

}




#endif /* __SIMPLE_CLIENT_H__ */
