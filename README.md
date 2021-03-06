# logger
Minimalistic portable logger implementation in C. 
It is making three calls to fprintf/vfprintf for every log line, so it is NOT thread-safe.
Provides FATAL, ERROR, WARNING, INFO and DEBUG logging levels and timestamps with 1 second resolution.

# loggerux
A better logger implementation which makes single call to fprintf. 
It is thread-safe but timestamps can be little off if calling thread is 
preempted between calls to logger_make_timestamp() and logger_msg_ex().
Currently works only on Linux but it could work on Windows with little ifdefs.
Provides FATAL, ERROR, WARNING and INFO logging levels and 29 DEBUG features. 
Timestamps are with 1 millisecond resolution (using gettimeofday() on Linux).


# loggerex
A even better logger implementation which makes single call to fprintf. 
It is thread-safe but timestamps can be little off if calling thread is 
preempted between calls to _logger_make_timestamp() and _logger_msg_ex().
It have somewhat different API from logger and loggerux.
Works on Linux and Windows XP.
Provides FATAL, ERROR, WARNING, INFO, DEBUG and TRACE logging levels and 32 DEBUG features. 
Timestamps are with 1 millisecond resolution using gettimeofday() on Linux and 
GetSystemTimePreciseAsFileTime() / GetSystemTimeAsFileTime() on Windows.

# loggerexp
A even better logger implementation which locks internals while getting timestamp and writing 
to log file. Makes a single call to fprintf. 
It is thread-safe when wtites to file. Syslog write is not protected by mutex.
The API is compatible to loggerux API but it is expanded to include some new features.
Works on Linux, Windows XP and Windows 7.
Provides FATAL, ERROR, WARNING, INFO, DEBUG and TRACE logging levels, 32 DEBUG features,
and 32 TRACE features. Debug and trace functionality have static masks to exclude unneeded
stuff at compile time.
Timestamps are with 1 millisecond resolution using gettimeofday() on Linux and 
GetSystemTimePreciseAsFileTime() / GetSystemTimeAsFileTime() on Windows.

