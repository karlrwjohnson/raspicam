#include <iostream>
#include <sstream>

#ifndef LOG_H
#define LOG_H

#define THROW_ERROR(error) \
	std::stringstream err_ss; \
	err_ss << __FUNCTION__ << ":" << __LINE__ << " " << error;\
	throw std::runtime_error(err_ss.str())

//#define TRACE_ON
#define MESSAGE_ON
#define WARNING_ON
#define ERROR_ON

#define LOG_TEMPLATE(level, x) \
	std::cerr << std::dec << "[" level "] " << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << " " << x << std::endl;

#ifdef TRACE_ON
# define IF_TRACE(statement) statement
# define TRACE(x) \
	LOG_TEMPLATE(" TRACE ", x)
# define TRACE_ENTER \
	TRACE("Entering")
# define TRACE_EXIT \
	TRACE("Exiting")
#else
# define IF_TRACE(statement)
# define TRACE(x)
# define TRACE_ENTER
# define TRACE_EXIT
#endif

#ifdef MESSAGE_ON
# define MESSAGE(x) \
	LOG_TEMPLATE("MESSAGE", x)
#else
# define MESSAGE(x)
#endif

#ifdef WARNING_ON
# define WARNING(x) \
	LOG_TEMPLATE("WARNING", x)
#else
# define WARNING(x)
#endif

#ifdef ERROR_ON
# define ERROR(x) \
	LOG_TEMPLATE(" ERROR ", x)
#else
# define ERROR(x)
#endif

#endif // LOG_H

