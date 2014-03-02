#include <pthread.h>
#include <stdexcept>
#include "Log.h"

template < class Class, class ArgumentType >
struct _CallbackArgs
{
	Class* object_p;
	void (Class::* method_p)(ArgumentType);
	ArgumentType arg;
};

template < class Class, class ArgumentType >
void*
_pthread_create_using_method_callback (void* spec)
{
	TRACE_ENTER;

	// Convert the arguments pointer into a statically-allocated object and
	// free the original pointer
	_CallbackArgs<Class, ArgumentType> *callbackArgs_p = static_cast< _CallbackArgs<Class, ArgumentType>* >(spec);
	_CallbackArgs<Class, ArgumentType> callbackArgs = *callbackArgs_p;
	delete callbackArgs_p;

	try
	{
		// Call the method on the object using the argument as the sole parameter.
		((callbackArgs.object_p)->*(callbackArgs.method_p))(callbackArgs.arg);
	}
	catch (std::runtime_error e)
	{
		ERROR("Uncaught exception in thread: " << e.what());
	}

	TRACE_EXIT;
	return NULL;
}

/**
 * A wrapper around pthread_create which can call a non-static method of an
 * object.
 *
 * @tparam Class         The base object's class
 * @tparam ArgumentType  The type of the argument to pass to the method
 * @param object         The object hosting the method
 * @param method         The method to call
 * @param arg            The argument to pass to the method.
 *                       (Might I recommend a shared_ptr?)
 */

template < class Class, class ArgumentType >
pthread_t
pthread_create_using_method
	( Class& object, void (Class::* method_p)(ArgumentType), ArgumentType arg )
{
	TRACE_ENTER;

	pthread_t retval;

	_CallbackArgs<Class, ArgumentType> *callbackArgs_p = new _CallbackArgs<Class, ArgumentType>();
	callbackArgs_p->object_p = &object;
	callbackArgs_p->method_p = method_p;
	callbackArgs_p->arg = arg;

	int err = pthread_create(&retval, NULL, _pthread_create_using_method_callback<Class, ArgumentType>, callbackArgs_p);
	if (err)
	{
		THROW_ERROR("Unable to create thread: " << strerror(err));
	}

	TRACE_EXIT;
	return retval;
}

///**
// * Encapsulates starting a thread which calls a member function of a class
// */
//template <class Class, class ArgumentType, class ReturnType>
//class Thread {
//
//	/**
//	 * Entry points must be member functions of their parent class and take
//	 * one parameter.
//	 */
//	typedef void (T::*entry_point_fn_t)(ParamType);
//
//  private:
//
//	entry_point_fn_t _entryPoint;
//
//	pthread_t handle;
//
//	static void*
//	_callback (void* argument);
//
//  public:
//
//  	Thread (Class object, entry_point_fn_t entryPoint, argument);
//
//  	void
//  	join();
//
//}
//
//#include "Thread.h"
//
//template <class Class, class Argument>
//struct ThreadSpec
//{
//	Class *object;
//	Argument argument;
//}
//
//Thread::Thread (Class object, entry_point_fn_t entryPoint, argument):
//	_entryPoint(entryPoint)
//{
//	int err = pthread_create(&handle, NULL, _callback, 
//	if (pthread_create
//}
//
//void
//Thread::_callback (void* argument)
//{
//	
//}
//
//void
//Thread::join ()
//{
//	int err = pthread_join(&handle
//}
