/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : test_rke_protocol.c
#   Last Modified : 2024-07-20
#   Describe      : Unit tests for RKE protocol commands
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
#include "../src/common/protocol.h"

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
 * Helper function to create mock connection info
 */
conn_info_t* create_mock_connection(unsigned char *body, uint32_t body_size) {
    conn_info_t *ci = malloc(sizeof(conn_info_t));
    if (ci == NULL) return NULL;
    
    memset(ci, 0, sizeof(conn_info_t));
    ci->body = body;
    ci->body_size = body_size;
    ci->output = NULL;
    ci->output_size = 0;
    ci->command_status = -999; // Invalid initial status
    
    // Set a test nonce
    for (int i = 0; i < 16; i++) {
        ci->nonce[i] = (unsigned char)(i * 17);
    }
    
    return ci;
}

/*
 * Helper function to cleanup mock connection
 */
void cleanup_mock_connection(conn_info_t *ci) {
    if (ci == NULL) return;
    
    if (ci->output != NULL) {
        free(ci->output);
    }
    free(ci);
}

/*
 * Test RKE Generate command
 */
int test_rke_generate_command() {
    printf("Testing RKE Generate command...\n");
    
    // Create test payload: 16-byte key_id + 1-byte key_type + 1-byte total_fragments + 1-byte threshold + 2-byte EOF
    unsigned char payload[21];
    
    // Fill key_id
    for (int i = 0; i < 16; i++) {
        payload[i] = (unsigned char)(i + 50);
    }
    payload[16] = RKE_KEY_TYPE_SYMMETRIC;  // key_type
    payload[17] = 5;                       // total_fragments
    payload[18] = 3;                       // threshold
    payload[19] = 0xFF;                    // EOF
    payload[20] = 0xFF;                    // EOF
    
    // Test valid command
    conn_info_t *ci = create_mock_connection(payload, 21);
    ASSERT(ci != NULL);
    
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == STATUS_SUCCESS);
    ASSERT(ci->output_size == 1);
    ASSERT(ci->output != NULL);
    ASSERT(ci->output[0] == 0x01);
    
    cleanup_mock_connection(ci);
    
    // Test invalid packet length
    ci = create_mock_connection(payload, 20);  // Too short
    ASSERT(ci != NULL);
    
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == ERROR_INVALID_PACKET_LENGTH);
    
    cleanup_mock_connection(ci);
    
    // Test invalid threshold (greater than total fragments)
    payload[17] = 3;  // total_fragments
    payload[18] = 5;  // threshold (invalid)
    
    ci = create_mock_connection(payload, 21);
    ASSERT(ci != NULL);
    
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == ERROR_INVALID_PARAMETER);
    
    cleanup_mock_connection(ci);
    
    printf("RKE Generate command tests passed!\n");
    return 0;
}

/*
 * Test RKE Query command
 */
int test_rke_query_command() {
    printf("Testing RKE Query command...\n");
    
    // First, generate a key to query
    unsigned char generate_payload[21];
    for (int i = 0; i < 16; i++) {
        generate_payload[i] = (unsigned char)(i + 100);
    }
    generate_payload[16] = RKE_KEY_TYPE_SYMMETRIC;
    generate_payload[17] = 5;  // total_fragments
    generate_payload[18] = 3;  // threshold
    generate_payload[19] = 0xFF;
    generate_payload[20] = 0xFF;
    
    conn_info_t *ci = create_mock_connection(generate_payload, 21);
    ASSERT(ci != NULL);
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == STATUS_SUCCESS);
    cleanup_mock_connection(ci);
    
    // Now test query command
    unsigned char query_payload[18];
    memcpy(query_payload, generate_payload, 16);  // Copy key_id
    query_payload[16] = 0xFF;  // EOF
    query_payload[17] = 0xFF;  // EOF
    
    ci = create_mock_connection(query_payload, 18);
    ASSERT(ci != NULL);
    
    cmd_rke_query(ci);
    ASSERT(ci->command_status == STATUS_SUCCESS);
    ASSERT(ci->output_size == sizeof(struct rke_key_metadata_t) + 32);  // metadata + fragment bitmap
    ASSERT(ci->output != NULL);
    
    cleanup_mock_connection(ci);
    
    // Test invalid packet length
    ci = create_mock_connection(query_payload, 17);  // Too short
    ASSERT(ci != NULL);
    
    cmd_rke_query(ci);
    ASSERT(ci->command_status == ERROR_INVALID_PACKET_LENGTH);
    
    cleanup_mock_connection(ci);
    
    printf("RKE Query command tests passed!\n");
    return 0;
}

/*
 * Test packet validation
 */
int test_packet_validation() {
    printf("Testing packet validation...\n");
    
    unsigned char payload[50];
    
    // Test RKE Generate with various invalid lengths
    for (int len = 0; len < 21; len++) {
        conn_info_t *ci = create_mock_connection(payload, len);
        ASSERT(ci != NULL);
        
        cmd_rke_generate(ci);
        ASSERT(ci->command_status == ERROR_INVALID_PACKET_LENGTH);
        
        cleanup_mock_connection(ci);
    }
    
    // Test RKE Query with various invalid lengths
    for (int len = 0; len < 18; len++) {
        conn_info_t *ci = create_mock_connection(payload, len);
        ASSERT(ci != NULL);
        
        cmd_rke_query(ci);
        ASSERT(ci->command_status == ERROR_INVALID_PACKET_LENGTH);
        
        cleanup_mock_connection(ci);
    }
    
    // Test RKE Reconstruct with various invalid lengths
    for (int len = 0; len < 18; len++) {
        conn_info_t *ci = create_mock_connection(payload, len);
        ASSERT(ci != NULL);
        
        cmd_rke_reconstruct(ci);
        ASSERT(ci->command_status == ERROR_INVALID_PACKET_LENGTH);
        
        cleanup_mock_connection(ci);
    }
    
    printf("Packet validation tests passed!\n");
    return 0;
}

/*
 * Test error conditions
 */
int test_error_conditions() {
    printf("Testing error conditions...\n");
    
    // Test RKE Generate with extreme parameters
    unsigned char payload[21];
    
    // Fill key_id
    for (int i = 0; i < 16; i++) {
        payload[i] = (unsigned char)(i + 200);
    }
    
    // Test with too many fragments
    payload[16] = RKE_KEY_TYPE_SYMMETRIC;
    payload[17] = 255;  // total_fragments (at limit)
    payload[18] = 254;  // threshold
    payload[19] = 0xFF;
    payload[20] = 0xFF;
    
    conn_info_t *ci = create_mock_connection(payload, 21);
    ASSERT(ci != NULL);
    
    cmd_rke_generate(ci);
    // This might succeed or fail depending on implementation limits
    // Just check that it doesn't crash and sets a valid status
    ASSERT(ci->command_status != -999);
    
    cleanup_mock_connection(ci);
    
    // Test with zero threshold
    payload[17] = 5;   // total_fragments
    payload[18] = 0;   // threshold (invalid)
    
    ci = create_mock_connection(payload, 21);
    ASSERT(ci != NULL);
    
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == ERROR_INVALID_PARAMETER);
    
    cleanup_mock_connection(ci);
    
    // Test with threshold = 1 (below minimum)
    payload[18] = 1;   // threshold (below minimum)
    
    ci = create_mock_connection(payload, 21);
    ASSERT(ci != NULL);
    
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == ERROR_INVALID_PARAMETER);
    
    cleanup_mock_connection(ci);
    
    printf("Error condition tests passed!\n");
    return 0;
}

/*
 * Test session management
 */
int test_session_management() {
    printf("Testing session management...\n");
    
    struct rke_session_t session;
    unsigned char sender_id[RKE_KEY_ID_SIZE];
    unsigned char receiver_id[RKE_KEY_ID_SIZE];
    
    // Initialize test IDs
    for (int i = 0; i < RKE_KEY_ID_SIZE; i++) {
        sender_id[i] = (unsigned char)(i + 10);
        receiver_id[i] = (unsigned char)(i + 20);
    }
    
    // Test session initialization
    int result = rke_init_session(&session, sender_id, receiver_id);
    ASSERT(result == RKE_SUCCESS);
    ASSERT(memcmp(session.sender_id, sender_id, RKE_KEY_ID_SIZE) == 0);
    ASSERT(memcmp(session.receiver_id, receiver_id, RKE_KEY_ID_SIZE) == 0);
    ASSERT(session.state == RKE_SESSION_STATE_INIT);
    
    // Test session cleanup
    rke_cleanup_session(&session);
    
    // Test invalid parameters
    result = rke_init_session(NULL, sender_id, receiver_id);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    result = rke_init_session(&session, NULL, receiver_id);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    result = rke_init_session(&session, sender_id, NULL);
    ASSERT(result == RKE_ERROR_INVALID_PARAM);
    
    printf("Session management tests passed!\n");
    return 0;
}

/*
 * Test complete protocol flow
 */
int test_protocol_flow() {
    printf("Testing complete protocol flow...\n");
    
    // Step 1: Generate a key
    unsigned char generate_payload[21];
    for (int i = 0; i < 16; i++) {
        generate_payload[i] = (unsigned char)(i + 150);
    }
    generate_payload[16] = RKE_KEY_TYPE_SYMMETRIC;
    generate_payload[17] = 5;  // total_fragments
    generate_payload[18] = 3;  // threshold
    generate_payload[19] = 0xFF;
    generate_payload[20] = 0xFF;
    
    conn_info_t *ci = create_mock_connection(generate_payload, 21);
    ASSERT(ci != NULL);
    cmd_rke_generate(ci);
    ASSERT(ci->command_status == STATUS_SUCCESS);
    cleanup_mock_connection(ci);
    
    // Step 2: Query the key
    unsigned char query_payload[18];
    memcpy(query_payload, generate_payload, 16);  // Copy key_id
    query_payload[16] = 0xFF;
    query_payload[17] = 0xFF;
    
    ci = create_mock_connection(query_payload, 18);
    ASSERT(ci != NULL);
    cmd_rke_query(ci);
    ASSERT(ci->command_status == STATUS_SUCCESS);
    
    // Verify metadata in response
    struct rke_key_metadata_t *returned_metadata = (struct rke_key_metadata_t *)ci->output;
    ASSERT(returned_metadata->total_fragments == 5);
    ASSERT(returned_metadata->threshold == 3);
    ASSERT(memcmp(returned_metadata->key_id, generate_payload, 16) == 0);
    
    cleanup_mock_connection(ci);
    
    printf("Protocol flow tests passed!\n");
    return 0;
}

/*
 * Main test function
 */
int main() {
    printf("RKE Protocol Tests\n");
    printf("==================\n");
    
    // Set log level to reduce noise during testing
    current_log_level = LOG_LEVEL_ERROR;
    
    // Run all tests
    TEST_FUNCTION(test_rke_generate_command);
    TEST_FUNCTION(test_rke_query_command);
    TEST_FUNCTION(test_packet_validation);
    TEST_FUNCTION(test_error_conditions);
    TEST_FUNCTION(test_session_management);
    TEST_FUNCTION(test_protocol_flow);
    
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