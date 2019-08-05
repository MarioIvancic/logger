/*  Copyright (c) 2014, Mario Ivančić
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

#include "loggerux.h"

#ifdef LOGGER_SYSLOG
#include <syslog.h>   // syslog(3), openlog(3), closelog(3)
#include <unistd.h>
#endif // LOGGER_SYSLOG


#ifdef _TIMEVAL_DEFINED
#define USE_MILLISECONDS
#endif


/*
    TODO:
    On Windows,
    LOG_TO_DEBUGGER: log via OutputDebugString().
    LOG_TO_EVENTLOG: log via ReportEvent(). Note: must link to Advapi32.lib.
*/



// _logger_log_level is bitfield acting as a mask.
// FATAL is never masked
// ERROR is on bit 0
// WARN is on bit 1
// INFO is on bit 2
// Other bits (3 .. 31) are for debugging using log_debug_ex() function
unsigned _logger_log_level = 0;
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
int _logger_syslog_facility = LOG_DAEMON;
char* logger_syslog_ident = 0;
static int _logger_syslog_open = 0;



// number of bits for log levels: fatal, error, warn and info
#define LEVEL_BITS  3
// mask for clearing log_level using bitwise AND operation
#define LEVEL_BITS_MASK ( ( 1 << LEVEL_BITS ) - 1 )
// debug mask start value (for feature 0)
#define DEBUG_MASK_START ( 1 << LEVEL_BITS )


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

        if(_logger_options & LOGGER_OPTION_SYSLOG)
        {
            logger_syslog_open();
        }
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
    if(_logger_syslog_open)
    {
        closelog();
        _logger_syslog_open = 0;
    }
}


// Set log level to one of LOGGER_LEVEL_FATAL, LOGGER_LEVEL_ERROR,
// LOGGER_LEVEL_WARNING, LOGGER_LEVEL_INFO.
// _logger_log_level will affect logging using log_fatal, log_error,
// log_warn, log_info and log_debug functions.
void logger_set_level(unsigned level)
{
    switch(level)
    {
        case LOGGER_LEVEL_FATAL:
            _logger_log_level = (_logger_log_level & ~LEVEL_BITS_MASK) | 0;
        break;

        case LOGGER_LEVEL_ERROR:
            _logger_log_level = (_logger_log_level & ~LEVEL_BITS_MASK) | 1;
        break;

        case LOGGER_LEVEL_WARN:
            _logger_log_level = (_logger_log_level & ~LEVEL_BITS_MASK) | 3;
        break;

        case LOGGER_LEVEL_INFO:
            _logger_log_level = (_logger_log_level & ~LEVEL_BITS_MASK) | 7;
        break;
    }
}


// This function is used to set bits 3 .. 31 in _logger_log_level
// from mask bits 0 .. 28. Every bit in mask controls one
// feature that you want to debug.
void logger_set_debug_mask(unsigned mask)
{
    _logger_log_level = (_logger_log_level & LEVEL_BITS_MASK) | (mask << LEVEL_BITS);
}


// This function is used to get bits 3 .. 31 from _logger_log_level
unsigned logger_get_debug_mask(void)
{
    return _logger_log_level >> LEVEL_BITS;
}


// Enable debuging for feature (in range 0 - 28)
void logger_enable_debug(unsigned feature)
{
    if(feature < (32 - LEVEL_BITS)) _logger_log_level |= DEBUG_MASK_START << feature;
}


// Disable debuging for feature (in range 0 - 28)
void logger_disable_debug(unsigned feature)
{
    if(feature < (32 - LEVEL_BITS)) _logger_log_level &= ~(DEBUG_MASK_START << feature);
}


void logger_make_timestamp(char* buffer, unsigned buff_size)
{
    struct tm TM, *ptm;
    unsigned ms = 0;
#ifdef USE_MILLISECONDS
    struct timeval ts;
    gettimeofday(&ts, 0);
    ms = ts.tv_usec / 1000;
    ptm = localtime(&ts.tv_sec);
    TM = *ptm;
#else // ! USE_MILLISECONDS
    time_t now;
    time(&now);
    ptm = localtime(&now);
    TM = *ptm;
#endif // USE_MILLISECONDS
    snprintf(buffer, buff_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
        TM.tm_hour, TM.tm_min, TM.tm_sec, ms);
}





void logger_msg_ex(const char* format, ...)
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





#ifdef LOGGER_SYSLOG
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

void logger_syslog_open(void)
{
    if(! _logger_syslog_open)
    {
        openlog(logger_syslog_ident, LOG_PID|LOG_NDELAY|LOG_CONS, _logger_syslog_facility);
        _logger_syslog_open = 1;
    }
}

void logger_syslog_fatal(const char *format, ...)
{
    if(! _logger_syslog_open) logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_CRIT, format, ptr);
    va_end(ptr);
}


void logger_syslog_err(const char *format, ...)
{
    if(! _logger_syslog_open) logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_ERR, format, ptr);
    va_end(ptr);
}

void logger_syslog_warn(const char *format, ...)
{
    if(! _logger_syslog_open) logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_WARNING, format, ptr);
    va_end(ptr);
}

void logger_syslog_info(const char *format, ...)
{
    if(! _logger_syslog_open) logger_syslog_open();
    va_list ptr;
    va_start(ptr, format);
    vsyslog(LOG_INFO, format, ptr);
    va_end(ptr);
}

#else // ! LOGGER_SYSLOG
// dummy implementations
void logger_syslog_open(void){ }
void logger_syslog_fatal(const char *format, ...) { (void) format; }
void logger_syslog_err(const char *format, ...) { (void) format; }
void logger_syslog_warn(const char *format, ...) { (void) format; }
void logger_syslog_info(const char *format, ...) { (void) format; }
#endif  // LOGGER_SYSLOG


// old logger code
#if 0

static void log_msg(const char* level, const char* format, va_list args)
{
    if(!fp && log_file) fp = fopen(log_file, "a");
    if(fp)
    {
        char tsb[64];   // timestamp buffer
        struct tm TM, *ptm;
        unsigned ms = 0;
#ifdef USE_MILLISECONDS
        struct timeval ts;
        gettimeofday(&ts, 0);
        ms = ts.tv_usec / 1000;
        ptm = localtime(&ts.tv_sec);
        TM = *ptm;
#else // ! USE_MILLISECONDS
        time_t now;
        time(&now);
        ptm = localtime(&now);
        TM = *ptm;
#endif // USE_MILLISECONDS

        sprintf(tsb, "\n%04d-%02d-%02d %02d:%02d:%02d.%03d [%s] ",
            TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
            TM.tm_hour, TM.tm_min, TM.tm_sec, ms, level);

        vfprintf(fp, format, args);

        if(log_options & LOGGER_OPTION_FLUSH_FILE) fflush(fp);
        if(log_options & LOGGER_OPTION_KEEP_FILE_OPEN) ;
        else
        {
            fclose(fp);
            fp = 0;
        }
    }
}



void log_fatal_exit(const char* format, ...)
{
    va_list args;
    va_start (args, format);
    log_msg("FATAL", format, args);
    va_end (args);
    logger_close();
    exit(1);
}

void log_fatal(const char* format, ...)
{
    va_list args;
    va_start (args, format);
    log_msg("FATAL", format, args);
    va_end (args);
}


void log_error(const char* format, ...)
{
    if(_logger_log_level & (1 << 0))
    {
        va_list args;
        va_start (args, format);
        log_msg("ERROR", format, args);
        va_end (args);
    }
}

void log_warn(const char* format, ...)
{
    if(_logger_log_level & (1 << 1))
    {
        va_list args;
        va_start (args, format);
        log_msg("WARN ", format, args);
        va_end (args);
    }
}


void log_info(const char* format, ...)
{
    if(_logger_log_level & (1 << 2))
    {
        va_list args;
        va_start (args, format);
        log_msg("INFO ", format, args);
        va_end (args);
    }
}


// feature is int in range 0 - 28
void log_debug_ex(unsigned feature, const char* format, ...)
{
    if((feature < 29) && (_logger_log_level & (8 << feature)))
    {
        va_list args;
        va_start (args, format);
        log_msg("DEBUG", format, args);
        va_end (args);
    }
}

#endif // if 0



