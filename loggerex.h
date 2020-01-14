/*  Copyright (c) 2014, 2019, Mario Ivanèiæ
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

// loggerex.h

/*
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
    This logger functions are thread-safe but timestamps can be little off if calling thread is preempted
    between calls to _logger_make_timestamp() and _logger_msg_ex().

    TODO: move some stuff from macros to logging function and use mutex/critical_section to make it
    thread-safe. Make file names relative to logger initialization call.
*/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    #define LOGGER_SYSLOG 0
#else
    #define LOGGER_SYSLOG 1
#endif // WIN32


// define what to do when ABORT_EXIT() is called
// #define ABORT_EXIT() exit(1)
#define ABORT_EXIT() abort()

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    //#include <processthreadsapi.h>
    // #define GETPID() GetCurrentProcessId()
    #define GETPID() GetCurrentThreadId()
#else
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/syscall.h>
    //#define GETPID() getpid()
    //#define GETPID() gettid()
    #define GETPID() syscall(SYS_gettid)
#endif // WIN32


extern unsigned _logger_log_level;
extern unsigned _logger_debug_mask;
extern unsigned _logger_options;

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
extern void logger_open(const char* log_file_name, unsigned options);
extern void logger_close(void);

// Set log level to one of LOGGER_LEVEL_FATAL, LOGGER_LEVEL_ERROR,
// LOGGER_LEVEL_WARNING, LOGGER_LEVEL_INFO, LOGGER_LEVEL_DEBUG, LOGGER_LEVEL_TRACE.
// _logger_log_level will affect logging using log_fatal, log_error,
// log_warn, log_info, log_debug and log_trace functions.
extern void logger_set_log_level(unsigned level);

// This function is used to set bits in _logger_debug_mask
// Every bit in mask controls one feature that you want to debug.
extern void logger_set_debug_mask(unsigned mask);

// This function is used to get _logger_debug_mask
extern unsigned logger_get_debug_mask(void);


// Enable debuging for feature (bitmask)
extern void logger_enable_debug(unsigned feature);

// Disable debuging for feature (bitmask)
extern void logger_disable_debug(unsigned feature);


// macro for testing log level and debug mask
// test is info level enabled
#define logger_is_info() (_logger_log_level >= LOGGER_LEVEL_INFO)

// test is warning level enabled
#define logger_is_warn() (_logger_log_level >= LOGGER_LEVEL_WARN)

// test is error level enabled
#define logger_is_error() (_logger_log_level >= LOGGER_LEVEL_ERROR)

// test is debug level enabled
#define logger_is_debug() (_logger_log_level >= LOGGER_LEVEL_DEBUG)

// test is trace level enabled
#define logger_is_trace() (_logger_log_level >= LOGGER_LEVEL_TRACE)

// test is debug feature enabled. feature is bitfield mask
#define logger_is_debug_feature(feature) ( _logger_debug_mask & ( feature ) )




// log macros
#if LOGGER_SYSLOG
#define log_fatal_exit(format, ...) \
    do { \
        if(_logger_options & LOGGER_OPTION_SYSLOG) _logger_syslog_fatal("[FATAL] " format "\n", ##__VA_ARGS__ ); \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [FATAL] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
        logger_close(); ABORT_EXIT(); \
    } while(0)

#define log_fatal(format, ...) \
    do { \
        if(_logger_options & LOGGER_OPTION_SYSLOG) _logger_syslog_fatal("[FATAL] " format "\n", ##__VA_ARGS__ ); \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [FATAL] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_error(format, ...) \
    do { \
        if(_logger_log_level & (1 << 0)) { \
            if(_logger_options & LOGGER_OPTION_SYSLOG) _logger_syslog_err("[ERROR] " format "\n", ##__VA_ARGS__ ); \
            if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
                char _log_time_stamp[32]; \
                _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
                _logger_msg_ex("%s (%d) [ERROR] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
            } \
        } \
    } while(0)

#define log_warn(format, ...) \
    do { \
        if(_logger_log_level & (1 << 1)) { \
            if(_logger_options & LOGGER_OPTION_SYSLOG) _logger_syslog_warn("[WARN] " format "\n", ##__VA_ARGS__ ); \
            if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
                char _log_time_stamp[32]; \
                _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
                _logger_msg_ex("%s (%d) [WARN] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
            } \
        } \
    } while(0)

#define log_info(format, ...) \
    do { \
        if(_logger_log_level & (1 << 2)) { \
            if(_logger_options & LOGGER_OPTION_SYSLOG) _logger_syslog_info("[INFO] " format "\n", ##__VA_ARGS__ ); \
            if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
                char _log_time_stamp[32]; \
                _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
                _logger_msg_ex("%s (%d) [INFO] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
            } \
        } \
    } while(0)


#else // ! LOGGER_SYSLOG

#define log_fatal_exit(format, ...) \
    do { \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [FATAL] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
        logger_close(); ABORT_EXIT(); \
    } while(0)

#define log_fatal(format, ...) \
    do { \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [FATAL] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_error(format, ...) \
    do { \
        if(_logger_log_level & (1 << 0) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) \
        { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [ERROR] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_warn(format, ...) \
    do { \
        if(_logger_log_level & (1 << 1) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) \
        { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [WARN] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_info(format, ...) \
    do { \
        if(_logger_log_level & (1 << 2) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) \
        { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [INFO] " format "\n", _log_time_stamp, GETPID(), ##__VA_ARGS__ ); \
        } \
    } while(0)
#endif // LOGGER_SYSLOG




#define log_debug(feature, format, ...) \
    do { \
        if( logger_is_debug() && logger_is_debug_feature( (feature) ) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [%s] %s @ %s:%d " format "\n", _log_time_stamp, GETPID(), #feature, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    } while(0)



#define log_trace_enter(format, ...) \
    do { \
        if( logger_is_trace() && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [ENTERING %s] @ %s:%d " format "\n", _log_time_stamp, GETPID(), __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    } while(0)



#define log_trace_exit(format, ...) \
    do { \
        if( logger_is_trace() && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [EXITING %s] @ %s:%d " format "\n", _log_time_stamp, GETPID(), __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    } while(0)

#ifdef __cplusplus
#include <typeinfo>
const char* _logger_stralpha(const char* name);
#define log_trace_member_enter(format, ...) \
    do { \
        if( logger_is_trace() && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [ENTERING %s::%s] @ %s:%d " format "\n", _log_time_stamp, GETPID(), _logger_stralpha(typeid(*this).name()), __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    } while(0)



#define log_trace_member_exit(format, ...) \
    do { \
        if( logger_is_trace() && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char _log_time_stamp[32]; \
            _logger_make_timestamp(_log_time_stamp, sizeof(_log_time_stamp)); \
            _logger_msg_ex("%s (%d) [EXITING %s::%s] @ %s:%d " format "\n", _log_time_stamp, GETPID(), _logger_stralpha(typeid(*this).name()), __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    } while(0)
#endif

// ###################################  LOGGER PRIVETE API  ###################################


extern void _logger_make_timestamp(char* buffer, unsigned buff_size);

// Core log function, variadic
extern void _logger_msg_ex(const char* format, ...);

// syslog functions
void _logger_syslog_open(void);
void _logger_syslog_fatal(const char *format, ...);
void _logger_syslog_err(const char *format, ...);
void _logger_syslog_warn(const char *format, ...);
void _logger_syslog_info(const char *format, ...);


#ifdef __cplusplus
}
#endif

#endif // __LOGGER_H__
