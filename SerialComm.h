#ifndef __ARVIND__SERIALCOMM__H__
#define  __ARVIND__SERIALCOMM__H__

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

// Serial Comm : A serial communications class
// Author: Arvind Pereira

#define MAX_SERIAL_SIZE	4096

class SerialComm
{
  protected:
	struct termios oldtio;
	struct termios newtio;
	fd_set	readfs;

	int fd;
	char *devStr;

	double readTime;
	struct timeval	Timeout;

	char	in[MAX_SERIAL_SIZE];
	char	out[MAX_SERIAL_SIZE];

  public:
	SerialComm(){}
	SerialComm(const char *ttyDevice, tcflag_t baudRate,bool);
	SerialComm(const char *ttyDevice, const char *baudRate,bool);
	SerialComm(SerialComm &);
	int Open(const char *ttyDevice, tcflag_t baudRate,bool);
	int Close();

	int GetFd();

	void SetTio(termios *tio);
	void GetTio(termios *tio);
	void StoreTio();
	void RestoreTio();

	void ToggleDTR(unsigned);

	void SetDTR();
	void ClearDTR();
	void SetRTS();
	int  GetDCD();

	void FlushWrite();
	void DrainWrite();
	void FlushRead();

	void Write(const char *, const int len);
	void Read(char *, int &);

	int  PeekAndRead(char *, int &);
	int  PeekAndRead(char *, int &, int);
	int  Peek();
	int  Peek(int);

	tcflag_t GetBaudFlag(const char *baud);
};

#endif
