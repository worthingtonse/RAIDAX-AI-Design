/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : log.h
#   Last Modified : 2024-07-20
#   Describe      : Logging functions for RKE implementation
#
# ====================================================*/

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

// Log levels
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

// Current log level (can be configured)
extern int current_log_level;

// Convenience macros for logging
#define debug(fmt, ...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define info(fmt, ...)  log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  log_message(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define error(fmt, ...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// Simple inline implementation for basic logging
static inline void log_message(int level, const char *file, int line, const char *format, ...) {
    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    
    if (level < current_log_level) return;
    
    time_t now;
    time(&now);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = 0; // Remove newline
    
    printf("[%s] %s:%d [%s] ", time_str, file, line, level_str[level]);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

// Global log level variable (define in one source file only)

#endif // LOG_H