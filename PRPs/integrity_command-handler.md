# Integrity Command Handlers (cmd_integrity)

## Module Purpose
This module implements command handlers for the Merkle Tree Integrity Protocol, supporting the DDoS-resistant two-stage synchronization system. It provides secure access to Merkle tree nodes, complete root hash collections, and raw page data for distributed integrity verification and healing operations across RAIDA servers.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `HASH_SIZE` | 32 | Size of SHA-256 hash values used in Merkle trees |
| `RECORDS_PER_PAGE` | Variable | Number of coin records per database page |
| `TOTAL_DENOMINATIONS` | Variable | Total number of supported coin denominations |

## Error Codes
| Constant | Description |
|----------|-------------|
| `ERROR_INVALID_PACKET_LENGTH` | Request payload size is incorrect for the operation |
| `ERROR_NOT_FOUND` | Requested Merkle tree node could not be found |
| `ERROR_MEMORY_ALLOC` | Failed to allocate memory for response buffer |
| `ERROR_FILE_NOT_EXIST` | Required database page file does not exist |
| `ERROR_FILESYSTEM` | File system operation failed during page access |

## Status Codes
| Constant | Description |
|----------|-------------|
| `STATUS_SUCCESS` | Operation completed successfully |

## Core Functionality

### 1. Get Merkle Node (`cmd_get_merkle_node`)
**Parameters:**
- Connection information structure containing request data
- Input: 27-byte payload (challenge + denomination + level + index + trailer)

**Returns:** None (modifies connection structure with 32-byte node hash)

**Purpose:** Provides access to specific nodes in the Merkle tree hierarchy, supporting the binary search phase of the distributed healing process for precise identification of corrupted data.

**Process:**
1. **Request Validation:**
   - Validates exact 27-byte payload size
   - Extracts denomination, tree level, and node index
   - Validates parameters within valid ranges

2. **Node Retrieval:**
   - Calls Merkle tree interface to retrieve specified node
   - Requests branch data with depth of 1 (single node)
   - Validates node exists at specified coordinates

3. **Response Generation:**
   - Returns exactly 32 bytes (SHA-256 hash size)
   - Provides raw node hash for comparison operations
   - Enables precise tree traversal during healing

**Security Features:**
- Challenge-response authentication in request
- Validated tree coordinates prevent invalid access
- Direct hash comparison enables tamper detection

**Used By:** Distributed healing protocols, integrity verification workflows

**Dependencies:** Integrity system for Merkle tree access

### 2. Get All Root Hashes (TCP) (`cmd_get_all_roots`)
**Parameters:**
- Connection information structure
- Input: 498-byte payload (challenge + peer's 480-byte root list + trailer)

**Returns:** None (modifies connection structure with all denomination root hashes)

**Purpose:** Implements the reliable "Ballot Collection" phase of the DDoS-resistant protocol by securely exchanging complete root hash collections between RAIDA servers for consensus building.

**Process:**
1. **DDoS Protection Validation:**
   - Validates exact 498-byte payload size
   - Requires peer to provide their own 480-byte root hash collection
   - Implements proof-of-work concept to prevent abuse

2. **Root Hash Collection:**
   - Allocates response buffer for all denomination root hashes
   - Iterates through all supported denominations
   - Retrieves current root hash for each denomination

3. **Missing Root Handling:**
   - For denominations without computed Merkle trees:
     - Fills root hash space with zeros
     - Ensures consistent response structure
     - Maintains protocol compatibility

4. **Response Structure:**
   - Total size: TOTAL_DENOMINATIONS × HASH_SIZE bytes
   - Sequential root hashes for all denominations
   - Fixed format enables efficient comparison

**Security Features:**
- DDoS protection through required peer root submission
- TCP-based reliable delivery for critical consensus data
- Consistent response format prevents manipulation

**Used By:** Distributed integrity consensus, network synchronization

**Dependencies:** Integrity system for root hash access

### 3. Get Page Data (`cmd_get_page_data`)
**Parameters:**
- Connection information structure
- Input: 23-byte payload (challenge + denomination + page number + trailer)

**Returns:** None (modifies connection structure with complete page data)

**Purpose:** Provides access to raw database page content for the final healing step, enabling complete page replacement when Merkle tree verification identifies corrupted data.

**Process:**
1. **Request Validation:**
   - Validates exact 23-byte payload size
   - Extracts target denomination and page number
   - Validates parameters within system limits

2. **Direct File Access:**
   - Constructs file path for specified page
   - Uses hierarchical directory structure (denomination/MSB/page.bin)
   - Opens page file in read-only mode

3. **Complete Page Transfer:**
   - Allocates buffer for full page data (RECORDS_PER_PAGE × 17 bytes)
   - Reads entire page content from disk
   - Validates complete read operation

4. **Response Generation:**
   - Returns raw page data for direct replacement
   - Includes all coin records and metadata
   - Enables atomic page healing operations

**Security Features:**
- Direct file access ensures data authenticity
- Complete page transfer prevents partial corruption
- Read-only access maintains data integrity

**Used By:** Final healing phase, page-level recovery operations

**Dependencies:** File system access, database page structure

## Data Structures and Formats

### Request Formats
| Operation | Size | Structure |
|-----------|------|-----------|
| Get Merkle Node | 27 bytes | Challenge (16) + Denomination (1) + Level (4) + Index (4) + Trailer (2) |
| Get All Roots | 498 bytes | Challenge (16) + Peer Roots (480) + Trailer (2) |
| Get Page Data | 23 bytes | Challenge (16) + Denomination (1) + Page Number (4) + Trailer (2) |

### Response Formats
| Operation | Response Size | Content |
|-----------|-------------|---------|
| Get Merkle Node | 32 bytes | Single SHA-256 hash |
| Get All Roots | 480 bytes | Root hashes for all denominations (TOTAL_DENOMINATIONS × 32) |
| Get Page Data | Variable | Complete page data (RECORDS_PER_PAGE × 17 bytes) |

### Page Data Structure
| Field | Size | Description |
|-------|------|-------------|
| Coin Record | 17 bytes | Authentication Number (16) + MFS Status (1) |
| Page Content | RECORDS_PER_PAGE × 17 | All coin records in sequential order |

### Merkle Tree Coordinates
| Field | Size | Description |
|-------|------|-------------|
| Denomination | 1 byte | Target coin denomination |
| Level | 4 bytes | Tree level (0 = leaf level) |
| Index | 4 bytes | Node index within level |

## Security Considerations

### DDoS Protection
- **Proof of Work:** Get All Roots requires peer root submission
- **Request Validation:** All requests include challenge-response authentication
- **Size Limits:** Fixed payload sizes prevent buffer overflow attacks

### Data Integrity
- **Direct Access:** Page data retrieved directly from authoritative files
- **Hash Verification:** Merkle nodes provide cryptographic verification
- **Atomic Operations:** Complete page transfers prevent partial corruption

### Access Control
- **Challenge Authentication:** All requests include authentication challenges
- **Read-Only Access:** No modification operations exposed
- **Parameter Validation:** All coordinates and identifiers validated

## Error Handling and Validation

### Input Validation
- **Size Validation:** Exact payload sizes enforced for all operations
- **Parameter Validation:** Denomination, level, and index values validated
- **Structure Validation:** Request format and trailer validation

### File System Error Handling
- **Missing Files:** Graceful handling of non-existent page files
- **Read Errors:** Proper error reporting for file system failures
- **Path Validation:** Secure path construction prevents directory traversal

### Memory Management
- **Allocation Validation:** Response buffer allocation checked
- **Resource Cleanup:** Proper cleanup on error conditions
- **Size Calculation:** Response sizes calculated safely

## Performance Characteristics

### Direct File Access
- **Bypass Cache:** Direct file access for guaranteed current data
- **Efficient I/O:** Single read operations for complete pages
- **Minimal Overhead:** Direct access avoids cache layer overhead

### Network Optimization
- **Fixed Sizes:** Predictable response sizes for network planning
- **Batch Operations:** Complete data sets transferred in single operations
- **Compression Ready:** Fixed formats suitable for network compression

### Memory Efficiency
- **Single Allocation:** Response buffers allocated once per operation
- **Minimal Copying:** Direct file-to-buffer transfers
- **Resource Management:** Immediate cleanup after response generation

## Dependencies and Integration

### Required Modules
- **Integrity System:** Merkle tree management and node access
- **File System:** Direct database page file access
- **Configuration Module:** Directory structure and denomination limits
- **Database Structure:** Page organization and record formats

### External Constants Required
- `TOTAL_DENOMINATIONS`: Number of supported denominations
- `RECORDS_PER_PAGE`: Page structure organization
- `HASH_SIZE`: SHA-256 hash size constant
- Database directory structure configuration

### Used By
- **Healing Protocols:** Binary search and page replacement operations
- **Integrity Verification:** Distributed consensus and validation
- **Network Synchronization:** Root hash comparison and consensus building
- **Recovery Tools:** Manual and automated recovery operations

### Cross-File Dependencies
- **Integrity Module:** Merkle tree construction and node access
- **Database Layer:** Page file structure and organization
- **Configuration Module:** Directory paths and system limits
- **File System Interface:** Direct file access capabilities

## Protocol Integration

### Two-Stage Healing Protocol
- **Stage 1 Support:** Get Merkle Node enables binary search phase
- **Stage 2 Support:** Get All Roots enables consensus building
- **Final Phase:** Get Page Data enables complete healing

### DDoS-Resistant Design
- **Proof of Work:** Required peer participation in Get All Roots
- **Rate Limiting Ready:** Fixed operation costs enable rate limiting
- **Resource Protection:** Bounded response sizes protect server resources

### Network Protocol Compliance
- **Challenge-Response:** Full authentication challenge support
- **Standard Headers:** Compatible with existing protocol infrastructure
- **Error Reporting:** Standard error codes and response formats

## Threading and Concurrency

### Thread Safety
- **Read-Only Operations:** All operations are read-only for thread safety
- **File System Safety:** Direct file access with proper error handling
- **Memory Safety:** Independent response buffers per operation

### Concurrent Access
- **Multiple Requests:** Supports concurrent integrity verification
- **Resource Isolation:** Each operation uses independent resources
- **Scalable Design:** Operations scale with available system resources

## Backward Compatibility

### Protocol Versioning
- **Standard Compliance:** Full compliance with integrity protocol specification
- **Format Stability:** Fixed response formats ensure compatibility
- **Error Handling:** Standard error codes for consistent behavior

### Integration Support
- **Legacy Systems:** Compatible with existing healing infrastructure
- **Migration Support:** Supports gradual rollout of integrity features
- **Fallback Mechanisms:** Graceful degradation when integrity data unavailable

This integrity command module provides essential infrastructure for the RAIDA network's distributed integrity verification and healing system, enabling secure and efficient data recovery while protecting against denial-of-service attacks and maintaining compatibility with existing network infrastructure.