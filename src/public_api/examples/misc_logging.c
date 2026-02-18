/*
 * This example demonstrates how to configure log level and handler.
 *
 * Building:
 *   cc -o misc_logging misc_logging.c -lroc
 *
 * Running:
 *   ./misc_logging
 *
 * License:
 *   public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/log.h>

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

static void my_log_handler(const roc_log_message* message, void* argument) {
    const char* lvl = NULL;

    switch (message->level) {
    case ROC_LOG_ERROR:
        lvl = "ERROR";
        break;

    case ROC_LOG_INFO:
        lvl = "INFO";
        break;

    case ROC_LOG_NOTE:
        lvl = "NOTE";
        break;

    case ROC_LOG_DEBUG:
        lvl = "DEBUG";
        break;

    case ROC_LOG_TRACE:
        lvl = "TRACE";
        break;

    default:
        lvl = "UNKNOWN";
        break;
    }

    printf("level=%s module=%s time=%lld pid=%llu tid=%llu text=%s\n", lvl,
           message->module, message->time, message->pid, message->tid, message->text);
}

int main() {
    /* Allow all log message starting from DEBUG level and higher. */
    roc_log_set_level(ROC_LOG_DEBUG);

    /* Set custom handler for log messages. */
    roc_log_set_handler(my_log_handler, NULL);

    /* Do something to trigger some logging. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));
    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
