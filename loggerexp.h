/*  Copyright (c) 2014, 2019, Mario Ivančić
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// loggerexp.h

/*
    This logger is protected by mutex. On Linux it should be linked against pthread library.

    NOTE:
    Since log_* are implemented as macros some macro arguments are evaluated several times
    and some functions are called in between.
    If you are logging errno or strerror(errno) it is strongly recomended to make a local
    copy of errno and supply it to macro.

    Example:

    int t = errno;
    log_info("Error %s", strerror(t));

    NOTE:
    format string MUST be literal string because it is concatenated with other literals in macros below.

    NOTE:
    This logger functions are protected by mutex/critical section to be thread-safe

    TODO: Make file names relative to logger initialization call.
*/

#ifndef LOGGEREXP_H_INCLUDED__
#define LOGGEREXP_H_INCLUDED__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #define LOGGER_SYSLOG 0
#else
    #define LOGGER_SYSLOG 1
#endif // _WIN32


// define what to do when ABORT_EXIT() is called
// #define ABORT_EXIT() exit(1)
#define ABORT_EXIT() abort()

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    //#include <processthreadsapi.h>
    // #define GETPID() GetCurrentProcessId()
    #define GETPID() GetCurrentThreadId()
#else
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <pthread.h>
    //#define GETPID() getpid()
    //#define GETPID() gettid()
    // this will return kernel task ID which is not the same as pthread_t object
    #define GETPID() syscall(SYS_gettid)
#endif // _WIN32


extern unsigned logger_log_level_;
extern unsigned logger_debug_mask_;
extern unsigned logger_trace_mask_;
extern unsigned logger_options_;

// logger levels
enum
{
    LOGGER_LEVEL_FATAL = 0,
    LOGGER_LEVEL_ERROR = 1,
    LOGGER_LEVEL_WARNING = 2,
    LOGGER_LEVEL_WARN = 2,
    LOGGER_LEVEL_INFO = 3,
    LOGGER_LEVEL_DEBUG = 4,
    LOGGER_LEVEL_TRACE = 5,
};


// logger options
enum
{
    LOGGER_OPTION_KEEP_FILE_OPEN    = 1 << 0,   // don't close file after every call
    LOGGER_OPTION_FLUSH_FILE        = 1 << 1,   // flush file after every call
    LOGGER_OPTION_FILE              = 1 << 2,   // log to file (default)
    LOGGER_OPTION_SYSLOG            = 1 << 3,   // log to syslog
    LOGGER_OPTION_STDERR            = 1 << 4,   // log to stderr
    LOGGER_OPTION_MILLISECONDS      = 1 << 5,   // enable milliseconds in timestamps
};

// Set log file name and options. Caller must provide storage for string
// log_file_name. If LOGGER_OPTION_KEEP_FILE_OPEN option is specified we will open
// named log file and save file handle for later use.
extern void logger_open_ex(const char* log_file_name, unsigned options, const char* file);
#define logger_open(logfile, opt) logger_open_ex((logfile), (opt), __FILE__)
extern void logger_close(void);

// Set log level to one of LOGGER_LEVEL_FATAL, LOGGER_LEVEL_ERROR,
// LOGGER_LEVEL_WARNING, LOGGER_LEVEL_INFO, LOGGER_LEVEL_DEBUG, LOGGER_LEVEL_TRACE.
// logger_log_level_ will affect logging using log_fatal, log_error,
// log_warn, log_info, log_debug and log_trace functions.
extern void logger_set_log_level(unsigned level);

// This function is used to set bits in logger_debug_mask_
// Every bit in mask controls one feature that you want to debug.
extern void logger_set_debug_mask(unsigned mask);

// This function is used to get logger_debug_mask_
extern unsigned logger_get_debug_mask(void);

// This function is used to set bits in logger_trace_mask_
// Every bit in mask controls one feature that you want to trace.
extern void logger_set_trace_mask(unsigned mask);

// This function is used to get logger_trace_mask_
extern unsigned logger_get_trace_mask(void);


// Enable debuging for feature (bitmask)
extern void logger_enable_debug(unsigned feature);

// Disable debuging for feature (bitmask)
extern void logger_disable_debug(unsigned feature);

// Enable trace for feature (bitmask)
extern void logger_enable_trace(unsigned feature);

// Disable trace for feature (bitmask)
extern void logger_disable_trace(unsigned feature);

// lock / unlock functions for logger
extern void logger_lock(void);
extern void logger_unlock(void);


// macro for testing log level and debug mask
// test is info level enabled
#define logger_is_info() (logger_log_level_ >= LOGGER_LEVEL_INFO)

// test is warning level enabled
#define logger_is_warn() (logger_log_level_ >= LOGGER_LEVEL_WARN)

// test is error level enabled
#define logger_is_error() (logger_log_level_ >= LOGGER_LEVEL_ERROR)

// test is debug level enabled
#define logger_is_debug() (logger_log_level_ >= LOGGER_LEVEL_DEBUG)

// test is trace level enabled
#define logger_is_trace() (logger_log_level_ >= LOGGER_LEVEL_TRACE)

// test is debug feature enabled. feature is bitfield mask
#define logger_is_debug_feature(feature) ( logger_debug_mask_ & ( feature ) )

// test is trace feature enabled. feature is bitfield mask
#define logger_is_trace_feature(feature) ( logger_trace_mask_ & ( feature ) )


#define log_fatal(format, ...) \
    do { \
        char logger_tmp_buffer__[512]; \
        logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), 0, "[FATAL]", 0, 0, 0, 0, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
    } while(0)

#define log_fatal_exit(format, ...) \
    do { \
        char logger_tmp_buffer__[512]; \
        logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), 0, "[FATAL]", 0, 0, 0, 0, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        logger_close(); ABORT_EXIT(); \
    } while(0)

#define log_error(format, ...) \
    do { \
        if(logger_is_error()) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), 1, "[ERROR]", 0, 0, 0, 0, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#define log_warn(format, ...) \
    do { \
        if(logger_is_warn()) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), 2, "[WARN]", 0, 0, 0, 0, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_info(format, ...) \
    do { \
        if(logger_is_info()) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), 2, "[INFO]", 0, 0, 0, 0, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#define log_debug(feature, format, ...) \
    do { \
        if( (feature) & DEBUG_STATIC_MASK && logger_is_debug() && logger_is_debug_feature( (feature) ) && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "[" #feature "]", 0, __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#define log_trace_enter(format, ...) \
    do { \
        if( logger_is_trace() && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  >>>>  ", 0, __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#define log_trace_exit(format, ...) \
    do { \
        if( logger_is_trace() && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  <<<<  ", 0, __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)



#define log_condtrace_enter(cond, format, ...) \
    do { \
        if( (cond) & TRACE_STATIC_MASK && logger_is_trace() && logger_is_trace_feature((cond)) && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  >>>>  ", 0, __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#define log_condtrace_exit(cond, format, ...) \
    do { \
        if( (cond) & TRACE_STATIC_MASK && logger_is_trace() && logger_is_trace_feature((cond)) && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  <<<<  ", 0, __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#ifdef __cplusplus
#include <typeinfo>

#define log_trace_member_enter(format, ...) \
    do { \
        if( logger_is_trace() && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  >>>>  ", typeid(*this).name(), __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)



#define log_trace_member_exit(format, ...) \
    do { \
        if( logger_is_trace() && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  <<<<  ", typeid(*this).name(), __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_condtrace_member_enter(cond, format, ...) \
    do { \
        if( (cond) & TRACE_STATIC_MASK && logger_is_trace() && logger_is_trace_feature((cond)) && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  >>>>  ", typeid(*this).name(), __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#define log_condtrace_member_exit(cond, format, ...) \
    do { \
        if( (cond) & TRACE_STATIC_MASK && logger_is_trace() && logger_is_trace_feature((cond)) && logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char logger_tmp_buffer__[512]; \
            logger_msg_ex_(logger_tmp_buffer__, sizeof(logger_tmp_buffer__), -1, "  <<<<  ", typeid(*this).name(), __func__, __FILE__, __LINE__, "%s " format "\n", logger_tmp_buffer__, ##__VA_ARGS__ ); \
        } \
    } while(0)

#endif



// ###################################  LOGGER PRIVETE API  ###################################


// Core log function, variadic
extern void logger_msg_ex_(char* logger_tmp_buffer__, unsigned len, int nseverity, const char* severity,
                           const char* theclass, const char* func, const char* file,
                           int line, const char* format, ...);


#ifdef __cplusplus
}
#endif

#endif // LOGGEREXP_H_INCLUDED__

