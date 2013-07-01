/*
 * Server.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: arvind
 */
#include <iostream>
#include <map>
#include <cstring>
#include "string.h"
#include "Server.h"

namespace ArvindsTools {
using namespace std;

/** Create an instance of the basic TCP_Server
 * Default constructor.
 * */
TCP_Server::TCP_Server( int portNum ) :  MAX_DATA_LEN(1025) {
	server_port = portNum;
	StartServer();
	sleep(1);
}

/** Start the server. Creates sockets,
 * initializes number of connected clients,
 * and creates a listen thread to check for
 * connecting clients.
 * */
void TCP_Server::StartServer( ) {
	CreateSocket( );
	numClientConnected = 0;
	terminateListener = false;
	cerr<<"Server Started"<<endl;
	int res=pthread_create(&listener,NULL,listen_thread, this );
	if(res!=0)
	{
		perror("Listen thread creation failed.");
		exit(EXIT_FAILURE);
	}
}

/** Stop the Server. Stops the server by terminating the
 * listen thread.
 */
void TCP_Server::StopServer() {
	terminateListener = true;
	cerr<<"Stopping Server"<<endl;
	void *thread_result;
	pthread_join(listener, &thread_result);
	cerr<<"Server Stopped";
}

/** Create a TCP/IP server socket, bind to server_port and
 * listen for connections.
 */
void TCP_Server::CreateSocket(  )
{
	unlink("server_socket");
	server_sockfd=socket(AF_INET, SOCK_STREAM,0);

	server_address.sin_family=AF_INET;
	server_address.sin_addr.s_addr=htonl(INADDR_ANY);
	server_address.sin_port=htons( server_port );
	server_len=sizeof(server_address);

	bind(server_sockfd, (struct sockaddr*)&server_address, server_len);

	// Create a connection queue and initialize readfds to handle input
	// from server_sockfd
	listen(server_sockfd, 10 );

	printf("Listen queue created... Awaiting clients...\n");
	FD_ZERO(&readfds);	// Initialize readfds with no fds...
	FD_SET(server_sockfd,&readfds);	// readfds has server_sockfd...
}

/** An implementation to do CRC-CCITT based Cyclic Redundancy Checking
 *
 * @param ptr
 * @param count
 * @return CRC-16
 */
unsigned int TCP_Server::calc_crc(unsigned char *ptr, int count) {
  unsigned int crc,i;

  crc=0;
  while(--count>=0) {
    crc^=((int)*ptr)<<8; ptr++;
    for(i=0;i<8;i++)
      {
	if(crc&0x8000)
	  crc^=0x1021;
	else crc<<=1;
      }
  }
  return crc;
}

/** Send a server frame to client connected at a particular file-descriptor
 *
 * @param fd
 * @param data
 * @param len
 * @return number of bytes written. 0 otherwise.
 */
int TCP_Server::send_frame(int fd, const char *data, int len )
{
	if( ClientTable.find(fd)!=ClientTable.end()) {
		char frame_2_send[ MAX_DATA_LEN ];
		memset(frame_2_send, 0, MAX_DATA_LEN );
		if( len>=MAX_DATA_LEN ) throw PacketSizeException("Oversized Packet.");
		else {
			strncpy( frame_2_send, data, len );
			len = write(fd,frame_2_send,len);
			return len;
		}
	}
	return 0;
}

/** Adds a client to the ClientTable. Once added, this client
 * will be monitored for incoming data.
 * @return client_sockfd the file-descriptor associated with the client that just connected.
 */
int TCP_Server::add_client()
{
    client_len=sizeof(client_address);
    client_sockfd=accept(server_sockfd,(struct sockaddr*)&client_address, &client_len);
    FD_SET(client_sockfd, &readfds);
	++numClientConnected;
    if( ClientTable.find(client_sockfd)!=ClientTable.end() ) {
    	cerr<<"This client already exists."<<client_sockfd;
    }
    else {
    	ClientInfo client; client.client_fd = client_sockfd;
    	ClientTable[client_sockfd] = client;
    }
	for(map<int,ClientInfo>::iterator it=ClientTable.begin();it!=ClientTable.end();++it)
		cout<<"Client["<<it->first<<"] is at fd="<<it->second.client_fd<<endl;;

    return client_sockfd;
}

/** Get the client table connection information. Returns a
 * map containing an integer corresponding to the file-descriptor associated
 * with a client and its corresponding ClientInfo information.
 * @return map<int,ClientInfo> a map with the client info associated with each file-descriptor.
 */
map<int, ClientInfo> TCP_Server::getClientTable() {
	return ClientTable;
}

/** Remove client associated with the file descriptor provided
 *
 * @param fd file descriptor to be removed
 */
void TCP_Server::remove_client(int fd)
{
	close(fd);
	FD_CLR(fd,&readfds);
	--numClientConnected;
	map<int,ClientInfo>::iterator it = ClientTable.find(fd);
	if(it!=ClientTable.end())
		ClientTable.erase(it);
}

/** Listen thread - does most of the heavy lifting */
void *TCP_Server::listen_thread(void *arg)
{
	TCP_Server *servInstance=reinterpret_cast<TCP_Server*>(arg);

	while( !(servInstance->terminateListener) )
	{
		usleep(10);
		servInstance->testfds=servInstance->readfds;
		struct timeval timeout; timeout.tv_sec=1; timeout.tv_usec=500000;
		int result=select(FD_SETSIZE,&(servInstance->testfds),(fd_set*)0,(fd_set*)0,&timeout);
		if ( result<0 ) break;
		for(int fd=0; fd<FD_SETSIZE; fd++)
		{
		  if( FD_ISSET(fd,&(servInstance->testfds)) )
		  {
			if( fd== servInstance->server_sockfd )
			{
			   servInstance->add_client();
			   for(map<int,ClientInfo>::iterator it=servInstance->ClientTable.begin();
					   it!=servInstance->ClientTable.end();++it) {
			   		printf("\nClient[%d] is at fd=%d ",it->first,it->second.client_fd);
			   }
			}
		  	else
			{
		  	  int nread = 0;
			  ioctl(fd,FIONREAD,&nread);
			  if(nread==0)
			  {
				servInstance->remove_client(fd);
			  }
			  else {
				  servInstance->read_client(fd);
			  }
			}
		   }
		}
	}
	if(servInstance->terminateListener)
	  cerr<<"Server listen thread is exiting..."<<endl;
	pthread_exit(NULL);
}

/** Close the server socket. Will close all the client connections
 * first and then close the server socket for a graceful shutdown.
 *
 */
void TCP_Server::CloseSocket() {
	for(map<int,ClientInfo>::iterator it=ClientTable.begin();
						   it!=ClientTable.end();++it)
		remove_client(it->first);
	close(server_sockfd);
}

/** Could be over-ridden - reads data from a particular file-descriptor */
void TCP_Server::read_client(int fd) {
	int len;
	memset(buf,0,MAX_BUFSIZE);
	len=read(fd,&buf,MAX_BUFSIZE);
	check_command(fd,len,(unsigned char*)buf);
}

/** Should be over-ridden to get domain specific things to happen
 *  @param fd (int) file descriptor from which we've received this data
 *  @param len (int) length of data in bytes
 *  @param buf (UCHAR*) the data received as bytes
 * */
void TCP_Server::check_command(int fd, int len, unsigned char *buf ) {
	for(int i=0;i<len;i++)
		cout<<buf[i];
	cout<<endl;
}
}

