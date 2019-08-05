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

// simple logging for C

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#include "logger.h"

static unsigned int log_level;
static const char* log_file;

void logger_set_filename(const char* log_file_name)
{
    log_file = log_file_name;
}

void logger_set_level(unsigned int level)
{
    log_level = level;
}


// we are using three calls to fprintf/vfprintf so this is NOT thread-safe
static void log_msg(const char* level, const char* format, va_list args)
{
    FILE* fp;
    fp = fopen(log_file, "a");
    if(fp)
    {
        struct tm TM, *ptm;
        time_t now;
        time(&now);
        ptm = localtime(&now);
        TM = *ptm;
        fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d [%s] ",
            TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
            TM.tm_hour, TM.tm_min, TM.tm_sec, level);
        vfprintf(fp, format, args);
        fprintf(fp, "\n");
        fclose(fp);
    }
}


void log_fatal(const char* format, ...)
{
    if(log_file != 0)
    {
        va_list args;
        va_start (args, format);
        log_msg("FATAL", format, args);
        va_end (args);
    }
    exit(1);
}

void log_error(const char* format, ...)
{
    if(log_level >= 1 && log_file != 0)
    {
        va_list args;
        va_start (args, format);
        log_msg("ERROR", format, args);
        va_end (args);
    }
}

void log_warn(const char* format, ...)
{
    if(log_level >= 2 && log_file != 0)
    {
        va_list args;
        va_start (args, format);
        log_msg("WARN ", format, args);
        va_end (args);
    }
}


void log_info(const char* format, ...)
{
    if(log_level >= 3 && log_file != 0)
    {
        va_list args;
        va_start (args, format);
        log_msg("INFO ", format, args);
        va_end (args);
    }
}


void log_debug(const char* format, ...)
{
    if(log_level >= 4 && log_file != 0)
    {
        va_list args;
        va_start (args, format);
        log_msg("DEBUG", format, args);
        va_end (args);
    }
}
