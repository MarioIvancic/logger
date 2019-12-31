// main.c
// testing loggerex

#include <stdio.h>
#include "../loggerex.h"
#include "../debug_features.h"


void test_function(void)
{
    log_trace_enter("args: void");

    printf("Inside test function\n");

    log_trace_exit("result: void");
}

int main(int argc, char ** argv)
{
    const char * log_file = "loggerex.log";
    unsigned logger_options = 0
    | LOGGER_OPTION_STDERR
    | LOGGER_OPTION_SYSLOG
    | LOGGER_OPTION_FILE
    | LOGGER_OPTION_FLUSH_FILE
    | LOGGER_OPTION_KEEP_FILE_OPEN
    | LOGGER_OPTION_MILLISECONDS;

    logger_open(log_file, logger_options);
    logger_set_log_level(LOGGER_LEVEL_TRACE);
    logger_set_debug_mask(-1);

    log_fatal("Fatal error: %d", 123);
    log_error("Error: %d", 234);
    log_warn("Warning: %d", 345);
    log_info("Status: %d", 456);

    test_function();

    log_debug(CSVDEBUG, "Some value: %d", 567);
    log_debug(VARDEBUG, "Some value: %d", 678);


    logger_close();

    return 0;
}
