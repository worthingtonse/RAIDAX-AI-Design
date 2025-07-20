/* ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : rke_storage.c
#   Last Modified : 2024-07-20
#   Describe      : RKE storage functions - fragment and metadata persistence
#
# ====================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../common/protocol.h"
#include "../common/log.h"
#include "../common/utils.h"
#include "rke.h"

/*
 * Store a key fragment to the filesystem
 * Path pattern: {config.cwd}/RKE/{key_id_prefix}/fragment_{fragment_id}.bin
 */
int rke_store_fragment(const struct rke_fragment_t *fragment, const unsigned char *key_id) {
    char fragment_path[4096];
    char dir_path[4096];
    int fd, rv;
    
    // Validate input parameters
    if (fragment == NULL || key_id == NULL) {
        error("Invalid parameters for fragment storage");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Validate fragment structure
    if (rke_validate_fragment(fragment) != RKE_SUCCESS) {
        error("Fragment validation failed");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Create directory path using first 4 bytes of key_id as prefix
    snprintf(dir_path, sizeof(dir_path), "%s/RKE/%02x%02x%02x%02x", 
            config.cwd, key_id[0], key_id[1], key_id[2], key_id[3]);
    
    // Create directory structure if it doesn't exist
    if (create_directory_recursive(dir_path) != 0) {
        error("Failed to create directory %s: %s", dir_path, strerror(errno));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Construct full file path
    snprintf(fragment_path, sizeof(fragment_path), "%s/fragment_%03d.bin", dir_path, fragment->fragment_id);
    
    debug("Storing fragment %d to %s", fragment->fragment_id, fragment_path);
    
    // Open file for writing with secure permissions
    fd = open(fragment_path, O_CREAT | O_WRONLY | O_TRUNC, 0640);
    if (fd < 0) {
        error("Failed to create fragment file %s: %s", fragment_path, strerror(errno));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Write fragment data
    rv = write(fd, fragment, sizeof(struct rke_fragment_t));
    close(fd);
    
    if (rv != sizeof(struct rke_fragment_t)) {
        error("Failed to write fragment data: wrote %d, expected %zu", rv, sizeof(struct rke_fragment_t));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    debug("Successfully stored fragment %d (%d bytes)", fragment->fragment_id, rv);
    return RKE_SUCCESS;
}

/*
 * Load a key fragment from the filesystem
 */
int rke_load_fragment(struct rke_fragment_t *fragment, const unsigned char *key_id, uint8_t fragment_id) {
    char fragment_path[4096];
    int fd, rv;
    
    // Validate input parameters
    if (fragment == NULL || key_id == NULL || fragment_id == 0) {
        error("Invalid parameters for fragment loading");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Construct file path
    snprintf(fragment_path, sizeof(fragment_path), "%s/RKE/%02x%02x%02x%02x/fragment_%03d.bin",
            config.cwd, key_id[0], key_id[1], key_id[2], key_id[3], fragment_id);
    
    debug("Loading fragment %d from %s", fragment_id, fragment_path);
    
    // Open file for reading
    fd = open(fragment_path, O_RDONLY);
    if (fd < 0) {
        error("Failed to open fragment file %s: %s", fragment_path, strerror(errno));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Read fragment data
    rv = read(fd, fragment, sizeof(struct rke_fragment_t));
    close(fd);
    
    if (rv != sizeof(struct rke_fragment_t)) {
        error("Failed to read fragment data: read %d, expected %zu", rv, sizeof(struct rke_fragment_t));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Validate loaded fragment
    if (rke_validate_fragment(fragment) != RKE_SUCCESS) {
        error("Loaded fragment failed validation");
        return RKE_ERROR_FRAGMENT_CORRUPT;
    }
    
    // Verify fragment ID matches
    if (fragment->fragment_id != fragment_id) {
        error("Fragment ID mismatch: expected %d, got %d", fragment_id, fragment->fragment_id);
        return RKE_ERROR_FRAGMENT_CORRUPT;
    }
    
    debug("Successfully loaded fragment %d (%d bytes)", fragment_id, rv);
    return RKE_SUCCESS;
}

/*
 * Store key metadata to the filesystem
 * Path pattern: {config.cwd}/RKE/{key_id_prefix}/metadata.bin
 */
int rke_store_metadata(const struct rke_key_metadata_t *metadata) {
    char metadata_path[4096];
    char dir_path[4096];
    int fd, rv;
    
    // Validate input parameters
    if (metadata == NULL) {
        error("Invalid parameters for metadata storage");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Create directory path using first 4 bytes of key_id as prefix
    snprintf(dir_path, sizeof(dir_path), "%s/RKE/%02x%02x%02x%02x", 
            config.cwd, metadata->key_id[0], metadata->key_id[1], 
            metadata->key_id[2], metadata->key_id[3]);
    
    // Create directory structure if it doesn't exist
    if (create_directory_recursive(dir_path) != 0) {
        error("Failed to create directory %s: %s", dir_path, strerror(errno));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Construct full file path
    snprintf(metadata_path, sizeof(metadata_path), "%s/metadata.bin", dir_path);
    
    debug("Storing metadata to %s", metadata_path);
    
    // Open file for writing with secure permissions
    fd = open(metadata_path, O_CREAT | O_WRONLY | O_TRUNC, 0640);
    if (fd < 0) {
        error("Failed to create metadata file %s: %s", metadata_path, strerror(errno));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Write metadata
    rv = write(fd, metadata, sizeof(struct rke_key_metadata_t));
    close(fd);
    
    if (rv != sizeof(struct rke_key_metadata_t)) {
        error("Failed to write metadata: wrote %d, expected %zu", rv, sizeof(struct rke_key_metadata_t));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    debug("Successfully stored metadata (%d bytes)", rv);
    return RKE_SUCCESS;
}

/*
 * Load key metadata from the filesystem
 */
int rke_load_metadata(struct rke_key_metadata_t *metadata, const unsigned char *key_id) {
    char metadata_path[4096];
    int fd, rv;
    
    // Validate input parameters
    if (metadata == NULL || key_id == NULL) {
        error("Invalid parameters for metadata loading");
        return RKE_ERROR_INVALID_PARAM;
    }
    
    // Construct file path
    snprintf(metadata_path, sizeof(metadata_path), "%s/RKE/%02x%02x%02x%02x/metadata.bin",
            config.cwd, key_id[0], key_id[1], key_id[2], key_id[3]);
    
    debug("Loading metadata from %s", metadata_path);
    
    // Open file for reading
    fd = open(metadata_path, O_RDONLY);
    if (fd < 0) {
        error("Failed to open metadata file %s: %s", metadata_path, strerror(errno));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Read metadata
    rv = read(fd, metadata, sizeof(struct rke_key_metadata_t));
    close(fd);
    
    if (rv != sizeof(struct rke_key_metadata_t)) {
        error("Failed to read metadata: read %d, expected %zu", rv, sizeof(struct rke_key_metadata_t));
        return RKE_ERROR_STORAGE_FAIL;
    }
    
    // Verify key_id matches
    if (memcmp(metadata->key_id, key_id, RKE_KEY_ID_SIZE) != 0) {
        error("Key ID mismatch in loaded metadata");
        return RKE_ERROR_FRAGMENT_CORRUPT;
    }
    
    debug("Successfully loaded metadata (%d bytes)", rv);
    return RKE_SUCCESS;
}

/*
 * Check if a fragment exists in storage
 */
int rke_fragment_exists(const unsigned char *key_id, uint8_t fragment_id) {
    char fragment_path[4096];
    
    if (key_id == NULL || fragment_id == 0) {
        return 0;
    }
    
    snprintf(fragment_path, sizeof(fragment_path), "%s/RKE/%02x%02x%02x%02x/fragment_%03d.bin",
            config.cwd, key_id[0], key_id[1], key_id[2], key_id[3], fragment_id);
    
    return (access(fragment_path, F_OK) == 0) ? 1 : 0;
}

/*
 * Count available fragments for a key
 */
int rke_count_fragments(const unsigned char *key_id) {
    int count = 0;
    
    if (key_id == NULL) {
        return 0;
    }
    
    // Check fragments 1-255
    for (int i = 1; i <= 255; i++) {
        if (rke_fragment_exists(key_id, i)) {
            count++;
        }
    }
    
    debug("Found %d fragments for key", count);
    return count;
}