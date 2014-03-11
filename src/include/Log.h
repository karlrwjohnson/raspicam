#include <iostream>
#include <sstream>

#ifndef LOG_H
#define LOG_H

#define THROW_ERROR(error) \
	std::stringstream err_ss; \
	err_ss << __FUNCTION__ << ":" << __LINE__ << " " << error;\
	throw std::runtime_error(err_ss.str())

#define USE_COLOR
#define TRACE_ON
#define MESSAGE_ON
#define WARNING_ON
#define ERROR_ON

#ifdef USE_COLOR
# define RESET_COLOR       "\033[0m"
# define TRACE_COLOR       "\033[1;30m"
# define MESSAGE_COLOR     "\033[0m"
# define WARNING_COLOR     "\033[1;33m"
# define ERROR_COLOR       "\033[1;31m"
# define PUNCTUATION_COLOR "\033[0;34m"
# define BOILERPLATE_COLOR "\033[0;36m"
#else
# define RESET_COLOR       ""
# define TRACE_COLOR       ""
# define MESSAGE_COLOR     ""
# define WARNING_COLOR     ""
# define ERROR_COLOR       ""
# define PUNCTUATION_COLOR ""
# define BOILERPLATE_COLOR ""
#endif

#define LOG_TEMPLATE(level, color, msg) \
	std::cerr << std::dec \
		<< PUNCTUATION_COLOR "[" color level PUNCTUATION_COLOR "] " \
		<< BOILERPLATE_COLOR << __FILE__ << PUNCTUATION_COLOR ":" \
		<< BOILERPLATE_COLOR << __LINE__ << PUNCTUATION_COLOR ":" \
		<< BOILERPLATE_COLOR << __FUNCTION__ << PUNCTUATION_COLOR ": " \
		<< color << msg << RESET_COLOR << std::endl;

#ifdef TRACE_ON
# define IF_TRACE(statement) statement
# define TRACE(x) \
	LOG_TEMPLATE(" TRACE ", TRACE_COLOR, x)
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
	LOG_TEMPLATE("MESSAGE", MESSAGE_COLOR, x)
#else
# define MESSAGE(x)
#endif

#ifdef WARNING_ON
# define WARNING(x) \
	LOG_TEMPLATE("WARNING", WARNING_COLOR, x)
#else
# define WARNING(x)
#endif

#ifdef ERROR_ON
# define ERROR(x) \
	LOG_TEMPLATE(" ERROR ", ERROR_COLOR, x)
#else
# define ERROR(x)
#endif

#endif // LOG_H

