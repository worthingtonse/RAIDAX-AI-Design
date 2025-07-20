/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : protocol.h
#   Last Modified : 2024-07-20
#   Describe      : Protocol definitions for RAIDAX/RKE
#
# ====================================================*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>

// Connection info structure for protocol handling
typedef struct conn_info_s {
    unsigned char *body;
    uint32_t body_size;
    unsigned char *output;
    uint32_t output_size;
    int command_status;
    unsigned char nonce[16];
} conn_info_t;

// Error codes
#define NO_ERROR                        0
#define STATUS_SUCCESS                  0
#define ERROR_INVALID_PACKET_LENGTH    -1
#define ERROR_INVALID_SN_OR_DENOMINATION -2
#define ERROR_MEMORY_ALLOC             -3
#define ERROR_INVALID_PARAMETER        -4
#define ERROR_FILESYSTEM               -5
#define ERROR_COIN_LOAD               -6
#define ERROR_COINS_NOT_DIV           -7
#define ERROR_NXDOMAIN                -8
#define ERROR_NXRECORD                -9
#define ERROR_INVALID_KEY_START       -10
#define ERROR_KEY_GENERATION          -11
#define ERROR_KEY_SPLITTING           -12

// Function to get body payload from connection info
static inline unsigned char *get_body_payload(conn_info_t *ci) {
    return ci->body;
}

// Function to extract serial number from payload
static inline uint32_t get_sn(unsigned char *payload) {
    return *((uint32_t *)payload);
}

#endif // PROTOCOL_H