/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : rke_core.c
#   Last Modified : 2024-07-20
#   Describe      : RKE core functionality - key generation, splitting, reconstruction
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

// Global storage for key fragments (in production, use proper storage)
static struct rke_fragment_t fragment_storage[256];
static int fragment_count = 0;

/*
 * Generate a quantum-safe cryptographic key
 */
int rke_generate_key(unsigned char *key, uint16_t key_size) {
    // Validate input parameters
    if (key == NULL || key_size == 0 || key_size > RKE_MAX_KEY_SIZE) {
        error("Invalid key generation parameters: key=%p, size=%d", key, key_size);
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Use cryptographically secure random number generation
    if (secure_random_bytes(key, key_size) != 0) {
        error("Failed to generate secure random key of size %d", key_size);
        return RKE_ERROR_CRYPTO_FAIL;
    }
    
    debug("Generated %d-byte quantum-safe key", key_size);
    return RKE_SUCCESS;
}

/*
 * Simple polynomial evaluation for Shamir's secret sharing
 * This is a simplified implementation - production should use proper GF(256) arithmetic
 */
static uint8_t evaluate_polynomial(uint8_t x, uint8_t *coefficients, uint8_t degree) {
    uint8_t result = 0;
    uint8_t x_power = 1;
    
    for (int i = 0; i <= degree; i++) {
        result ^= (coefficients[i] * x_power) & 0xFF;
        x_power = (x_power * x) & 0xFF;
    }
    
    return result;
}

/*
 * Split a key into fragments using simplified Shamir's secret sharing
 */
int rke_split_key(const unsigned char *key, uint16_t key_size, struct rke_key_metadata_t *metadata) {
    // Validate input parameters
    if (key == NULL || metadata == NULL || key_size == 0 || key_size > RKE_MAX_KEY_SIZE) {
        error("Invalid key splitting parameters");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    if (metadata->threshold > metadata->total_fragments) {
        error("Threshold %d cannot exceed total fragments %d", 
              metadata->threshold, metadata->total_fragments);
        return RKE_ERROR_INVALID_PARAM;
    }
    
    if (metadata->threshold < RKE_MIN_THRESHOLD) {
        error("Threshold %d too low, minimum is %d", metadata->threshold, RKE_MIN_THRESHOLD);
        return RKE_ERROR_INVALID_PARAM;
    }
    
    debug("Splitting %d-byte key into %d fragments (threshold=%d)", 
          key_size, metadata->total_fragments, metadata->threshold);
    
    // Clear existing fragments
    fragment_count = 0;
    memset(fragment_storage, 0, sizeof(fragment_storage));
    
    // Simplified secret sharing for demonstration
    // Fragment 0 contains the original key, other fragments contain XOR masks
    for (int frag_id = 1; frag_id <= metadata->total_fragments; frag_id++) {
        struct rke_fragment_t *fragment = &fragment_storage[frag_id - 1];
        
        // Initialize fragment metadata
        fragment->fragment_id = frag_id;
        fragment->total_fragments = metadata->total_fragments;
        fragment->threshold = metadata->threshold;
        fragment->fragment_size = key_size;
        memset(fragment->data, 0, RKE_FRAGMENT_DATA_SIZE);
        
        if (frag_id == 1) {
            // First fragment gets the original key
            memcpy(fragment->data, key, key_size);
        } else {
            // Other fragments get random XOR masks
            if (secure_random_bytes(fragment->data, key_size) != 0) {
                error("Failed to generate random mask for fragment");
                return RKE_ERROR_CRYPTO_FAIL;
            }
            
            // XOR the first fragment with this mask
            for (int i = 0; i < key_size; i++) {
                fragment_storage[0].data[i] ^= fragment->data[i];
            }
        }
    }
    
    fragment_count = metadata->total_fragments;
    
    // Calculate checksums for all fragments
    for (int i = 0; i < metadata->total_fragments; i++) {
        if (rke_calculate_checksum(&fragment_storage[i]) != RKE_SUCCESS) {
            error("Failed to calculate checksum for fragment %d", i);
            return RKE_ERROR_CRYPTO_FAIL;
        }
    }
    
    debug("Successfully split key into %d fragments", metadata->total_fragments);
    return RKE_SUCCESS;
}

/*
 * Lagrange interpolation for secret reconstruction
 * Simplified implementation for demonstration
 */
static uint8_t lagrange_interpolate(uint8_t *x_coords, uint8_t *y_coords, int count) {
    uint16_t result = 0;
    
    for (int i = 0; i < count; i++) {
        uint16_t numerator = 1;
        uint16_t denominator = 1;
        
        for (int j = 0; j < count; j++) {
            if (i != j) {
                numerator = (numerator * (256 - x_coords[j])) & 0xFFFF;
                denominator = (denominator * (x_coords[i] ^ x_coords[j])) & 0xFFFF;
            }
        }
        
        // Simplified division (proper implementation needs modular inverse)
        if (denominator != 0) {
            uint16_t term = (y_coords[i] * numerator / denominator) & 0xFF;
            result ^= term;
        }
    }
    
    return result & 0xFF;
}

/*
 * Reconstruct a key from sufficient fragments
 */
int rke_reconstruct_key(unsigned char *key, uint16_t key_size, const struct rke_key_metadata_t *metadata) {
    // Validate input parameters
    if (key == NULL || metadata == NULL || key_size == 0) {
        error("Invalid key reconstruction parameters");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    if (fragment_count < metadata->threshold) {
        error("Insufficient fragments: have %d, need %d", fragment_count, metadata->threshold);
        return RKE_ERROR_INSUFFICIENT_FRAGMENTS;
    }
    
    debug("Reconstructing %d-byte key from %d fragments (threshold=%d)", 
          key_size, fragment_count, metadata->threshold);
    
    // Verify fragment integrity
    for (int i = 0; i < metadata->threshold; i++) {
        if (rke_verify_checksum(&fragment_storage[i]) != RKE_SUCCESS) {
            error("Fragment %d failed integrity check", i);
            return RKE_ERROR_FRAGMENT_CORRUPT;
        }
    }
    
    // Simplified reconstruction - XOR all fragments together
    memcpy(key, fragment_storage[0].data, key_size);  // Start with first fragment
    
    // XOR with all other fragments to undo the masks
    for (int frag_idx = 1; frag_idx < fragment_count; frag_idx++) {
        for (int byte_idx = 0; byte_idx < key_size; byte_idx++) {
            key[byte_idx] ^= fragment_storage[frag_idx].data[byte_idx];
        }
    }
    
    debug("Successfully reconstructed %d-byte key", key_size);
    return RKE_SUCCESS;
}

/*
 * Validate a fragment structure
 */
int rke_validate_fragment(const struct rke_fragment_t *fragment) {
    if (fragment == NULL) {
        return RKE_ERROR_INVALID_PARAM;
    }
    
    if (fragment->fragment_id == 0 || fragment->fragment_id > fragment->total_fragments) {
        error("Invalid fragment ID: %d (total: %d)", fragment->fragment_id, fragment->total_fragments);
        return RKE_ERROR_INVALID_PARAM;
    }
    
    if (fragment->threshold > fragment->total_fragments) {
        error("Invalid threshold: %d > %d", fragment->threshold, fragment->total_fragments);
        return RKE_ERROR_INVALID_PARAM;
    }
    
    if (fragment->fragment_size > RKE_FRAGMENT_DATA_SIZE) {
        error("Fragment size too large: %d > %d", fragment->fragment_size, RKE_FRAGMENT_DATA_SIZE);
        return RKE_ERROR_INVALID_PARAM;
    }
    
    return RKE_SUCCESS;
}