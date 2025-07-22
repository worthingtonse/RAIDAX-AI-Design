# Merkle Tree Integrity System (integrity.c)

## Module Purpose
This module implements a comprehensive Merkle Tree-based integrity verification and self-healing system for the RAIDA network. It provides distributed consensus mechanisms, DDoS-resistant two-stage protocols, selective hashing for standardized verification, and automated healing capabilities to maintain network data integrity across all RAIDA servers.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `TOTAL_DENOMINATIONS` | Variable | Number of supported coin denominations |
| `HASH_SIZE` | 32 | Size of SHA-256 hash values in bytes |
| `TOTAL_RAIDA_SERVERS` | 25 | Number of servers in RAIDA network |
| `TOTAL_PAGES` | Variable | Number of pages per denomination |
| `RECORDS_PER_PAGE` | Variable | Number of coin records per page |

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
   - Initializes cache array for all denominations
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
   - Updates cache with fresh tree data
   - Establishes baseline for integrity comparison

3. **Stage 1: UDP Quick Vote (DDoS-Proof):**
   - Sends UDP vote requests to all other RAIDA servers
   - Includes complete root hash collection in request
   - Receives match/no-match votes from peers
   - Counts votes to determine consensus status

4. **Consensus Evaluation:**
   - If majority matches: Local data is correct, cycle complete
   - If minority matches: Proceed to Stage 2 for healing

5. **Stage 2: TCP Ballot Collection (Reliable & Secure):**
   - Sends TCP requests to disagreeing RAIDA servers
   - Collects complete root hash collections via reliable protocol
   - Analyzes collected data to determine true majority
   - Identifies trusted peer with correct data

6. **Healing Process:**
   - For each denomination with disagreement:
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

**Returns:** None

**Purpose:** Performs intelligent binary search through Merkle tree to identify and heal specific corrupted pages with minimal network overhead.

**Process:**
1. **Tree Traversal Setup:**
   - Creates traversal queue for systematic tree examination
   - Starts from root level and works down to leaf level
   - Uses breadth-first approach for efficient healing

2. **Binary Search Process:**
   - For each tree node:
     - Requests corresponding node hash from trusted peer
     - Compares local hash with trusted peer's hash
     - If hashes match: subtree is correct, skip children
     - If hashes differ: add child nodes to examination queue

3. **Leaf Node Healing:**
   - When reaching leaf level (individual pages):
     - Identifies specific corrupted pages
     - Downloads complete page data from trusted peer
     - Replaces local page file atomically

4. **Efficient Navigation:**
   - Uses tree structure to minimize network requests
   - Focuses healing effort on actually corrupted data
   - Avoids unnecessary downloads of correct data

**Performance Features:**
- **Logarithmic Complexity:** O(log n) navigation through tree structure
- **Minimal Network Usage:** Only downloads necessary data
- **Targeted Healing:** Focuses on actually corrupted pages
- **Atomic Replacement:** Page updates are atomic operations

**Used By:** Merkle sync thread during healing operations

**Dependencies:** Network communication, file system operations

### 4. Heal Page (`heal_page`)
**Parameters:**
- Denomination (8-bit integer)
- Page number (integer)
- Trusted RAIDA index (integer)

**Returns:** None

**Purpose:** Downloads and atomically replaces a specific corrupted page with correct data from a trusted RAIDA server.

**Process:**
1. **Data Request:**
   - Constructs page data request for trusted RAIDA server
   - Sends request via TCP for reliable delivery
   - Validates response size and format

2. **Atomic Replacement:**
   - Opens local page file in write mode
   - Writes complete page data in single operation
   - Closes file to ensure atomic update

3. **Validation:**
   - Verifies complete write operation
   - Logs successful healing operation
   - Updates internal state as needed

**Security Features:**
- **Trusted Source:** Only downloads from consensus-verified peers
- **Atomic Updates:** Complete page replacement prevents partial corruption
- **Validation:** Write operations validated for success

**Used By:** Binary search healing process

**Dependencies:** Network layer, file system operations

### 5. Build Merkle Tree for Denomination (`build_merkle_tree_for_denomination`)
**Parameters:**
- Denomination (8-bit integer)

**Returns:** Merkle tree pointer (NULL on failure)

**Purpose:** Constructs complete Merkle tree for a denomination using selective hashing to ensure consistent verification across servers with different default data.

**Process:**
1. **Leaf Hash Generation (Selective Hashing):**
   - For each page in denomination:
     - Reads complete page data from disk
     - Applies selective hashing algorithm:
       - **Circulating Coins (MFS ≠ 0):** Include actual authentication number
       - **Available Coins (MFS = 0):** Use standardized placeholder (zeros)
     - Generates SHA-256 hash of standardized page representation
     - Stores hash as leaf node in tree

2. **Tree Construction:**
   - Calculates required tree levels based on leaf count
   - Builds tree bottom-up from leaf level to root
   - For each internal level:
     - Pairs adjacent nodes from level below
     - Combines paired hashes using SHA-256
     - Handles odd node counts by duplicating last node

3. **Memory Management:**
   - Allocates tree structure and all node arrays
   - Handles memory allocation failures gracefully
   - Provides cleanup function for tree destruction

**Selective Hashing Benefits:**
- **Standardization:** Ensures identical hashes across servers despite different default data
- **Consistency:** Only actual coin data affects hash values
- **Compatibility:** Works correctly regardless of page initialization methods
- **Integrity:** Detects any changes to actual coin data

**Used By:** Merkle sync thread, tree cache management

**Dependencies:** File system access, cryptographic hashing

### 6. Get Merkle Root (`get_merkle_root`)
**Parameters:**
- Denomination (8-bit integer)
- Output hash buffer (32 bytes)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Retrieves root hash for specified denomination from cache with thread-safe access.

**Process:**
1. **Cache Access:**
   - Acquires denomination-specific mutex
   - Accesses cached tree for denomination
   - Validates tree exists and is properly constructed

2. **Root Extraction:**
   - Copies root hash from tree structure
   - Root is located at highest level, index 0
   - Returns 32-byte SHA-256 hash

3. **Error Handling:**
   - Returns error if tree not cached
   - Handles missing or corrupted tree data
   - Releases mutex in all cases

**Used By:** Integrity verification, consensus protocols, UDP vote requests

**Dependencies:** Thread synchronization, cache management

### 7. Get Merkle Branch (`get_merkle_branch`)
**Parameters:**
- Denomination (8-bit integer)
- Level (integer)
- Index (integer)  
- Depth (integer)
- Output branch data pointer
- Output branch size pointer

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Extracts branch data from Merkle tree for healing operations, supporting multi-level queries for efficient tree traversal.

**Process:**
1. **Parameter Validation:**
   - Validates level and index within tree bounds
   - Ensures depth doesn't exceed tree structure
   - Calculates total data size needed

2. **Branch Extraction:**
   - For each requested depth level:
     - Calculates node range for current level
     - Copies hash data for all nodes in range
     - Advances to next level toward leaves

3. **Response Construction:**
   - Allocates output buffer for all hash data
   - Copies hash data sequentially
   - Returns total size and data pointer

**Used By:** Binary search healing, tree navigation

**Dependencies:** Cache management, memory allocation

### 8. Network Communication Functions

#### Send UDP Vote Request (`send_udp_vote_request`)
**Parameters:**
- RAIDA index (integer)
- Local root hashes (byte array)

**Returns:** Integer (1=match, 0=no match, -1=error)

**Purpose:** Sends DDoS-resistant UDP vote request to peer RAIDA server for quick consensus check.

**Process:**
1. **Request Construction:**
   - Creates UDP packet with command ID 7
   - Includes complete local root hash collection
   - Adds cryptographic nonce for response validation

2. **Network Communication:**
   - Sends UDP packet to specified RAIDA server
   - Configures short timeout for responsiveness
   - Handles network errors gracefully

3. **Response Validation:**
   - Validates response format and nonce
   - Extracts match/no-match vote
   - Returns consensus vote to caller

#### Send TCP Get All Roots (`send_tcp_get_all_roots`)
**Parameters:**
- RAIDA index (integer)
- Request body data (byte array)
- Response body pointer
- Response length pointer

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Sends reliable TCP request for complete root hash collection or specific tree data.

**Process:**
1. **Connection Establishment:**
   - Creates TCP connection to target RAIDA server
   - Configures appropriate timeouts
   - Handles connection failures

2. **Protocol Communication:**
   - Constructs proper protocol headers
   - Includes challenge-response authentication
   - Sends request with specified body data

3. **Response Processing:**
   - Receives and validates response headers
   - Extracts response body data
   - Validates response format and size

**Used By:** Ballot collection, binary search communication

**Dependencies:** Network stack, protocol layer

### 9. Free Merkle Tree (`free_merkle_tree`)
**Parameters:**
- Tree pointer

**Returns:** None

**Purpose:** Safely deallocates all memory associated with a Merkle tree structure.

**Process:**
1. **Level-by-Level Cleanup:**
   - Iterates through all tree levels
   - Frees individual hash node memory
   - Frees level array memory

2. **Structure Cleanup:**
   - Frees main tree structure
   - Nullifies pointer to prevent reuse
   - Ensures complete memory cleanup

**Used By:** Cache management, system shutdown

**Dependencies:** Memory management

## Selective Hashing Algorithm

### Standardization Process
1. **Page Reading:** Read complete page data (RECORDS_PER_PAGE × 17 bytes)
2. **Coin Processing:** For each coin record:
   - **If MFS ≠ 0:** Copy actual 16-byte authentication number
   - **If MFS = 0:** Use 16 bytes of zeros (standardized placeholder)
3. **Hash Generation:** Apply SHA-256 to standardized 16 × RECORDS_PER_PAGE byte buffer
4. **Result:** 32-byte hash that's identical across servers regardless of default data

### Benefits of Selective Hashing
- **Consensus Compatibility:** Different servers with different default data produce identical hashes
- **Integrity Detection:** Any change to actual coin data affects hash value
- **Efficiency:** Only meaningful data contributes to hash calculations
- **Standardization:** Provides consistent verification baseline across network

## Two-Stage DDoS-Resistant Protocol

### Stage 1: UDP Quick Vote
- **Purpose:** Fast consensus check with DDoS protection
- **Method:** UDP packets with complete root hash collections
- **Protection:** Requires peer to submit their own data (proof of work)
- **Outcome:** Majority/minority determination for local data

### Stage 2: TCP Ballot Collection  
- **Purpose:** Reliable consensus building for healing
- **Method:** TCP connections for guaranteed delivery
- **Security:** Challenge-response authentication
- **Data:** Complete root hash collections for consensus analysis

### Healing Phase
- **Binary Search:** Efficient identification of corrupted data
- **Targeted Downloads:** Only corrupted pages downloaded
- **Atomic Updates:** Complete page replacement prevents partial corruption

## Performance Characteristics

### Network Efficiency
- **UDP Stage:** Minimal network overhead for common case (no healing needed)
- **Binary Search:** Logarithmic complexity reduces healing network traffic
- **Targeted Healing:** Only corrupted data downloaded and replaced

### Memory Management
- **Tree Caching:** Hot trees kept in memory for fast access
- **Selective Construction:** Trees built only when needed
- **Memory Bounds:** Cache size limits prevent memory exhaustion

### Computational Efficiency
- **SHA-256 Optimization:** Hardware acceleration when available
- **Incremental Updates:** Trees rebuilt only when necessary
- **Parallel Processing:** Multiple denomination trees processed concurrently

## Security Considerations

### Consensus Security
- **Byzantine Fault Tolerance:** Correct operation despite arbitrary server failures
- **Majority Rule:** Democratic decision-making prevents minority attacks
- **Authenticated Communication:** All inter-server communication authenticated

### Data Integrity
- **Cryptographic Hashing:** SHA-256 provides strong integrity guarantees
- **Atomic Operations:** Page updates are atomic to prevent corruption
- **Verification:** All downloaded data verified before use

### DDoS Protection
- **Resource Limits:** Bounded response sizes protect server resources
- **Proof of Work:** UDP requests require peer participation
- **Rate Limiting Ready:** Fixed operation costs enable rate limiting

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
- **Utilities Module:** Cryptographic hashing functions

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

This integrity system provides comprehensive data verification and healing capabilities for the RAIDA network, ensuring consistent data across all servers while protecting against denial-of-service attacks and providing efficient recovery from data corruption or inconsistencies.