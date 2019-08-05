/*  Copyright (c) 2014, Mario Ivancic
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

// loggerux.h

/*
    NOTE:
    Since log_* are implemented as macros some macro arguments are evaluated several times
    and some functions are called in between.
    If you are logging errno or strerror(errno) it is strongly recomended to make a local
    copy of errno and supply it to macro.

    Example:

    int t = errno;
    log_info("Error %s", strerror(t));
*/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif


#define LOGGER_SYSLOG 1
#define _TIMEVAL_DEFINED 1


extern unsigned _logger_log_level;
extern unsigned _logger_options;
extern int _logger_syslog_facility;
extern char* logger_syslog_ident;

// logger levels
enum
{
    LOGGER_LEVEL_FATAL = 0,
    LOGGER_LEVEL_ERROR = 1,
    LOGGER_LEVEL_WARNING = 2,
    LOGGER_LEVEL_WARN = 2,
    LOGGER_LEVEL_INFO = 3,
};


// logger options
enum
{
    LOGGER_OPTION_KEEP_FILE_OPEN    = 1 << 0,   // don't close file after every call
    LOGGER_OPTION_FLUSH_FILE        = 1 << 1,   // flush file after every call
    LOGGER_OPTION_FILE              = 1 << 2,   // log to file (default)
    LOGGER_OPTION_SYSLOG            = 1 << 3,   // log to syslog
    LOGGER_OPTION_STDERR            = 1 << 4,   // log to stderr
};

// Set log file name and options. Caller must provide storage for string
// log_file_name. If LOGGER_OPTION_KEEP_FILE_OPEN option is specified we will open
// named log file and save file handle for later use.
extern void logger_open(const char* log_file_name, unsigned options);
extern void logger_close(void);

// Set log level to one of LOGGER_LEVEL_FATAL, LOGGER_LEVEL_ERROR,
// LOGGER_LEVEL_WARNING, LOGGER_LEVEL_INFO.
// _logger_log_level will affect logging using log_fatal, log_error,
// log_warn, log_info and log_debug functions.
extern void logger_set_level(unsigned level);


extern void logger_make_timestamp(char* buffer, unsigned buff_size);

// Core log function, variadic
extern void logger_msg_ex(const char* format, ...);

// syslog functions
void logger_syslog_open(void);
void logger_syslog_fatal(const char *format, ...);
void logger_syslog_err(const char *format, ...);
void logger_syslog_warn(const char *format, ...);
void logger_syslog_info(const char *format, ...);

// macro for testing log level and debug mask
// test is info level enabled
#define logger_is_info() (_logger_log_level & (1 << 2))

// test is warning level enabled
#define logger_is_warn() (_logger_log_level & (1 << 1))

// test is error level enabled
#define logger_is_error() (_logger_log_level & (1 << 0))

// test is debug feature enabled
#define logger_is_debug(feature) \
    ( ( (feature) < 29 ) && ( _logger_log_level & ( 8 << (feature) ) ) )




// log macros
#ifdef LOGGER_SYSLOG
#define log_fatal_exit(format, ...) \
    do { \
        if(_logger_options & LOGGER_OPTION_SYSLOG) logger_syslog_fatal("[FATAL] " format "\n", ##__VA_ARGS__ ); \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [FATAL] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
        logger_close(); exit(1); \
    } while(0)

#define log_fatal(format, ...) \
    do { \
        if(_logger_options & LOGGER_OPTION_SYSLOG) logger_syslog_fatal("[FATAL] " format "\n", ##__VA_ARGS__ ); \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [FATAL] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_error(format, ...) \
    do { \
        if(_logger_log_level & (1 << 0)) { \
            if(_logger_options & LOGGER_OPTION_SYSLOG) logger_syslog_err("[ERROR] " format "\n", ##__VA_ARGS__ ); \
            if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
                char __log_time_stamp[32]; \
                logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
                logger_msg_ex("%s (%d) [ERROR] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
            } \
        } \
    } while(0)

#define log_warn(format, ...) \
    do { \
        if(_logger_log_level & (1 << 1)) { \
            if(_logger_options & LOGGER_OPTION_SYSLOG) logger_syslog_warn("[WARN] " format "\n", ##__VA_ARGS__ ); \
            if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
                char __log_time_stamp[32]; \
                logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
                logger_msg_ex("%s (%d) [WARN] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
            } \
        } \
    } while(0)

#define log_info(format, ...) \
    do { \
        if(_logger_log_level & (1 << 2)) { \
            if(_logger_options & LOGGER_OPTION_SYSLOG) logger_syslog_info("[INFO] " format "\n", ##__VA_ARGS__ ); \
            if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
                char __log_time_stamp[32]; \
                logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
                logger_msg_ex("%s (%d) [INFO] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
            } \
        } \
    } while(0)


#else // ! LOGGER_SYSLOG
#define log_fatal_exit(format, ...) \
    do { \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [FATAL] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
        logger_close(); \
        exit(1); \
    } while(0)

#define log_fatal(format, ...) \
    do { \
        if(_logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [FATAL] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_error(format, ...) \
    do { \
        if(_logger_log_level & (1 << 0) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) \
        { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [ERROR] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_warn(format, ...) \
    do { \
        if(_logger_log_level & (1 << 1) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) \
        { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [WARN] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
    } while(0)

#define log_info(format, ...) \
    do { \
        if(_logger_log_level & (1 << 2) && _logger_options & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR)) \
        { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [INFO] " format "\n", __log_time_stamp, getpid(), ##__VA_ARGS__ ); \
        } \
    } while(0)
#endif // LOGGER_SYSLOG

// This function is used to set bits 3 .. 31 in _logger_log_level
// from mask bits 0 .. 28. Every bit in mask controls one
// feature that you want to debug.
extern void logger_set_debug_mask(unsigned mask);

// This function is used to get bits 3 .. 31 from _logger_log_level
// shifted to posiotions 0 .. 28
extern unsigned logger_get_debug_mask(void);


// Enable debuging for feature (in range 0 - 28)
extern void logger_enable_debug(unsigned feature);

// Disable debuging for feature (in range 0 - 28)
extern void logger_disable_debug(unsigned feature);

// feature is int in range 0 - 28

#define log_debug(feature, format, ...) \
    do { \
        if( logger_is_debug( (feature) ) && _logger_options & LOGGER_OPTION_FILE) { \
            char __log_time_stamp[32]; \
            logger_make_timestamp(__log_time_stamp, sizeof(__log_time_stamp)); \
            logger_msg_ex("%s (%d) [%s] %s @ %s:%d " format "\n", __log_time_stamp, #feature, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    } while(0)


#ifdef __cplusplus
}
#endif

#endif // __LOGGER_H__
