/*
 * SignalTools.cpp
 *
 *  Created on: Jun 26, 2013
 *      Author: arvind
 */
#ifndef _WINDOWS_ // Never use this in Windoze... ;-)

#include "SignalTools.h"

namespace ArvindsTools {

SingletonSignalHandler* SingletonSignalHandler::mySelf=NULL; // The only pointer to this instance


SingletonSignalHandler * SingletonSignalHandler::getInstance() {
		if( !mySelf )
			SingletonSignalHandler::mySelf = new SingletonSignalHandler;
		return mySelf;
	}

void SingletonSignalHandler::captureSignal( int signalType, void (*pfunct)(int) ) {
		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );
		sa.sa_handler = pfunct;
		switch( signalType ) {
			case InterruptSignal:
				if( sigaction(SIGINT, &sa, NULL)) { perror("sigaction");
				throw std::runtime_error("Could not capture Interrupt signal."); }
				break;
			case TerminateSignal:
				if( sigaction(SIGTERM, &sa, NULL) ) { perror("sigaction");
					throw std::runtime_error("Could not capture Terminate signal."); }
				break;
			case KillSignal:
				if( sigaction(SIGKILL, &sa, NULL) ) { perror("sigkill");
					throw std::runtime_error("Could not capture Kill signal."); }
				break;
			default:
				throw std::invalid_argument("Unknown/Unimplemented signal.");
				break;
		}
}

void SingletonSignalHandler::endSignalCapture() {
	delete SingletonSignalHandler::mySelf;
	SingletonSignalHandler::mySelf = NULL;
}

}

#endif
