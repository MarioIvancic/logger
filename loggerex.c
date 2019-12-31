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

#include "loggerex.h"

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



// _logger_log_level constants
// FATAL is 0 (never masked)
// ERROR is 1
// WARN is 2
// INFO is 3
// DEBUG is 4
// TRACE is 5
unsigned _logger_log_level = 0;

// debug level bits (0 .. 31) are for debugging using log_debug_ex() function
unsigned _logger_debug_mask = 0;

unsigned _logger_options = LOGGER_OPTION_FILE;
static const char* log_file = 0;
static FILE* fp = 0;

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
#endif // LOGGER_SYSLOG




// Set log file name and options. Caller must provide storage for string
// log_file_name. If LOGGER_OPTION_KEEP_FILE_OPEN option is specified we will open
// named log file and save file handle for later use.
void logger_open(const char* log_file_name, unsigned options)
{
    log_file = log_file_name;
    _logger_options = options;

    if(_logger_options & LOGGER_OPTION_KEEP_FILE_OPEN)
    {
        if(_logger_options & LOGGER_OPTION_FILE)
        {
            if(fp) fclose(fp);
            fp = fopen(log_file, "a");
        }
#if LOGGER_SYSLOG
        if(_logger_options & LOGGER_OPTION_SYSLOG)
        {
            _logger_syslog_open();
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
#if LOGGER_SYSLOG
    if(syslog_open)
    {
        closelog();
        syslog_open = 0;
    }
#endif // LOGGER_SYSLOG
}


// Set log level to one of LOGGER_LEVEL_FATAL, LOGGER_LEVEL_ERROR,
// LOGGER_LEVEL_WARNING, LOGGER_LEVEL_INFO.
// _logger_log_level will affect logging using log_fatal, log_error,
// log_warn, log_info and log_debug functions.
void logger_set_log_level(unsigned level)
{
    _logger_log_level = level;
}


// This function is used to set bits in _logger_debug_mask
// Every bit in mask controls one feature that you want to debug.
void logger_set_debug_mask(unsigned mask)
{
    _logger_debug_mask = mask;
}


// This function is used to get _logger_debug_mask
unsigned logger_get_debug_mask(void)
{
    return _logger_debug_mask;
}


// Enable debuging for feature (bitmask)
void logger_enable_debug(unsigned feature)
{
    _logger_debug_mask |= feature;
}


// Disable debuging for feature (bitmask)
void logger_disable_debug(unsigned feature)
{
    _logger_debug_mask &= ~feature;
}

#ifdef WIN32
#include <Windows.h>
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if (tv) {
        FILETIME               filetime; /* 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 00:00 UTC */
        ULARGE_INTEGER         x;
        ULONGLONG              usec;
        static const ULONGLONG epoch_offset_us = 11644473600000000ULL; /* microseconds betweeen Jan 1,1601 and Jan 1,1970 */

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
        GetSystemTimePreciseAsFileTime(&filetime);
#else
        GetSystemTimeAsFileTime(&filetime);
#endif
        x.LowPart =  filetime.dwLowDateTime;
        x.HighPart = filetime.dwHighDateTime;
        usec = x.QuadPart / 10  -  epoch_offset_us;
        tv->tv_sec  = (time_t)(usec / 1000000ULL);
        tv->tv_usec = (long)(usec % 1000000ULL);
    }
    if (tz) {
        TIME_ZONE_INFORMATION timezone;
        GetTimeZoneInformation(&timezone);
        tz->tz_minuteswest = timezone.Bias;
        tz->tz_dsttime = 0;
    }
    return 0;
}
#endif


void _logger_make_timestamp(char* buffer, unsigned buff_size)
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

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime(&filetime);
#else
    GetSystemTimeAsFileTime(&filetime);
#endif
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

    if(_logger_options & LOGGER_OPTION_MILLISECONDS)
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




// "%s (%d) [FATAL] " format "\n", time_stamp, getpid()
void _logger_msg_ex(const char* format, ...)
{
    if(_logger_options & LOGGER_OPTION_FILE)
    {
        if(!fp && log_file) fp = fopen(log_file, "a");
        if(fp)
        {
            va_list args;
            va_start (args, format);
            vfprintf(fp, format, args);
            va_end (args);

            if(_logger_options & LOGGER_OPTION_FLUSH_FILE) fflush(fp);
            if(_logger_options & LOGGER_OPTION_KEEP_FILE_OPEN) ;
            else
            {
                fclose(fp);
                fp = 0;
            }
        }
    }
    if(_logger_options & LOGGER_OPTION_STDERR)
    {
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
    }
}





#if LOGGER_SYSLOG
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

void _logger_syslog_open(void)
{
    if(! syslog_open)
    {
        openlog(syslog_ident, LOG_PID|LOG_NDELAY|LOG_CONS, syslog_facility);
        syslog_open = 1;
    }
}

// "[FATAL] " format "\n"
void _logger_syslog_fatal(const char *format, ...)
{
    if(! syslog_open) _logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_CRIT, format, ptr);
    va_end(ptr);
}


// "[FATAL] " format "\n"
void _logger_syslog_err(const char *format, ...)
{
    if(! syslog_open) _logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_ERR, format, ptr);
    va_end(ptr);
}

// "[FATAL] " format "\n"
void _logger_syslog_warn(const char *format, ...)
{
    if(! syslog_open) _logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_WARNING, format, ptr);
    va_end(ptr);
}

// "[FATAL] " format "\n"
void _logger_syslog_info(const char *format, ...)
{
    if(! syslog_open) _logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_INFO, format, ptr);
    va_end(ptr);
}
#endif  // LOGGER_SYSLOG




