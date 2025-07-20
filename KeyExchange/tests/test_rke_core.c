/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : test_rke_core.c
#   Last Modified : 2024-07-20
#   Describe      : Unit tests for RKE core functionality
#
# ====================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../src/rke/rke.h"
#include "../src/common/log.h"
#include "../src/common/utils.h"

// Define global variables
int current_log_level = LOG_LEVEL_DEBUG;
struct config_s config = {
    .cwd = "/tmp/rke_test",
    .coin_id = 1
};

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

// Test macros
#define ASSERT(condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("✓ Test %d passed\n", tests_run); \
    } else { \
        printf("✗ Test %d failed: %s\n", tests_run, #condition); \
        return -1; \
    } \
} while(0)

#define TEST_FUNCTION(name) \
    printf("\n=== Testing %s ===\n", #name); \
    if (name() != 0) { \
        printf("Test function %s failed!\n", #name); \
        return -1; \
    }

/*
 * Test key generation functionality
 */
int test_key_generation() {
    unsigned char key[256];
    unsigned char key2[256];
    
    printf("Testing key generation...\n");
    
    // Test normal key generation
    int result = rke_generate_key(key, 256);
    ASSERT(result == RKE_SUCCESS);
    
    // Test that generated keys are different
    result = rke_generate_key(key2, 256);
    ASSERT(result == RKE_SUCCESS);
    ASSERT(memcmp(key, key2, 256) != 0);
    
    // Test invalid parameters
    result = rke_generate_key(NULL, 256);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    result = rke_generate_key(key, 0);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    result = rke_generate_key(key, RKE_MAX_KEY_SIZE + 1);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    printf("Key generation tests passed!\n");
    return 0;
}

/*
 * Test key splitting and reconstruction
 */
int test_key_splitting() {
    unsigned char original_key[256];
    unsigned char reconstructed_key[256];
    struct rke_key_metadata_t metadata;
    
    printf("Testing key splitting and reconstruction...\n");
    
    // Generate test key
    int result = rke_generate_key(original_key, 256);
    ASSERT(result == RKE_SUCCESS);
    
    // Setup metadata for 5 fragments with threshold of 3
    memset(metadata.key_id, 0xAA, RKE_KEY_ID_SIZE);
    metadata.key_type = RKE_KEY_TYPE_SYMMETRIC;
    metadata.total_fragments = 5;
    metadata.threshold = 3;
    metadata.timestamp = (uint32_t)time(NULL);
    metadata.den = 1;
    metadata.sn = 12345;
    
    // Test key splitting
    result = rke_split_key(original_key, 256, &metadata);
    ASSERT(result == RKE_SUCCESS);
    
    // Test key reconstruction
    result = rke_reconstruct_key(reconstructed_key, 256, &metadata);
    ASSERT(result == RKE_SUCCESS);
    
    // Verify keys match
    ASSERT(memcmp(original_key, reconstructed_key, 256) == 0);
    
    // Test invalid parameters for splitting
    result = rke_split_key(NULL, 256, &metadata);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    result = rke_split_key(original_key, 256, NULL);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    // Test invalid threshold (greater than total fragments)
    metadata.threshold = 6;
    result = rke_split_key(original_key, 256, &metadata);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    // Test threshold too low
    metadata.threshold = 1;
    result = rke_split_key(original_key, 256, &metadata);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    printf("Key splitting tests passed!\n");
    return 0;
}

/*
 * Test fragment integrity checking
 */
int test_fragment_integrity() {
    struct rke_fragment_t fragment;
    
    printf("Testing fragment integrity...\n");
    
    // Setup test fragment
    fragment.fragment_id = 1;
    fragment.total_fragments = 5;
    fragment.threshold = 3;
    fragment.fragment_size = 64;
    
    // Fill with test data
    for (int i = 0; i < 64; i++) {
        fragment.data[i] = (unsigned char)(i * 3 + 42);
    }
    
    // Test checksum calculation
    int result = rke_calculate_checksum(&fragment);
    ASSERT(result == RKE_SUCCESS);
    
    // Test checksum verification
    result = rke_verify_checksum(&fragment);
    ASSERT(result == RKE_SUCCESS);
    
    // Test corruption detection
    unsigned char original_byte = fragment.data[0];
    fragment.data[0] = ~original_byte;  // Corrupt first byte
    result = rke_verify_checksum(&fragment);
    ASSERT(result == RKE_ERROR_FRAGMENT_CORRUPT);
    
    // Restore original data
    fragment.data[0] = original_byte;
    result = rke_verify_checksum(&fragment);
    ASSERT(result == RKE_SUCCESS);
    
    // Test invalid parameters
    result = rke_calculate_checksum(NULL);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    result = rke_verify_checksum(NULL);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    printf("Fragment integrity tests passed!\n");
    return 0;
}

/*
 * Test fragment validation
 */
int test_fragment_validation() {
    struct rke_fragment_t fragment;
    
    printf("Testing fragment validation...\n");
    
    // Setup valid fragment
    fragment.fragment_id = 3;
    fragment.total_fragments = 5;
    fragment.threshold = 3;
    fragment.fragment_size = 128;
    
    // Test valid fragment
    int result = rke_validate_fragment(&fragment);
    ASSERT(result == RKE_SUCCESS);
    
    // Test invalid fragment ID (0)
    fragment.fragment_id = 0;
    result = rke_validate_fragment(&fragment);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    // Test invalid fragment ID (greater than total)
    fragment.fragment_id = 6;
    result = rke_validate_fragment(&fragment);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    // Restore valid ID
    fragment.fragment_id = 3;
    
    // Test invalid threshold
    fragment.threshold = 6;
    result = rke_validate_fragment(&fragment);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    // Restore valid threshold
    fragment.threshold = 3;
    
    // Test invalid fragment size
    fragment.fragment_size = RKE_FRAGMENT_DATA_SIZE + 1;
    result = rke_validate_fragment(&fragment);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    // Test NULL parameter
    result = rke_validate_fragment(NULL);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    printf("Fragment validation tests passed!\n");
    return 0;
}

/*
 * Test complete key lifecycle
 */
int test_key_lifecycle() {
    unsigned char original_key[256];
    unsigned char reconstructed_key[256];
    struct rke_key_metadata_t metadata;
    
    printf("Testing complete key lifecycle...\n");
    
    // Step 1: Generate key
    int result = rke_generate_key(original_key, 256);
    ASSERT(result == RKE_SUCCESS);
    
    // Step 2: Setup metadata
    for (int i = 0; i < RKE_KEY_ID_SIZE; i++) {
        metadata.key_id[i] = (unsigned char)(i + 100);
    }
    metadata.key_type = RKE_KEY_TYPE_SYMMETRIC;
    metadata.total_fragments = 7;
    metadata.threshold = 4;
    metadata.timestamp = (uint32_t)time(NULL);
    metadata.den = 2;
    metadata.sn = 67890;
    
    // Step 3: Split key
    result = rke_split_key(original_key, 256, &metadata);
    ASSERT(result == RKE_SUCCESS);
    
    // Step 4: Reconstruct key
    result = rke_reconstruct_key(reconstructed_key, 256, &metadata);
    ASSERT(result == RKE_SUCCESS);
    
    // Step 5: Verify reconstruction
    ASSERT(memcmp(original_key, reconstructed_key, 256) == 0);
    
    printf("Key lifecycle test passed!\n");
    return 0;
}

/*
 * Main test function
 */
int main() {
    printf("RKE Core Tests\n");
    printf("==============\n");
    
    // Set log level to reduce noise during testing
    current_log_level = LOG_LEVEL_ERROR;
    
    // Run all tests
    TEST_FUNCTION(test_key_generation);
    TEST_FUNCTION(test_key_splitting);
    TEST_FUNCTION(test_fragment_integrity);
    TEST_FUNCTION(test_fragment_validation);
    TEST_FUNCTION(test_key_lifecycle);
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Total tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ Some tests failed!\n");
        return 1;
    }
}