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
