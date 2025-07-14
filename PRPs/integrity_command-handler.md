# Integrity Command Handlers (cmd_integrity.c)

## Module Purpose
This module implements the Merkle Tree Integrity Protocol command handlers for the RAIDA network, providing cryptographic verification of data integrity across the distributed system. It enables servers to exchange Merkle tree information for detecting and resolving data inconsistencies, ensuring network-wide data integrity through cryptographic proofs.

## Core Functionality

### 1. Merkle Root Retrieval (`cmd_get_merkle_root`)
**Parameters:**
- Connection information structure containing request data
- Input: 19-byte payload with denomination request

**Returns:** None (modifies connection structure with root hash)

**Purpose:** Provides the root hash of a Merkle tree for a specific denomination, enabling other servers to quickly verify data integrity at the highest level.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (19 bytes)
   - Extracts denomination from payload (1 byte)
   - Validates denomination within acceptable range

2. **Root Hash Retrieval:**
   - Calls integrity system to get Merkle root for denomination
   - Verifies Merkle tree exists and is built for requested denomination
   - Retrieves 32-byte SHA-256 root hash

3. **Response Generation:**
   - Allocates response buffer for hash data
   - Copies root hash to response buffer
   - Sets appropriate response size and status

**Security Features:**
- Cryptographic hash verification
- Denomination validation
- Integrity system integration

**Input Format:**
- 16 bytes: Challenge data
- 1 byte: Denomination identifier
- 2 bytes: End-of-frame marker

**Output Format:**
- 32 bytes: SHA-256 root hash of Merkle tree

**Dependencies:**
- Integrity system for Merkle tree management
- Cryptographic hash functions (SHA-256)

### 2. Merkle Node Retrieval (`cmd_get_merkle_node`)
**Parameters:**
- Connection information structure containing request data
- Input: 27-byte payload with denomination, level, and index

**Returns:** None (modifies connection structure with node hash)

**Purpose:** Provides a specific node hash from a Merkle tree, enabling detailed integrity verification and efficient synchronization between servers.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (27 bytes)
   - Extracts denomination (1 byte), level (4 bytes), and index (4 bytes)
   - Validates all parameters within acceptable ranges

2. **Node Hash Retrieval:**
   - Calls integrity system to get specific Merkle node
   - Verifies node exists at specified level and index
   - Retrieves 32-byte SHA-256 node hash

3. **Response Generation:**
   - Allocates response buffer for hash data
   - Copies node hash to response buffer
   - Sets appropriate response size and status

**Synchronization Features:**
- Granular tree node access
- Level-based tree traversal
- Efficient inconsistency detection

**Input Format:**
- 16 bytes: Challenge data
- 1 byte: Denomination identifier
- 4 bytes: Tree level (0 = leaf level)
- 4 bytes: Node index at specified level
- 2 bytes: End-of-frame marker

**Output Format:**
- 32 bytes: SHA-256 hash of requested node

**Dependencies:**
- Integrity system for Merkle tree management
- Cryptographic hash functions (SHA-256)

## Data Structures and Formats

### Hash Format
- **Hash Size:** 32 bytes (SHA-256 standard)
- **Root Hash:** Single hash representing entire denomination state
- **Node Hash:** Hash of specific tree node at given level and index
- **Leaf Hash:** Hash of individual page data (level 0)

### Request Format
- **Challenge:** 16-byte authentication challenge
- **Denomination:** 1-byte denomination identifier
- **Level:** 4-byte tree level identifier (0-based)
- **Index:** 4-byte node index within level
- **Trailer:** 2-byte end-of-frame marker

### Response Format
- **Hash Data:** 32-byte SHA-256 hash value
- **Status Code:** Success or specific error condition
- **Size Information:** Response payload size

### Tree Structure
- **Level 0:** Leaf nodes (page hashes)
- **Level N:** Internal nodes (combined child hashes)
- **Root Level:** Single root hash (top of tree)
- **Binary Tree:** Each internal node has up to 2 children

## Security Considerations

### Cryptographic Security
- **SHA-256 Hashing:** Cryptographically secure hash function
- **Tamper Detection:** Any data change affects hash chain
- **Collision Resistance:** Computationally infeasible to generate collisions
- **One-Way Function:** Cannot reverse engineer data from hash

### Data Integrity
- **Merkle Tree Properties:** Efficient integrity verification
- **Hierarchical Verification:** Can verify subsets without full data
- **Consistency Checking:** Detects inconsistencies across servers
- **Efficient Synchronization:** Minimal data transfer for verification

### Network Security
- **Challenge/Response:** Standard protocol authentication
- **Request Validation:** All parameters validated before processing
- **Error Handling:** Secure error reporting without information leakage
- **Resource Protection:** Prevents excessive resource consumption

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for exact expected length
- **Parameter Validation:** All tree parameters within acceptable ranges
- **Denomination Validation:** Denomination exists and is valid
- **Tree Validation:** Merkle tree exists and is built

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_NOT_FOUND`: Merkle tree or node not found
- `ERROR_MEMORY_ALLOC`: Memory allocation failure for response
- `STATUS_SUCCESS`: Operation completed successfully

### Recovery Mechanisms
- **Graceful Degradation:** Missing trees reported as not found
- **Resource Cleanup:** Memory properly released on errors
- **State Consistency:** No state changes on error conditions
- **Error Reporting:** Clear error codes for debugging

## Performance Characteristics

### Merkle Tree Efficiency
- **Logarithmic Verification:** Tree height grows logarithmically with data size
- **Selective Verification:** Can verify parts without full tree
- **Batch Operations:** Efficient batch verification possible
- **Memory Efficiency:** Only requested nodes need to be in memory

### Network Efficiency
- **Minimal Data Transfer:** Only hash values transmitted
- **Efficient Synchronization:** Can identify differences quickly
- **Parallel Verification:** Multiple nodes can be verified simultaneously
- **Caching Benefits:** Frequently accessed nodes cached

### Resource Management
- **Memory Allocation:** Dynamic allocation for response buffers
- **Resource Cleanup:** Proper cleanup on all code paths
- **Thread Safety:** Safe for concurrent access
- **Error Recovery:** Graceful handling of resource exhaustion

## Dependencies and Integration

### Required Modules
- **Integrity System:** Core Merkle tree management and generation
- **Cryptographic Libraries:** SHA-256 hash function implementation
- **Database Layer:** Access to underlying page data for tree generation
- **Network Protocol:** Standard request/response handling

### External Constants Required
- `HASH_SIZE`: Size of SHA-256 hash (32 bytes)
- `ERROR_*`: Error condition definitions
- `STATUS_SUCCESS`: Success status code
- Denomination range constants

### Used By
- **Network Synchronization:** Inter-server integrity verification
- **Administrative Tools:** Data integrity monitoring and verification
- **Healing Operations:** Consistency checking during recovery
- **Monitoring Systems:** Integrity status reporting

## Integration Points

### Integrity System Interface
- **get_merkle_root():** Retrieves root hash for denomination
- **get_merkle_node():** Retrieves specific node hash
- **Tree Management:** Background tree building and maintenance
- **Cache Integration:** Efficient access to tree data

### Network Protocol Integration
- **Command Registration:** Handlers registered with protocol system
- **Request Processing:** Standard protocol request handling
- **Response Generation:** Standard protocol response formatting
- **Error Handling:** Consistent error reporting

### Database Integration
- **Page Access:** Reads page data for tree generation
- **Denomination Support:** Works with all supported denominations
- **Data Consistency:** Reflects current database state
- **Cache Coordination:** Coordinates with page cache system

## Threading and Concurrency
- **Thread Safety:** All operations are thread-safe
- **Concurrent Access:** Multiple threads can access tree data
- **Resource Locking:** Proper locking for shared resources
- **Atomic Operations:** Tree access operations are atomic

## Merkle Tree Protocol
- **Binary Tree Structure:** Each node has at most 2 children
- **Bottom-Up Construction:** Built from leaf hashes upward
- **Efficient Verification:** Can verify data integrity efficiently
- **Hierarchical Organization:** Organized by levels and indices

## Use Cases

### Network Synchronization
- **Consistency Checking:** Verify data consistency across servers
- **Efficient Synchronization:** Identify differences quickly
- **Minimal Data Transfer:** Only transfer necessary hash values
- **Parallel Verification:** Multiple servers can verify simultaneously

### Data Integrity Monitoring
- **Continuous Monitoring:** Background integrity checking
- **Anomaly Detection:** Detect data corruption or tampering
- **Audit Trail:** Track integrity status over time
- **Alerting:** Notify of integrity violations

### Recovery Operations
- **Healing Support:** Verify data during recovery operations
- **Consistency Restoration:** Ensure consistent state after healing
- **Validation:** Verify recovery operations completed correctly
- **Quality Assurance:** Confirm data integrity after repairs

This integrity command module provides essential cryptographic verification capabilities for the RAIDA network, enabling efficient data integrity checking and synchronization while maintaining security and performance.