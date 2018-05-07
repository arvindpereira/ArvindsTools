#include <sys/ioctl.h>
#include <iostream>
#include "SerialComm.h"

int SerialComm::Open(const char *ttyDevice, tcflag_t baudRate, bool isCanon)
{
	std::cout << "TTY device: " << ttyDevice << std::endl;
	// Default Open - read/write, canonical and non-blocking
	fd = open( ttyDevice, O_RDWR | O_NOCTTY ); // | O_NONBLOCK );

	if ( fd == -1 ) {
		fprintf(stderr,"\nERROR OPENING PORT %s. Error %d. ",ttyDevice,errno);
		perror(ttyDevice);
		exit(EXIT_FAILURE);
	}
	else {
		// By default, we want to hold the previous settings so that
		// we can restore them back before closing the port.
		StoreTio();

		newtio = oldtio;
		// Initialize the new TIO settings
		bzero(&newtio, sizeof( newtio ));

		newtio.c_cflag = baudRate ;
		newtio.c_cflag |= CS8 | CLOCAL | CREAD ;	// 8N1
		newtio.c_cflag &= ~CRTSCTS;			// No H/W flow control
		newtio.c_cflag &= ~(PARENB | PARODD);
		cfsetospeed( &newtio, (speed_t) baudRate);
		cfsetispeed( &newtio, (speed_t) baudRate);

		fprintf(stderr,"\nnewtio.c_cflag=%d\n",newtio.c_cflag);

		newtio.c_iflag = IGNBRK ;
		newtio.c_oflag = 0;
		newtio.c_lflag = 0; // &=  ~(ICANON | ECHO | ECHOE | ISIG);  /*ICANON*/
		if(isCanon) {
			newtio.c_lflag = ICANON;
			newtio.c_cc[VEOF]      =4;
		}
		newtio.c_cc[VINTR] = 0;
		newtio.c_cc[VQUIT] = 0;
		newtio.c_cc[VERASE] = 0;
		newtio.c_cc[VKILL]     =0;

        	newtio.c_cc[VTIME]     =0;
        	newtio.c_cc[VMIN]      =1;
        	newtio.c_cc[VSWTC]     =0;
        	newtio.c_cc[VSTART]    =0;
        	newtio.c_cc[VSTOP]     =0;
        	newtio.c_cc[VSUSP]     =0;
        	newtio.c_cc[VEOL]      =0;
        	newtio.c_cc[VREPRINT]  =0;
        	newtio.c_cc[VDISCARD]  =0;
        	newtio.c_cc[VWERASE]   =0;
        	newtio.c_cc[VLNEXT]    =0;
        	newtio.c_cc[VEOL2]     =0;

		SetTio(&newtio);
		FlushWrite();
		FlushRead();

		SetDTR();
		SetRTS();
		fprintf(stderr, "Done Configuring the port %s",ttyDevice);
	}

	return fd;
}

int SerialComm::GetFd() {
	return fd;
}


void SerialComm::ToggleDTR(unsigned usec)
{
	ClearDTR();
	usleep (usec);
	SetDTR();
}

void SerialComm::SetDTR()
{
	#ifdef TIOCSDTR
	  ioctl(fd,TIOCSDTR, 0);
	#endif
}

void SerialComm::ClearDTR()
{
	#ifdef TIOCCDTR
	  ioctl(fd, TIOCCDTR, 0);
	#endif
}



void SerialComm::SetRTS()
{
	#if defined(TIOCM_RTS) && defined(TIOCMODG)
	{
		int mcs = 0;

		ioctl(fd, TIOCMODG, &mcs);
		mcs |= TIOCM_RTS;
		ioctl(fd, TIOCMODS, &mcs);
	}
	#endif
	#ifdef _COHERENT
		ioctl(fd, TIOCSRTS, 0);
	#endif
}


int SerialComm::GetDCD()
{
#ifdef TIOCMGET
  {
	int mcs = 0;

	ioctl( fd, TIOCMGET, &mcs);
	return (mcs & TIOCM_CD);
  }
#endif
#ifdef TIOCMODG
  {
	int mcs = 0;

	ioctl(fd, TIOCMODG, &mcs);
	return(mcs & TIOCM_CAR ? 1 : 0);
  }
#endif
	// For those that don't have this capability...
	return -1;
}

void SerialComm::FlushWrite()
{
	if(tcflush(fd,TCOFLUSH)) {
		perror("tcflush");
	}
}

void SerialComm::DrainWrite()
{
	if( tcdrain(fd)) {
		perror("tcdrain");
	}
}

void SerialComm::FlushRead()
{
	if(tcflush(fd,TCIFLUSH)) {
		perror("tcflush");
	}
}

int SerialComm::Close()
{
	RestoreTio();

	close(fd);
	return 0;
}

void SerialComm::GetTio(termios *tio)
{
	if( tcgetattr(fd,tio) ) {
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}
}


void SerialComm::StoreTio()
{
	GetTio(&oldtio);
}

void SerialComm::SetTio(termios *tio)
{
	if( tcsetattr(fd, TCSADRAIN, tio)) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}

void SerialComm::RestoreTio()
{
	SetTio(&oldtio);
}


tcflag_t SerialComm::GetBaudFlag(const char *baud)
{
	unsigned baudRate;

  	if ((baudRate = (atol(baud))) == 0 && baud[0] != '0') baudRate = -1;

	switch (baudRate) {
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		default:
			fprintf(stderr,"Invalid baud-rate %s\n",baud);
			exit(EXIT_FAILURE);
	}
}

void	SerialComm::Write(const char *out, const int len)
{
	int w = 0 ;
	if ( len > 0 ) {
		w = write( fd, out, len );
		if ( w <= 0 ) {
			perror("Write");
			fprintf( stderr,"Error : %d",errno );
			#ifdef EAGAIN
				if(errno == EAGAIN ) {
					usleep(1000);	// Try after 1ms
				}
			#endif
			// exit(EXIT_FAILURE);
		}
	}
}

void SerialComm::Read(char *idata, int &len)
{
	int i;
	int r ;
	ioctl( fd, TIOCINQ, &r);
	r = read( fd, in, sizeof in );
	if( r<=0 ) {
		perror("Read");
		// exit(EXIT_FAILURE);
	}
	else {
		for ( i = 0; i < r ; i++ ) {
			idata[i] = in[i];
		}
	}
	len = r;
}


int	SerialComm::PeekAndRead(char *idata, int &len) {
	return PeekAndRead(idata,len,10000);
}

int	SerialComm::PeekAndRead(char *idata, int &len, int timeout) {
	int result = Peek(timeout);
	if( !result ) {
		Read(idata,len);
	}
	return result;
}

int	SerialComm::Peek() {
	return Peek(10000);
}

int	SerialComm::Peek(int timeout)
{
	Timeout.tv_sec = 0;
	Timeout.tv_sec = timeout;
	FD_ZERO(&readfs);
	FD_SET(fd,&readfs);

	int result = select ( FD_SETSIZE , &readfs, NULL, NULL, &Timeout);
	if( result > 0 ) {
		if ( FD_ISSET( fd, &readfs )) {
			return 0;
		}
	}

	return -1;
}

SerialComm::SerialComm(const char *dev, speed_t brate, bool isCanon)
{
	Open(dev,brate,isCanon);
}

SerialComm::SerialComm(const char *dev, const char *baudRate, bool isCanon)
{
	tcflag_t bRate;

	bRate = GetBaudFlag(baudRate);
	fprintf(stderr,"\nbaudRate=%d, 115200=%d\n",bRate,B115200);
	Open(dev,bRate,isCanon);
	termios a;

	GetTio(&a);
	fprintf(stderr,"\nc_cflag = %d",a.c_cflag);
	fflush(stderr);
}
