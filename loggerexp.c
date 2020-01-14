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

// simple logging for C


#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include "loggerexp.h"

#if LOGGER_SYSLOG
#include <syslog.h>   // syslog(3), openlog(3), closelog(3)
#include <unistd.h>
#endif // LOGGER_SYSLOG





/*
    TODO:
    On Windows,
    LOG_TO_DEBUGGER: log via OutputDebugString().
    LOG_TO_EVENTLOG: log via ReportEvent(). Note: must link to Advapi32.lib.
*/



// logger_log_level_ constants
// FATAL is 0 (never masked)
// ERROR is 1
// WARN is 2
// INFO is 3
// DEBUG is 4
// TRACE is 5
unsigned logger_log_level_ = 0;

// debug level bits (0 .. 31) are for debugging using log_debug_ex() function
unsigned logger_debug_mask_ = 0;
unsigned logger_trace_mask_ = 0;

unsigned logger_options_ = LOGGER_OPTION_FILE;
static const char* log_file = 0;
static FILE* fp = 0;

#ifdef _WIN32
CRITICAL_SECTION mutex;
#else
pthread_mutex_t mutex;
#endif // _WIN32

static char *file_name_prefix = 0;

/*
    Code for logging to syslog

    severity can be:
    LOG_EMERG      system is unusable
    LOG_ALERT      action must be taken immediately
    LOG_CRIT       critical conditions
    LOG_ERR        error conditions
    LOG_WARNING    warning conditions
    LOG_NOTICE     normal, but significant, condition
    LOG_INFO       informational message
    LOG_DEBUG      debug-level message

    syslog_facility can be LOG_USER, LOG_DAEMON, LOG_LOCAL0 .. LOG_LOCAL7
    syslog_ident is prepended to every message (typically program name).
    If syslog_ident is NULL, the program name is used.
*/
#if LOGGER_SYSLOG
static int syslog_facility = LOG_DAEMON;
static char* syslog_ident = 0;
static int syslog_open = 0;

static void logger_syslog_open_(void)
{
    if(! syslog_open)
    {
        // openlog(syslog_ident, LOG_PID|LOG_NDELAY|LOG_CONS, syslog_facility);
        openlog(syslog_ident, LOG_NDELAY|LOG_CONS, syslog_facility);
        syslog_open = 1;
    }
}

#endif  // LOGGER_SYSLOG





// Set log file name and options. Caller must provide storage for string
// log_file_name. If LOGGER_OPTION_KEEP_FILE_OPEN option is specified we will open
// named log file and save file handle for later use.
void logger_open_ex(const char* log_file_name, unsigned options, const char* file)
{
    log_file = log_file_name;
    logger_options_ = options;

#ifdef _WIN32
    InitializeCriticalSection(&mutex);
#else
    pthread_mutexattr_t attr;
	pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &attr);
#endif // _WIN32

    // we have to find file_name_prefix to the last directory separator in file
    file_name_prefix = strdup(file);
    if(file_name_prefix)
    {
        char *p1 = strrchr(file_name_prefix, '/');
        char *p2 = strrchr(file_name_prefix, '\\');
        if(p1 && !p2) p1[1] = 0;
        else if(!p1 && p2) p2[1] = 0;
        else if(p1 > p2) p1[1] = 0;
        else p2[1] = 0;
    }


    if(logger_options_ & LOGGER_OPTION_KEEP_FILE_OPEN)
    {
        if(logger_options_ & LOGGER_OPTION_FILE)
        {
            if(fp) fclose(fp);
            fp = fopen(log_file, "a");
        }
#if LOGGER_SYSLOG
        if(logger_options_ & LOGGER_OPTION_SYSLOG)
        {
            logger_syslog_open_();
        }
#endif // LOGGER_SYSLOG
    }
}



// this function will close log file (if open) and set file handle to NULL.
void logger_close(void)
{
    if(fp)
    {
        fclose(fp);
        fp = 0;
    }
    if(file_name_prefix)
    {
        free(file_name_prefix);
        file_name_prefix = 0;
    }
#if LOGGER_SYSLOG
    if(syslog_open)
    {
        closelog();
        syslog_open = 0;
    }
#endif // LOGGER_SYSLOG

#ifdef _WIN32
    DeleteCriticalSection(&mutex);
#else
    pthread_mutex_destroy(&mutex);
#endif // _WIN32
}


// Set log level to one of LOGGER_LEVEL_FATAL, LOGGER_LEVEL_ERROR,
// LOGGER_LEVEL_WARNING, LOGGER_LEVEL_INFO.
// logger_log_level_ will affect logging using log_fatal, log_error,
// log_warn, log_info and log_debug functions.
void logger_set_log_level(unsigned level)
{
    logger_log_level_ = level;
}


// This function is used to set bits in logger_debug_mask_
// Every bit in mask controls one feature that you want to debug.
void logger_set_debug_mask(unsigned mask)
{
    logger_debug_mask_ = mask;
}


// This function is used to get logger_debug_mask_
unsigned logger_get_debug_mask(void)
{
    return logger_debug_mask_;
}


// Enable debuging for feature (bitmask)
void logger_enable_debug(unsigned feature)
{
    logger_debug_mask_ |= feature;
}


// Disable debuging for feature (bitmask)
void logger_disable_debug(unsigned feature)
{
    logger_debug_mask_ &= ~feature;
}

// Enable trace for feature (bitmask)
void logger_enable_trace(unsigned feature)
{
    logger_trace_mask_ |= feature;
}

// Disable trace for feature (bitmask)
void logger_disable_trace(unsigned feature)
{
    logger_trace_mask_ &= ~feature;
}


// This function is used to set bits in logger_trace_mask_
// Every bit in mask controls one feature that you want to trace.
void logger_set_trace_mask(unsigned mask)
{
    logger_trace_mask_ = mask;
}


// This function is used to get logger_trace_mask_
unsigned logger_get_trace_mask(void)
{
    return logger_trace_mask_;
}



static void make_timestamp(char* buffer, unsigned buff_size)
{
    struct tm TM, *ptm;
    unsigned ms = 0;

#ifdef WIN32
    /* 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 00:00 UTC */
    FILETIME               filetime;
    ULARGE_INTEGER         x;
    ULONGLONG              usec;
    /* microseconds betweeen Jan 1,1601 and Jan 1,1970 */
    static const ULONGLONG epoch_offset_us = 11644473600000000ULL;

//#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
//    GetSystemTimePreciseAsFileTime(&filetime);
//#else
    GetSystemTimeAsFileTime(&filetime);
//#endif
    x.LowPart =  filetime.dwLowDateTime;
    x.HighPart = filetime.dwHighDateTime;
    usec = x.QuadPart / 10  -  epoch_offset_us;
    time_t now = (time_t)(usec / 1000000ULL);
    usec = usec % 1000000ULL;
    ms = (unsigned)(usec / 1000ULL);
    ptm = localtime(&now);
    TM = *ptm;
#else // ! WIN32
    struct timeval ts;
    gettimeofday(&ts, 0);
    ms = ts.tv_usec / 1000;
    ptm = localtime(&ts.tv_sec);
    TM = *ptm;
#endif // WIN32

    if(logger_options_ & LOGGER_OPTION_MILLISECONDS)
    {
        snprintf(buffer, buff_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
            TM.tm_hour, TM.tm_min, TM.tm_sec, ms);
    }
    else
    {
        snprintf(buffer, buff_size, "%04d-%02d-%02d %02d:%02d:%02d",
            TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
            TM.tm_hour, TM.tm_min, TM.tm_sec);
    }
}


void logger_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&mutex);
#else
    pthread_mutex_lock(&mutex);
#endif // _WIN32
}


void logger_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&mutex);
#else
    pthread_mutex_unlock(&mutex);
#endif // _WIN32
}


static const char* logger_stripfile(const char* file)
{
    const char *p = file_name_prefix;

    if(!file) return file;

    while(*p == *file) ++p, ++file;
    return file;
}

// helper function for C++ class name
// class name is returned by typeid(*this).name() and on some
// compilers like GCC/G++ class name have numeric prefix that
// need to be stripped off.
static const char* logger_stralpha(const char* name)
{
    int i = 0;

    if(!name) return name;

    while(name[i])
    {
        if(name[i] >= '0' && name[i] <= '9')
        {
            i++;
            continue;
        }
        return name + i;
    }
    return name;
}


// "%s (%d) [FATAL] " format "\n", time_stamp, getpid()
// "%s (%d) [%s] %s @ %s:%d " format "\n", time_stamp, getpid(), #feature, __func__, __FILE__, __LINE__
// "%s (%d) [ENTERING %s] @ %s:%d " format "\n", time_stamp, getpid(), __func__, __FILE__, __LINE__
// "%s (%d) [ENTERING %s::%s] @ %s:%d " format "\n", time_stamp, getpid(), logger_stralpha_(typeid(*this).name()), __func__, __FILE__, __LINE__
void logger_msg_ex_(char* buff, unsigned len, int nseverity, const char* severity, const char* theclass, const char* func, const char* file, int line, const char* format, ...)
{
    if(logger_options_ & (LOGGER_OPTION_FILE | LOGGER_OPTION_STDERR))
    {
        const char* class_name = logger_stralpha(theclass);
        const char* file_name = logger_stripfile(file);
        make_timestamp(buff, len);
        char *p = buff + strlen(buff);
        len -= p - buff;
        unsigned pid = GETPID();
        if(!class_name) class_name = "";
        if(!file_name) file_name = "";

        // print first part
        if(!func) snprintf(p, len, " (%d) %s", pid, severity);
        else if(!theclass) snprintf(p, len, " (%d) %s %s @ %s:%d", pid, severity, func, file_name, line);
        else snprintf(p, len, " (%d) %s %s::%s @ %s:%d", pid, severity, class_name, func, file_name, line);
        p[len - 1] = 0;

        logger_lock();

        if(logger_options_ & LOGGER_OPTION_FILE)
        {
            if(!fp && log_file) fp = fopen(log_file, "a");
            if(fp)
            {
                va_list args;
                va_start (args, format);
                vfprintf(fp, format, args);
                va_end (args);

                if(logger_options_ & LOGGER_OPTION_FLUSH_FILE) fflush(fp);
                if(logger_options_ & LOGGER_OPTION_KEEP_FILE_OPEN) ;
                else
                {
                    fclose(fp);
                    fp = 0;
                }
            }
        }

        if(logger_options_ & LOGGER_OPTION_STDERR)
        {
            va_list args;
            va_start (args, format);
            vfprintf(stderr, format, args);
            va_end (args);
        }

        logger_unlock();
    }

#if LOGGER_SYSLOG

    if(logger_options_ & LOGGER_OPTION_SYSLOG)
    {
        if(! syslog_open) logger_syslog_open_();
        switch(nseverity)
        {
            case 0: nseverity = LOG_CRIT; break;
            case 1: nseverity = LOG_ERR; break;
            case 2: nseverity = LOG_WARNING; break;
            case 3: nseverity = LOG_INFO; break;
            default: nseverity = LOG_DEBUG;
        }

        int len;
        if(logger_options_ & LOGGER_OPTION_MILLISECONDS) len = 24;
        else len = 20;
        int slen = strlen(buff);
        memmove(buff, buff + len, slen - len);
        buff[slen - len - 1] = 0;

        va_list ptr;
        va_start(ptr, format);
        vsyslog(nseverity, format, ptr);
        va_end(ptr);
    }

#endif // LOGGER_SYSLOG

}




