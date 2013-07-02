#ifndef __SIGNAL_TOOLS__H__
#define __SIGNAL_TOOLS__H__

#ifndef _WINDOWS_ // This is only for OS-X and Linux.

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits>
#include <stdexcept>

namespace ArvindsTools {

enum SignalsToCapture {
	InterruptSignal = 0x01,
	TerminateSignal = 0x02,
	KillSignal = 0x03,
	TimerSignal = 0x04,
	AbortSignal = 0x05,
	IllegalSignal = 0x06,
	SegmentViolationSignal = 0x07,
	FloatingPointExceptionSignal = 0x08
};


/** A singleton patterned signal handler class which I've
 * written so that I don't pollute the global namespace
 * with utility functions.
 *
 *
 */
class SingletonSignalHandler {

	static SingletonSignalHandler *mySelf; // Will be required to get back to this instance.
	SingletonSignalHandler() {};
	SingletonSignalHandler(SingletonSignalHandler const &) {};
	SingletonSignalHandler& operator=(SingletonSignalHandler const&) const; // Will Never be done.
public:
	static SingletonSignalHandler *getInstance();

	static void captureSignal( int signalType, void (*pfunct)(int) );

	static void endSignalCapture();
};

}

#endif // (Not) _WINDOWS_

#endif // __SIGNAL_TOOLS__H__
