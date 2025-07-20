/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : rke_protocol.c
#   Last Modified : 2024-07-20
#   Describe      : RKE protocol command implementations
#
# ====================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "../common/protocol.h"
#include "../common/log.h"
#include "../common/utils.h"
#include "rke.h"

/*
 * RKE Generate Command
 * Generates a new key and splits it into fragments
 * Packet format: 16-byte key_id + 1-byte key_type + 1-byte total_fragments + 1-byte threshold + 2-byte EOF = 21 bytes
 */
void cmd_rke_generate(conn_info_t *ci) {
    unsigned char *payload = get_body_payload(ci);
    struct rke_key_metadata_t metadata;
    unsigned char key[RKE_MAX_KEY_SIZE];
    
    debug("CMD RKE Generate");
    
    // Validate packet size first
    if (ci->body_size != 21) {
        error("Invalid command length: %d. Need 21", ci->body_size);
        ci->command_status = ERROR_INVALID_PACKET_LENGTH;
        return;
    }
    
    // Extract parameters from payload
    memcpy(metadata.key_id, payload, RKE_KEY_ID_SIZE);
    metadata.key_type = payload[16];
    metadata.total_fragments = payload[17];
    metadata.threshold = payload[18];
    
    // Set additional metadata
    metadata.timestamp = (uint32_t)time(NULL);
    metadata.den = 0; // TODO: Extract from coin context if needed
    metadata.sn = 0;  // TODO: Extract from coin context if needed
    
    // Validate business logic
    if (metadata.threshold > metadata.total_fragments) {
        error("Threshold %d cannot exceed total fragments %d", 
              metadata.threshold, metadata.total_fragments);
        ci->command_status = ERROR_INVALID_PARAMETER;
        return;
    }
    
    if (metadata.total_fragments == 0) {
        error("Total fragments cannot be zero");
        ci->command_status = ERROR_INVALID_PARAMETER;
        return;
    }
    
    if (metadata.threshold < RKE_MIN_THRESHOLD) {
        error("Threshold %d below minimum %d", metadata.threshold, RKE_MIN_THRESHOLD);
        ci->command_status = ERROR_INVALID_PARAMETER;
        return;
    }
    
    // Generate key
    if (rke_generate_key(key, 256) != RKE_SUCCESS) {
        error("Failed to generate key");
        ci->command_status = ERROR_KEY_GENERATION;
        return;
    }
    
    // Split key into fragments
    if (rke_split_key(key, 256, &metadata) != RKE_SUCCESS) {
        error("Failed to split key");
        ci->command_status = ERROR_KEY_SPLITTING;
        return;
    }
    
    // Store metadata
    if (rke_store_metadata(&metadata) != RKE_SUCCESS) {
        error("Failed to store metadata");
        ci->command_status = ERROR_FILESYSTEM;
        return;
    }
    
    // Prepare response
    ci->output_size = 1;  // Success indicator
    ci->output = (unsigned char *) malloc(1);
    if (ci->output == NULL) {
        error("Can't alloc buffer for the response");
        ci->command_status = ERROR_MEMORY_ALLOC;
        return;
    }
    
    ci->output[0] = 0x01;  // Success
    ci->command_status = STATUS_SUCCESS;
    
    debug("CMD RKE Generate finished - generated key with %d fragments", metadata.total_fragments);
}

/*
 * RKE Exchange Command
 * Exchanges key fragments with a peer
 * Packet format: 16-byte key_id + 1-byte fragment_id + fragment_data + 2-byte EOF
 */
void cmd_rke_exchange(conn_info_t *ci) {
    unsigned char *payload = get_body_payload(ci);
    unsigned char key_id[RKE_KEY_ID_SIZE];
    uint8_t fragment_id;
    struct rke_fragment_t fragment;
    
    debug("CMD RKE Exchange");
    
    // Minimum packet size: 16-byte key_id + 1-byte fragment_id + 2-byte EOF = 19 bytes
    if (ci->body_size < 19) {
        error("Invalid command length: %d. Need at least 19", ci->body_size);
        ci->command_status = ERROR_INVALID_PACKET_LENGTH;
        return;
    }
    
    // Extract key_id and fragment_id
    memcpy(key_id, payload, RKE_KEY_ID_SIZE);
    fragment_id = payload[RKE_KEY_ID_SIZE];
    
    debug("Exchanging fragment %d for key %02x%02x%02x%02x...", 
          fragment_id, key_id[0], key_id[1], key_id[2], key_id[3]);
    
    // Check if we have this fragment
    if (rke_fragment_exists(key_id, fragment_id)) {
        // Load and return the requested fragment
        if (rke_load_fragment(&fragment, key_id, fragment_id) != RKE_SUCCESS) {
            error("Failed to load fragment %d", fragment_id);
            ci->command_status = ERROR_FILESYSTEM;
            return;
        }
        
        // Prepare response with fragment data
        ci->output_size = sizeof(struct rke_fragment_t);
        ci->output = (unsigned char *) malloc(ci->output_size);
        if (ci->output == NULL) {
            error("Can't alloc buffer for the response");
            ci->command_status = ERROR_MEMORY_ALLOC;
            return;
        }
        
        memcpy(ci->output, &fragment, sizeof(struct rke_fragment_t));
        ci->command_status = STATUS_SUCCESS;
        
        debug("Returned fragment %d (%zu bytes)", fragment_id, sizeof(struct rke_fragment_t));
    } else {
        // Fragment not found
        error("Fragment %d not found for key", fragment_id);
        ci->command_status = ERROR_INVALID_PARAMETER;
        return;
    }
    
    debug("CMD RKE Exchange finished");
}

/*
 * RKE Reconstruct Command
 * Reconstructs a key from available fragments
 * Packet format: 16-byte key_id + 2-byte EOF = 18 bytes
 */
void cmd_rke_reconstruct(conn_info_t *ci) {
    unsigned char *payload = get_body_payload(ci);
    unsigned char key_id[RKE_KEY_ID_SIZE];
    struct rke_key_metadata_t metadata;
    unsigned char reconstructed_key[RKE_MAX_KEY_SIZE];
    
    debug("CMD RKE Reconstruct");
    
    // Validate packet size
    if (ci->body_size != 18) {
        error("Invalid command length: %d. Need 18", ci->body_size);
        ci->command_status = ERROR_INVALID_PACKET_LENGTH;
        return;
    }
    
    // Extract key_id
    memcpy(key_id, payload, RKE_KEY_ID_SIZE);
    
    debug("Reconstructing key %02x%02x%02x%02x...", 
          key_id[0], key_id[1], key_id[2], key_id[3]);
    
    // Load metadata
    if (rke_load_metadata(&metadata, key_id) != RKE_SUCCESS) {
        error("Failed to load metadata for key");
        ci->command_status = ERROR_FILESYSTEM;
        return;
    }
    
    // Check if we have sufficient fragments
    int available_fragments = rke_count_fragments(key_id);
    if (available_fragments < metadata.threshold) {
        error("Insufficient fragments: have %d, need %d", available_fragments, metadata.threshold);
        ci->command_status = ERROR_INVALID_PARAMETER;
        return;
    }
    
    // Load required fragments
    for (int i = 1; i <= metadata.threshold; i++) {
        if (rke_fragment_exists(key_id, i)) {
            struct rke_fragment_t fragment;
            if (rke_load_fragment(&fragment, key_id, i) != RKE_SUCCESS) {
                error("Failed to load fragment %d", i);
                ci->command_status = ERROR_FILESYSTEM;
                return;
            }
        }
    }
    
    // Reconstruct the key
    if (rke_reconstruct_key(reconstructed_key, 256, &metadata) != RKE_SUCCESS) {
        error("Failed to reconstruct key");
        ci->command_status = ERROR_KEY_GENERATION;
        return;
    }
    
    // Prepare response with reconstructed key
    ci->output_size = 256;  // Key size
    ci->output = (unsigned char *) malloc(ci->output_size);
    if (ci->output == NULL) {
        error("Can't alloc buffer for the response");
        ci->command_status = ERROR_MEMORY_ALLOC;
        return;
    }
    
    memcpy(ci->output, reconstructed_key, 256);
    ci->command_status = STATUS_SUCCESS;
    
    debug("CMD RKE Reconstruct finished - reconstructed 256-byte key");
}

/*
 * RKE Query Command
 * Queries available fragments for a key
 * Packet format: 16-byte key_id + 2-byte EOF = 18 bytes
 */
void cmd_rke_query(conn_info_t *ci) {
    unsigned char *payload = get_body_payload(ci);
    unsigned char key_id[RKE_KEY_ID_SIZE];
    struct rke_key_metadata_t metadata;
    unsigned char fragment_map[32]; // Bitmap for up to 256 fragments
    
    debug("CMD RKE Query");
    
    // Validate packet size
    if (ci->body_size != 18) {
        error("Invalid command length: %d. Need 18", ci->body_size);
        ci->command_status = ERROR_INVALID_PACKET_LENGTH;
        return;
    }
    
    // Extract key_id
    memcpy(key_id, payload, RKE_KEY_ID_SIZE);
    
    debug("Querying fragments for key %02x%02x%02x%02x...", 
          key_id[0], key_id[1], key_id[2], key_id[3]);
    
    // Load metadata
    if (rke_load_metadata(&metadata, key_id) != RKE_SUCCESS) {
        error("Failed to load metadata for key");
        ci->command_status = ERROR_FILESYSTEM;
        return;
    }
    
    // Initialize fragment bitmap
    memset(fragment_map, 0, sizeof(fragment_map));
    
    // Check which fragments are available
    int available_count = 0;
    for (int i = 1; i <= metadata.total_fragments; i++) {
        if (rke_fragment_exists(key_id, i)) {
            // Set bit in bitmap
            int byte_index = (i - 1) / 8;
            int bit_index = (i - 1) % 8;
            fragment_map[byte_index] |= (1 << bit_index);
            available_count++;
        }
    }
    
    // Prepare response: metadata + fragment bitmap
    ci->output_size = sizeof(struct rke_key_metadata_t) + sizeof(fragment_map);
    ci->output = (unsigned char *) malloc(ci->output_size);
    if (ci->output == NULL) {
        error("Can't alloc buffer for the response");
        ci->command_status = ERROR_MEMORY_ALLOC;
        return;
    }
    
    memcpy(ci->output, &metadata, sizeof(struct rke_key_metadata_t));
    memcpy(ci->output + sizeof(struct rke_key_metadata_t), fragment_map, sizeof(fragment_map));
    
    ci->command_status = STATUS_SUCCESS;
    
    debug("CMD RKE Query finished - %d/%d fragments available", 
          available_count, metadata.total_fragments);
}

/*
 * Initialize an RKE session
 */
int rke_init_session(struct rke_session_t *session, const unsigned char *sender_id, const unsigned char *receiver_id) {
    if (session == NULL || sender_id == NULL || receiver_id == NULL) {
        error("Invalid parameters for session initialization");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Generate random session ID
    if (secure_random_bytes(session->session_id, RKE_SESSION_ID_SIZE) != 0) {
        error("Failed to generate session ID");
        return RKE_ERROR_CRYPTO_FAIL;
    }
    
    memcpy(session->sender_id, sender_id, RKE_KEY_ID_SIZE);
    memcpy(session->receiver_id, receiver_id, RKE_KEY_ID_SIZE);
    session->state = RKE_SESSION_STATE_INIT;
    session->timeout = (uint32_t)time(NULL) + 3600; // 1 hour timeout
    
    debug("Initialized RKE session %02x%02x%02x%02x...", 
          session->session_id[0], session->session_id[1], 
          session->session_id[2], session->session_id[3]);
    
    return RKE_SUCCESS;
}

/*
 * Cleanup an RKE session
 */
void rke_cleanup_session(struct rke_session_t *session) {
    if (session == NULL) {
        return;
    }
    
    debug("Cleaning up RKE session %02x%02x%02x%02x...", 
          session->session_id[0], session->session_id[1], 
          session->session_id[2], session->session_id[3]);
    
    // Clear sensitive data
    memset(session, 0, sizeof(struct rke_session_t));
}