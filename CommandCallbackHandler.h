/*
 * CommandCallbackHandler.h
 *
 *  Created on: Jun 26, 2013
 *      Author: Arvind A. de Menezes Pereira
 */

#ifndef COMMANDCALLBACKHANDLER_H_
#define COMMANDCALLBACKHANDLER_H_

#include <string>

namespace ArvindsTools {

typedef void (*CommandHandlerFunc)(std::string);


/** A class designed to be used to handle callbacks based
 * upon commands received in the form of strings in this case.
 *
 * This is meant to be inherited and specialized according
 * to the command type.
 */
class CommandCallbackHandler {
protected:
	std::string funcName;
 	CommandHandlerFunc myFunc;
	bool funcRegistered;

public:
	/** Construct a command callback handler */
	CommandCallbackHandler() {
		myFunc = NULL;
		funcRegistered = false;
	}

	/** Add a callback which can be called based upon some criteria.
	 *
	 * @param _funcName a Name given to this callback. Used mainly to identify it.
	 * @param _myFunc a pointer to the function which is of type void Func(std::string).
	 *
	 *
	 * */
	virtual bool AddFunction( std::string _funcName, CommandHandlerFunc _myFunc ) {
		funcName       = _funcName;
		myFunc         = _myFunc;
		funcRegistered = true;
		return funcRegistered;
	}


	/** Get the functions name
	 *
	 * @return funcName the name of the callback registered.
	 */
	std::string GetCallbackName() {
		return funcName;
	}


	/** Call the function given this string argument
	 * This method is designed to be over-ridden polymorphically.
	 *
	 * One might want to change the behavior of this class dynamically
	 * based upon the contents of the string str. This is useful
	 * when say the string contains data which needs to match before
	 * we attempt to execute the callback function. The over-ridden
	 * callback function could look in the string for information which
	 * may help it determine if this particular callback should be
	 * executed (for eg.)
	 *
	 * @param str
	 * @return true if a function is registered (since it executes it), false otherwise.
	 *
	 */
	virtual bool CallFunction( std::string str ) {
		if( funcRegistered ) {
			myFunc( str );
			return true;
		}
		return false;
	}

};


}

#endif /* COMMANDCALLBACKHANDLER_H_ */
