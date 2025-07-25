# Merkle Tree Integrity System (integrity.c)

## Module Purpose
This module implements a comprehensive Merkle Tree-based integrity verification and self-healing system for the RAIDA network. It provides distributed consensus mechanisms, DDoS-resistant two-stage protocols, selective hashing for standardized verification, automated healing capabilities, and fixed critical bugs in TCP request logic to maintain network data integrity across all RAIDA servers.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `TOTAL_DENOMINATIONS` | Variable | Number of supported coin denominations |
| `HASH_SIZE` | 32 | Size of SHA-256 hash values in bytes |
| `TOTAL_RAIDA_SERVERS` | 25 | Number of servers in RAIDA network |
| `TOTAL_PAGES` | Variable | Number of pages per denomination |
| `RECORDS_PER_PAGE` | Variable | Number of coin records per page |
| `STATUS_SUCCESS` | 250 | Success status code for integrity operations |

## Data Structures

### Merkle Tree Structure
| Field | Type | Description |
|-------|------|-------------|
| `levels` | Hash Array Pointer Array | 2D array of hash pointers for each tree level |
| `num_levels` | Integer | Total number of levels in the tree |
| `leaf_count` | Integer | Number of leaf nodes (pages) in the tree |

### Tree Node Coordinates
| Field | Size | Description |
|-------|------|-------------|
| `level` | 4 bytes | Tree level (0 = leaf level, increases toward root) |
| `index` | 4 bytes | Node index within the specified level |

### Cache Management
| Field | Type | Description |
|-------|------|-------------|
| `merkle_tree_cache[TOTAL_DENOMINATIONS]` | Tree Pointer Array | Cached Merkle trees for each denomination |
| `merkle_tree_locks[TOTAL_DENOMINATIONS]` | Mutex Array | Thread safety locks for each cached tree |

## Core Functionality

### 1. Initialize Integrity System (`init_integrity_system`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Initializes the Merkle Tree integrity system with master on/off switch support, thread-safe caching, and background synchronization processes.

**Process:**
1. **Configuration Check:**
   - Checks `synchronization_enabled` flag in configuration
   - If disabled, logs warning and returns success without initialization
   - Enables controlled deployment of integrity features

2. **Cache Initialization:**
   - Initializes cache array for all denominations to NULL
   - Creates thread-safety mutexes for each denomination
   - Sets up cache management data structures

3. **Background Thread Startup:**
   - Launches Merkle sync thread for periodic operations
   - Configures thread for continuous background operation
   - Establishes thread cleanup and shutdown procedures

**Feature Control:**
- **Master Switch:** Allows deployment with integrity system disabled
- **Safe Deployment:** System operates normally without integrity features
- **Gradual Rollout:** Enables controlled feature activation across network

**Used By:** Server initialization, system startup

**Dependencies:** Configuration system, threading infrastructure

### 2. Merkle Sync Thread (`merkle_sync_thread`)
**Parameters:**
- Thread argument (unused)

**Returns:** Thread result

**Purpose:** Background thread implementing the two-stage integrity verification and healing protocol with distributed consensus and automated recovery.

**Process:**
1. **Periodic Cycle Initiation:**
   - Sleeps for configured integrity frequency
   - Wakes up to perform integrity checking cycle
   - Continues until system shutdown signal

2. **Local Merkle Tree Rebuild:**
   - Rebuilds all Merkle trees for current server
   - Updates cache with fresh tree data for all denominations
   - Establishes baseline for integrity comparison

3. **Root Collection:**
   - Collects root hashes from all denomination trees
   - Builds complete root hash collection for comparison
   - Handles missing roots with error reporting

4. **Stage 1: UDP Quick Vote (DDoS-Proof):**
   - Sends UDP vote requests to all other RAIDA servers
   - Includes complete local root hash collection in request
   - Receives match/no-match votes from peers
   - Counts votes to determine consensus status

5. **Consensus Evaluation:**
   - If majority matches: Local data is correct, cycle complete
   - If minority matches: Proceed to Stage 2 for healing
   - Logs vote results for debugging and monitoring

6. **Stage 2: TCP Ballot Collection (Reliable & Secure):**
   - Sends TCP requests to disagreeing RAIDA servers
   - Uses fixed TCP request function with proper parameters
   - Collects complete root hash collections via reliable protocol
   - Analyzes collected data to determine true majority

7. **Healing Process:**
   - Identifies denominations with disagreement by comparing roots
   - For each disagreeing denomination:
     - Performs binary search through Merkle tree structure
     - Identifies specific corrupted pages
     - Downloads correct page data from trusted peer
     - Replaces corrupted local data atomically

**Security Features:**
- **DDoS Resistance:** UDP stage prevents amplification attacks
- **Byzantine Fault Tolerance:** Works correctly despite server failures
- **Majority Consensus:** Democratic decision-making across network
- **Authenticated Healing:** Only trusted sources used for data repair

**Used By:** Background integrity maintenance

**Dependencies:** Network layer, Merkle tree construction, consensus protocols

### 3. Find and Heal Discrepancies (`find_and_heal_discrepancies`)
**Parameters:**
- Denomination (8-bit integer)
- Trusted peer RAIDA index (integer)
- Majority root hashes (byte array)

**Returns:** None (static function)

**Purpose:** Performs intelligent binary search through Merkle tree to identify and heal specific corrupted pages with minimal network overhead.

**Process:**
1. **Tree Access Setup:**
   - Acquires denomination-specific lock for thread safety
   - Accesses cached Merkle tree for denomination
   - Validates tree exists and is properly constructed

2. **Binary Search Queue Initialization:**
   - Creates traversal queue for systematic tree examination
   - Allocates memory for node coordinates (level, index pairs)
   - Starts from root level and works down to leaf level
   - Uses breadth-first approach for efficient healing

3. **Tree Traversal Process:**
   - For each tree node in queue:
     - Requests corresponding node hash from trusted peer using fixed TCP function
     - Compares local hash with trusted peer's hash
     - If hashes match: subtree is correct, skip children
     - If hashes differ: add child nodes to examination queue

4. **Leaf Node Healing:**
   - When reaching leaf level (individual pages):
     - Identifies specific corrupted pages
     - Calls heal_page function for each corrupted page
     - Downloads complete page data from trusted peer
     - Replaces local page file atomically

5. **Child Node Calculation:**
   - For internal nodes with differences:
     - Adds left child (index * 2) to queue
     - Calculates nodes in child level to determine if right child exists
     - Adds right child (index * 2 + 1) if it exists in tree

6. **Resource Cleanup:**
   - Frees traversal queue memory
   - Releases denomination lock
   - Ensures no memory leaks

**Performance Features:**
- **Logarithmic Complexity:** O(log n) navigation through tree structure
- **Minimal Network Usage:** Only downloads necessary data for comparison
- **Targeted Healing:** Focuses healing effort on actually corrupted data
- **Efficient Queue Management:** Breadth-first traversal minimizes memory usage

**Used By:** Merkle sync thread during healing operations

**Dependencies:** Network communication, file system operations, memory management

### 4. Heal Page (`heal_page`)
**Parameters:**
- Denomination (8-bit integer)
- Page number (integer)
- Trusted RAIDA index (integer)

**Returns:** None (static function)

**Purpose:** Downloads and atomically replaces a specific corrupted page with correct data from a trusted RAIDA server.

**Process:**
1. **Request Construction:**
   - Builds request body with denomination and page number
   - Uses fixed TCP integrity request function
   - Prepares for reliable data transfer

2. **Data Request:**
   - Sends page data request to trusted RAIDA server using command 4
   - Uses fixed TCP function with correct body length
   - Validates response size matches expected page size (RECORDS_PER_PAGE * 17)
   - Handles network errors and invalid responses

3. **File Path Construction:**
   - Builds hierarchical path using denomination and page MSB
   - Uses same directory structure as database layer
   - Ensures consistent file organization

4. **Atomic File Replacement:**
   - Opens local page file with O_CREAT | O_TRUNC flags
   - Writes complete page data in single operation
   - Validates write operation success
   - Closes file to ensure atomic update

5. **Error Handling:**
   - Handles file creation failures
   - Manages write operation errors
   - Frees response data in all cases
   - Logs successful healing operations

**Security Features:**
- **Trusted Source:** Only downloads from consensus-verified peers
- **Atomic Updates:** Complete page replacement prevents partial corruption
- **Validation:** Write operations validated for success
- **Data Integrity:** Response size validation ensures complete data

**Used By:** Binary search healing process

**Dependencies:** Network layer, file system operations

### 5. Send UDP Vote Request (`send_udp_vote_request`)
**Parameters:**
- RAIDA index (integer)
- Local root hashes (byte array)

**Returns:** Integer (1=match, 0=no match, -1=error/timeout, static function)

**Purpose:** Sends DDoS-resistant UDP vote request to peer RAIDA server for quick consensus check.

**Process:**
1. **Request Construction:**
   - Creates UDP packet with command ID 7 for integrity vote
   - Includes complete local root hash collection (TOTAL_DENOMINATIONS * HASH_SIZE bytes)
   - Generates cryptographic nonce (16 bytes) for response validation
   - Appends nonce to request for replay protection

2. **Network Communication:**
   - Creates UDP socket for communication
   - Sets receive timeout to 2 seconds for responsiveness
   - Sends UDP packet to specified RAIDA server using configuration addresses
   - Handles network errors gracefully

3. **Response Validation:**
   - Receives response with vote (1 byte) and nonce echo (16 bytes)
   - Validates response size matches expected format
   - Verifies nonce echo matches sent nonce for security
   - Returns vote result to caller

4. **Resource Cleanup:**
   - Closes socket after operation
   - Handles timeout and error conditions
   - Returns appropriate error codes

**Security Features:**
- **Nonce Validation:** Prevents replay attacks and ensures response authenticity
- **Timeout Protection:** Short timeout prevents hanging operations
- **Size Validation:** Response format validation prevents protocol attacks

**Used By:** UDP vote collection phase

**Dependencies:** Network stack, configuration addresses

### 6. Send TCP Integrity Request (`send_tcp_integrity_request`)
**Parameters:**
- RAIDA index (integer)
- Command identifier (integer)
- Request body (byte array)
- Body length (integer)
- Response body pointer (byte array pointer)
- Response length pointer (integer pointer)

**Returns:** Integer (0 for success, -1 for failure, static function)

**Purpose:** Sends generic TCP request to another RAIDA server for integrity operations with fixed critical bugs in request logic.

**Process:**
1. **Connection Establishment:**
   - Creates TCP socket for reliable communication
   - Sets receive timeout to 10 seconds for reliable operations
   - Connects to target RAIDA server using configuration addresses
   - Handles connection failures gracefully

2. **Critical Bug Fix - Request Construction:**
   - Calculates total body size: 16 (challenge) + body_len + 2 (trailer)
   - Calculates total request size: REQUEST_HEADER_SIZE + total_body_size
   - Allocates request buffer with correct size
   - Initializes header with proper fields

3. **Header Population:**
   - Sets target RAIDA index in header byte 2
   - Sets command group to 14 (INTEGRITY) in header byte 4
   - Sets specific command identifier in header byte 5
   - Sets body size in header bytes 22-23 (big-endian format)

4. **Body Construction:**
   - Generates 12-byte random challenge at body start
   - Calculates CRC32 of challenge and stores in bytes 12-15
   - Copies actual request body after 16-byte challenge
   - Adds 2-byte trailer (0x3e, 0x3e) at end

5. **Request Transmission:**
   - Sends complete request using TCP send
   - Validates send operation success
   - Frees request buffer after transmission

6. **Response Processing:**
   - Receives response header (RESPONSE_HEADER_SIZE bytes)
   - Validates response status is STATUS_SUCCESS
   - Extracts response body size from header bytes 9-11
   - Adjusts size to exclude 2-byte trailer

7. **Response Body Handling:**
   - Allocates response body buffer if size > 0
   - Receives response body using MSG_WAITALL for complete data
   - Reads and discards 2-byte trailer
   - Returns response data to caller

8. **Resource Management:**
   - Closes socket in all cases
   - Frees allocated memory on errors
   - Sets output pointers appropriately

**Critical Bug Fixes:**
- **Proper Parameter Passing:** Function now accepts command and body_len parameters
- **Correct Size Calculations:** Fixed request size calculation logic
- **Proper Body Length:** Uses actual body_len parameter instead of hardcoded values
- **Flexible Command Support:** Supports different integrity commands (2, 4, 5)

**Used By:** Ballot collection, binary search communication, page data requests

**Dependencies:** Network stack, protocol layer, CRC calculation

### 7. Build Merkle Tree for Denomination (`build_merkle_tree_for_denomination`)
**Parameters:**
- Denomination (8-bit integer)

**Returns:** Merkle tree pointer (NULL on failure)

**Purpose:** Constructs complete Merkle tree for a denomination using selective hashing to ensure consistent verification across servers with different default data.

**Process:**
1. **Leaf Hash Generation (Selective Hashing):**
   - For each page in denomination (0 to TOTAL_PAGES):
     - Constructs page file path using hierarchical structure
     - Opens and reads complete page data (RECORDS_PER_PAGE * 17 bytes)
     - Applies selective hashing algorithm to standardize data
     - Generates SHA-256 hash of standardized page representation

2. **Selective Hashing Algorithm:**
   - For each coin record in page:
     - **Circulating Coins (MFS ≠ 0):** Include actual 16-byte authentication number
     - **Available Coins (MFS = 0):** Use standardized placeholder (16 zeros)
   - Creates standardized buffer (RECORDS_PER_PAGE * 16 bytes)
   - Hashes standardized buffer to produce consistent page hash

3. **Tree Structure Calculation:**
   - Calculates required tree levels: ceil(log2(leaf_count)) + 1
   - Allocates tree structure and level arrays
   - Handles memory allocation failures gracefully

4. **Level 0 Initialization (Leaf Level):**
   - Allocates array for leaf node hashes
   - Copies calculated page hashes to leaf level
   - Each leaf represents one page's standardized hash

5. **Tree Construction (Bottom-Up):**
   - For each internal level (1 to num_levels-1):
     - Calculates nodes in level: (nodes_in_previous_level + 1) / 2
     - Allocates hash arrays for level nodes
     - For each node in level:
       - Combines two child hashes using SHA-256
       - Handles odd node counts by duplicating last node
       - Stores resulting hash in parent node

6. **Root Level:**
   - Top level contains single root hash
   - Root represents hash of entire denomination's data
   - Used for consensus comparison with other servers

**Selective Hashing Benefits:**
- **Standardization:** Ensures identical hashes across servers despite different default data
- **Consistency:** Only actual coin data affects hash values
- **Compatibility:** Works correctly regardless of page initialization methods
- **Integrity:** Detects any changes to actual coin data

**Error Handling:**
- **File Access Errors:** Missing pages handled with zero hashes
- **Memory Allocation:** Graceful failure with proper cleanup
- **Tree Construction:** Validates each step of tree building

**Used By:** Merkle sync thread, tree cache management

**Dependencies:** File system access, cryptographic hashing, memory management

### 8. Get Merkle Root (`get_merkle_root`)
**Parameters:**
- Denomination (8-bit integer)
- Output hash buffer (32 bytes)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Retrieves root hash for specified denomination from cache with thread-safe access.

**Process:**
1. **Index Calculation:**
   - Converts denomination to array index using get_den_idx
   - Validates denomination is within supported range

2. **Thread-Safe Cache Access:**
   - Acquires denomination-specific mutex
   - Accesses cached tree for denomination
   - Validates tree exists and is properly constructed

3. **Root Extraction:**
   - Copies root hash from tree structure
   - Root is located at highest level (num_levels - 1), index 0
   - Returns 32-byte SHA-256 hash to output buffer

4. **Error Handling:**
   - Returns error if tree not cached or invalid
   - Handles missing or corrupted tree data
   - Releases mutex in all cases (success or failure)

**Used By:** Integrity verification, consensus protocols, UDP vote requests

**Dependencies:** Thread synchronization, cache management

### 9. Get Merkle Branch (`get_merkle_branch`)
**Parameters:**
- Denomination (8-bit integer)
- Level (integer)
- Index (integer)
- Depth (integer)
- Output branch data pointer (byte array pointer)
- Output branch size pointer (integer pointer)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Extracts branch data from Merkle tree for healing operations, supporting multi-level queries for efficient tree traversal.

**Process:**
1. **Parameter Validation:**
   - Converts denomination to index and validates range
   - Acquires tree lock and validates tree exists
   - Ensures level and depth are within tree bounds

2. **Size Calculation:**
   - For each depth level from specified level downward:
     - Calculates node range for current level
     - Calculates number of nodes in level
     - Accounts for tree structure and leaf count
     - Accumulates total data size needed

3. **Memory Allocation:**
   - Allocates output buffer for all hash data
   - Handles allocation failures gracefully
   - Returns appropriate error codes

4. **Branch Data Extraction:**
   - For each requested depth level:
     - Calculates node range based on index and depth
     - Copies hash data for all nodes in range
     - Advances to next level toward leaves
     - Updates offset in output buffer

5. **Response Construction:**
   - Returns total size and data pointer to caller
   - Sets output parameters appropriately
   - Releases tree lock after completion

**Used By:** Binary search healing, tree navigation, integrity commands

**Dependencies:** Cache management, memory allocation, tree structure calculations

### 10. Free Merkle Tree (`free_merkle_tree`)
**Parameters:**
- Tree pointer

**Returns:** None

**Purpose:** Safely deallocates all memory associated with a Merkle tree structure.

**Process:**
1. **Validation:**
   - Checks tree pointer is not NULL
   - Returns immediately if tree is NULL

2. **Level-by-Level Cleanup:**
   - For each tree level (0 to num_levels-1):
     - Calculates number of nodes in level
     - Frees individual hash node memory
     - Frees level array memory
     - Handles partially constructed trees

3. **Structure Cleanup:**
   - Frees levels array pointer
   - Frees main tree structure
   - Ensures complete memory cleanup

**Used By:** Cache management, system shutdown, error recovery

**Dependencies:** Memory management

### 11. Hash Data (`hash_data`)
**Parameters:**
- Data buffer 1 (byte array)
- Length 1 (integer)
- Data buffer 2 (byte array, optional)
- Length 2 (integer)
- Output hash buffer (32 bytes)

**Returns:** None (static function)

**Purpose:** Provides unified SHA-256 hashing for both single and combined data buffers used throughout the integrity system.

**Process:**
1. **Hash Context Initialization:**
   - Initializes SHA-256 context structure
   - Prepares for data input processing

2. **Data Processing:**
   - Updates hash with first data buffer and length
   - If second buffer provided and length > 0:
     - Updates hash with second data buffer
   - Handles single or combined data hashing

3. **Hash Finalization:**
   - Finalizes SHA-256 calculation
   - Outputs 32-byte hash to provided buffer
   - Ensures consistent hash generation

**Used By:** Tree construction, page hashing, node combination

**Dependencies:** Cryptographic library (OpenSSL SHA-256)

## Two-Stage DDoS-Resistant Protocol

### Stage 1: UDP Quick Vote
- **Purpose:** Fast consensus check with DDoS protection
- **Method:** UDP packets with complete root hash collections
- **Protection:** Requires peer to submit their own data (proof of work)
- **Outcome:** Majority/minority determination for local data
- **Timeout:** 2-second timeout for responsiveness

### Stage 2: TCP Ballot Collection  
- **Purpose:** Reliable consensus building for healing
- **Method:** TCP connections for guaranteed delivery
- **Security:** Challenge-response authentication with CRC validation
- **Data:** Complete root hash collections for consensus analysis
- **Timeout:** 10-second timeout for reliability

### Healing Phase
- **Binary Search:** Efficient identification of corrupted data using tree structure
- **Targeted Downloads:** Only corrupted pages downloaded from trusted peers
- **Atomic Updates:** Complete page replacement prevents partial corruption
- **Verification:** All operations validated for success

## Performance Characteristics

### Network Efficiency
- **UDP Stage:** Minimal network overhead for common case (no healing needed)
- **Binary Search:** Logarithmic complexity reduces healing network traffic
- **Targeted Healing:** Only corrupted data downloaded and replaced
- **Fixed TCP Bugs:** Proper request construction improves reliability

### Memory Management
- **Tree Caching:** Hot trees kept in memory for fast access
- **Selective Construction:** Trees built only when needed
- **Memory Bounds:** Cache size limits prevent memory exhaustion
- **Proper Cleanup:** All allocated memory properly freed

### Computational Efficiency
- **SHA-256 Optimization:** Hardware acceleration when available
- **Incremental Updates:** Trees rebuilt only when necessary
- **Parallel Processing:** Multiple denomination trees processed concurrently
- **Selective Hashing:** Standardized hashing reduces computation overhead

## Security Considerations

### Consensus Security
- **Byzantine Fault Tolerance:** Correct operation despite arbitrary server failures
- **Majority Rule:** Democratic decision-making prevents minority attacks
- **Authenticated Communication:** All inter-server communication authenticated
- **Nonce Protection:** UDP requests include nonce for replay protection

### Data Integrity
- **Cryptographic Hashing:** SHA-256 provides strong integrity guarantees
- **Atomic Operations:** Page updates are atomic to prevent corruption
- **Verification:** All downloaded data verified before use
- **Selective Hashing:** Ensures consistent verification across different server states

### DDoS Protection
- **Resource Limits:** Bounded response sizes protect server resources
- **Proof of Work:** UDP requests require peer participation
- **Rate Limiting Ready:** Fixed operation costs enable rate limiting
- **Timeout Management:** Appropriate timeouts prevent resource holding

## Dependencies and Integration

### Required Modules
- **Database Layer:** Page file access and coin data reading
- **Network Layer:** UDP/TCP communication with other RAIDA servers
- **Configuration System:** Integrity frequency, enable/disable flags
- **Cryptographic Library:** SHA-256 hashing functions
- **Threading System:** Background thread management and synchronization

### External Constants Required
- `TOTAL_RAIDA_SERVERS`: Network size for consensus calculations
- `TOTAL_DENOMINATIONS`: Number of denominations for tree cache
- `RECORDS_PER_PAGE`: Page structure for hash calculations
- Configuration settings for timing and network addresses

### Used By
- **Integrity Command Handlers:** Provide access to tree data and nodes
- **Network Maintenance:** Automated integrity verification and healing
- **Administrative Tools:** Manual integrity verification and reporting
- **System Monitoring:** Integrity status reporting and alerting

### Cross-File Dependencies
- **Database Module:** Page file structure and access methods
- **Network Module:** Inter-RAIDA communication protocols
- **Configuration Module:** System settings and RAIDA server addresses
- **Utilities Module:** Cryptographic hashing functions and denomination utilities

## Threading and Concurrency

### Thread Safety Design
- **Per-Denomination Locks:** Fine-grained locking prevents contention
- **Cache Protection:** Tree cache protected by mutex arrays
- **Background Processing:** Sync thread operates independently
- **Safe Updates:** Tree updates use proper synchronization

### Concurrent Operations
- **Multiple Trees:** Different denominations processed concurrently
- **Network Parallelism:** Multiple RAIDA servers contacted simultaneously
- **Read-Write Separation:** Read operations don't block each other
- **Lock-Free Algorithms:** Minimal locking for performance

## Administrative Interface

### Manual Control
- **Enable/Disable:** Master switch in configuration file
- **Frequency Control:** Configurable integrity checking intervals
- **Debug Information:** Detailed logging of integrity operations
- **Status Reporting:** Real-time integrity status visibility

### Monitoring and Alerting  
- **Consensus Failures:** Alerts when local data differs from network
- **Healing Operations:** Logging of all healing activities
- **Performance Metrics:** Timing and efficiency measurements
- **Network Health:** Visibility into RAIDA server connectivity

## Critical Bug Fixes

### TCP Request Logic Fixes
- **Parameter Flexibility:** Fixed function to accept command and body length parameters
- **Size Calculations:** Corrected request and body size calculation logic
- **Command Support:** Added support for different integrity commands (2, 4, 5)
- **Memory Management:** Fixed memory allocation and cleanup logic

### Protocol Improvements
- **Request Construction:** Proper header and body formatting
- **Response Handling:** Correct response size parsing and validation
- **Error Recovery:** Improved error handling and resource cleanup
- **Network Reliability:** Better timeout and connection management

This integrity system provides comprehensive data verification and healing capabilities for the RAIDA network, ensuring consistent data across all servers while protecting against denial-of-service attacks and providing efficient recovery from data corruption or inconsistencies, with critical bug fixes for improved reliability.