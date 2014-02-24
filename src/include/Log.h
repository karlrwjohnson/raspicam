

#define THROW_ERROR(error) \
	stringstream err_ss; \
	err_ss << __FUNCTION__ << ":" << __LINE__ << " " << error;\
	throw runtime_error(err_ss.str())

//#define TRACE_ON
#ifdef TRACE_ON
# define IF_TRACE(statement) statement
# define TRACE(x) \
	cout << __FUNCTION__ << ":" << __LINE__ << " " << x << "\n"
# define TRACE_ENTER \
	cout << "-- Trace -- Entering " << __FUNCTION__
# define TRACE_EXIT \
	cout << "-- Trace -- Exiting " << __FUNCTION__
#else
# define IF_TRACE(statement)
# define TRACE(x)
# define TRACE_ENTER
# define TRACE_EXIT
#endif


