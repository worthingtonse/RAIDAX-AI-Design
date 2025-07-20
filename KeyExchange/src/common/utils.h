/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : utils.h
#   Last Modified : 2024-07-20
#   Describe      : Utility functions for RKE implementation
#
# ====================================================*/

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

// Configuration structure
struct config_s {
    char cwd[1024];        // Current working directory
    uint16_t coin_id;      // Coin identifier
};

extern struct config_s config;

// Directory creation utility
static inline int create_directory_recursive(const char *path) {
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

// Simple implementation of secure random (should use proper crypto in production)
static inline int secure_random_bytes(unsigned char *buffer, size_t length) {
    FILE *fp = fopen("/dev/urandom", "rb");
    if (fp == NULL) {
        return -1;
    }
    
    size_t read_bytes = fread(buffer, 1, length, fp);
    fclose(fp);
    
    return (read_bytes == length) ? 0 : -1;
}

// Global config instance (define in one source file only)

#endif // UTILS_H