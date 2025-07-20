/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : rke.h
#   Last Modified : 2024-07-20
#   Describe      : RKE (RAIDA Key Exchange) header file
#
# ====================================================*/

#ifndef RKE_H
#define RKE_H

#include <stdint.h>
#include <sys/types.h>

// Maximum sizes and limits
#define RKE_MAX_KEY_SIZE 256
#define RKE_MAX_FRAGMENTS 255
#define RKE_MIN_THRESHOLD 2
#define RKE_FRAGMENT_DATA_SIZE 256
#define RKE_CHECKSUM_SIZE 32
#define RKE_KEY_ID_SIZE 16
#define RKE_SESSION_ID_SIZE 16

// RKE key types
#define RKE_KEY_TYPE_SYMMETRIC 0x01
#define RKE_KEY_TYPE_EPHEMERAL 0x02

// RKE session states
#define RKE_SESSION_STATE_INIT     0x00
#define RKE_SESSION_STATE_ACTIVE   0x01
#define RKE_SESSION_STATE_COMPLETE 0x02
#define RKE_SESSION_STATE_EXPIRED  0x03

// RKE error codes
#define RKE_SUCCESS                0
#define RKE_ERROR_INVALID_PARAM   -1
#define RKE_ERROR_MEMORY_ALLOC    -2
#define RKE_ERROR_CRYPTO_FAIL     -3
#define RKE_ERROR_STORAGE_FAIL    -4
#define RKE_ERROR_FRAGMENT_CORRUPT -5
#define RKE_ERROR_INSUFFICIENT_FRAGMENTS -6
#define RKE_ERROR_SESSION_EXPIRED -7

// Forward declaration for conn_info_t (from protocol.h)
typedef struct conn_info_s conn_info_t;

// RKE key fragment structure
struct rke_fragment_t {
    uint8_t fragment_id;         // Fragment identifier (0-255)
    uint8_t total_fragments;     // Total number of fragments
    uint8_t threshold;           // Minimum fragments needed for reconstruction
    uint16_t fragment_size;      // Size of this fragment
    unsigned char data[RKE_FRAGMENT_DATA_SIZE];     // Fragment data
    unsigned char checksum[RKE_CHECKSUM_SIZE];  // SHA-256 checksum for integrity
};

// RKE key metadata
struct rke_key_metadata_t {
    unsigned char key_id[RKE_KEY_ID_SIZE];    // Unique key identifier
    uint8_t key_type;           // Key type (symmetric, etc.)
    uint8_t total_fragments;    // Total fragments created
    uint8_t threshold;          // Minimum fragments for reconstruction
    uint32_t timestamp;         // Creation timestamp
    uint8_t den;                // Denomination of owning coin
    uint32_t sn;                // Serial number of owning coin
};

// RKE session context
struct rke_session_t {
    unsigned char session_id[RKE_SESSION_ID_SIZE]; // Session identifier
    unsigned char sender_id[RKE_KEY_ID_SIZE];  // Sender identity
    unsigned char receiver_id[RKE_KEY_ID_SIZE]; // Receiver identity
    uint8_t state;               // Session state
    uint32_t timeout;            // Session timeout
};

// Core RKE functions
int rke_generate_key(unsigned char *key, uint16_t key_size);
int rke_split_key(const unsigned char *key, uint16_t key_size, struct rke_key_metadata_t *metadata);
int rke_reconstruct_key(unsigned char *key, uint16_t key_size, const struct rke_key_metadata_t *metadata);

// Storage functions
int rke_store_fragment(const struct rke_fragment_t *fragment, const unsigned char *key_id);
int rke_load_fragment(struct rke_fragment_t *fragment, const unsigned char *key_id, uint8_t fragment_id);
int rke_store_metadata(const struct rke_key_metadata_t *metadata);
int rke_load_metadata(struct rke_key_metadata_t *metadata, const unsigned char *key_id);

// Crypto functions
int rke_encrypt_fragment(struct rke_fragment_t *fragment, const unsigned char *key, const unsigned char *nonce);
int rke_decrypt_fragment(struct rke_fragment_t *fragment, const unsigned char *key, const unsigned char *nonce);
int rke_calculate_checksum(struct rke_fragment_t *fragment);
int rke_verify_checksum(const struct rke_fragment_t *fragment);

// Protocol commands
void cmd_rke_generate(conn_info_t *ci);
void cmd_rke_exchange(conn_info_t *ci);
void cmd_rke_reconstruct(conn_info_t *ci);
void cmd_rke_query(conn_info_t *ci);

// Utility functions
int rke_init_session(struct rke_session_t *session, const unsigned char *sender_id, const unsigned char *receiver_id);
int rke_validate_fragment(const struct rke_fragment_t *fragment);
void rke_cleanup_session(struct rke_session_t *session);
int rke_fragment_exists(const unsigned char *key_id, uint8_t fragment_id);
int rke_count_fragments(const unsigned char *key_id);

#endif // RKE_H