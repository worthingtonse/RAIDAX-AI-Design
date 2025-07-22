# Healing Command Handlers (cmd_healing)

## Module Purpose
This module implements the RAIDA network's healing and recovery commands, enabling distributed consensus-based coin recovery and network synchronization. It provides ticket-based authentication systems, distributed validation mechanisms, and consensus-driven coin recovery with dual hashing support for maintaining network integrity across RAIDA servers.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `TOTAL_RAIDA_SERVERS` | 25 | Total number of RAIDA servers in the network |
| `MAX_COINS_PER_TICKET` | Variable | Maximum coins that can be included in a single ticket |

## Error Codes
| Constant | Description |
|----------|-------------|
| `ERROR_INVALID_PACKET_LENGTH` | Request payload size is incorrect for the operation |
| `ERROR_COINS_NOT_DIV` | Coin data size not properly divisible by record size |
| `ERROR_MEMORY_ALLOC` | Failed to allocate memory for response buffer |
| `ERROR_NO_TICKET_SLOT` | No available ticket slots in the system |
| `ERROR_NO_TICKET_FOUND` | Specified ticket could not be found |
| `ERROR_TICKET_CLAIMED_ALREADY` | Ticket has already been claimed by the requesting RAIDA |
| `ERROR_WRONG_RAIDA` | Invalid RAIDA server identifier provided |

## Status Codes
| Constant | Description |
|----------|-------------|
| `STATUS_ALL_PASS` | All coins passed validation and ticket issued |
| `STATUS_ALL_FAIL` | No coins passed validation |
| `STATUS_MIXED` | Some coins passed validation (partial ticket) |
| `STATUS_SUCCESS` | Operation completed successfully |
| `STATUS_FIND_ALL_AN` | All coins found with current authentication numbers |
| `STATUS_FIND_ALL_PAN` | All coins found with proposed authentication numbers |
| `STATUS_FIND_NEITHER` | No coins found with either authentication number |
| `STATUS_FIND_MIXED` | Mixed results for coin authentication status |

## Core Functionality

### 1. Get Ticket (`cmd_get_ticket`)
**Parameters:**
- Connection information structure containing request data
- Input: Variable-length payload (minimum 39 bytes) containing coin authentication data

**Returns:** None (modifies connection structure with ticket information and validation results)

**Purpose:** Validates a collection of coins and issues a ticket containing authenticated coins for use in distributed healing operations across RAIDA servers.

**Process:**
1. **Request Validation:**
   - Validates minimum 39-byte payload size
   - Calculates coin count from payload (21 bytes per coin: denomination + serial number + authentication number)
   - Validates payload alignment to coin record boundaries

2. **Coin Authentication:**
   - For each coin in the request:
     - Extracts denomination and serial number
     - Retrieves coin page using on-demand cache
     - Compares provided authentication number with stored value
     - Sets validation bit for authentic coins
     - Counts passed and failed validations

3. **Ticket Generation:**
   - If any coins are authentic, allocates a ticket from the ticket pool
   - Stores validated coin information in the ticket
   - Generates unique ticket identifier
   - Tracks which coins passed validation

4. **Response Generation:**
   - **STATUS_ALL_PASS:** All coins authentic, ticket issued with ticket ID
   - **STATUS_ALL_FAIL:** No coins authentic, no ticket issued
   - **STATUS_MIXED:** Partial authentication, ticket issued with bitmap and ticket ID

**Security Features:**
- Individual coin authentication required
- Ticket-based proof of validation
- Distributed ticket system prevents replay attacks

**Used By:** Healing initiation workflows, distributed coin recovery

### 2. Validate Ticket (`cmd_validate_ticket`)
**Parameters:**
- Connection information structure
- Input: 23-byte payload (RAIDA ID + ticket identifier + reserved bytes)

**Returns:** None (modifies connection structure with ticket coin data)

**Purpose:** Allows other RAIDA servers to claim and validate tickets issued by this server, enabling distributed consensus and coin recovery operations.

**Process:**
1. **Request Validation:**
   - Validates exact 23-byte payload size
   - Extracts requesting RAIDA server identifier
   - Validates RAIDA ID within valid range (0 to TOTAL_RAIDA_SERVERS-1)

2. **Ticket Lookup:**
   - Searches ticket pool for specified ticket identifier
   - Validates ticket exists and is still valid
   - Checks ticket expiration and availability

3. **Claim Validation:**
   - Verifies the requesting RAIDA has not already claimed this ticket
   - Prevents duplicate claims from the same RAIDA server
   - Marks ticket as claimed by the requesting RAIDA

4. **Coin Data Response:**
   - Returns complete coin list from the ticket
   - Each coin includes denomination and serial number
   - Enables requesting RAIDA to verify coin states

**Security Features:**
- RAIDA authentication prevents unauthorized access
- Single-claim per RAIDA prevents duplicate consensus
- Ticket expiration ensures temporal validity

**Used By:** Distributed healing protocols, inter-RAIDA validation

### 3. Find Command (`cmd_find`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) with coin data and dual authentication numbers

**Returns:** None (modifies connection structure with coin status results)

**Purpose:** Checks coin status against both current and proposed authentication numbers, supporting healing operations that need to determine coin state during recovery.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size and coin alignment
   - Calculates coin count from payload (37 bytes per coin: denomination + serial number + current AN + proposed AN)

2. **Dual Authentication Check:**
   - For each coin:
     - Retrieves stored authentication number from database
     - Compares against provided current authentication number
     - Compares against provided proposed authentication number
     - Records which authentication number matches (if any)

3. **Status Classification:**
   - **Current AN Match:** Coin found with current authentication number
   - **Proposed AN Match:** Coin found with proposed authentication number  
   - **No Match:** Coin not found with either authentication number

4. **Response Generation:**
   - **STATUS_FIND_ALL_AN:** All coins match current authentication numbers
   - **STATUS_FIND_ALL_PAN:** All coins match proposed authentication numbers
   - **STATUS_FIND_NEITHER:** No coins match either authentication number
   - **STATUS_FIND_MIXED:** Mixed results with detailed response array

**Used By:** Healing assessment, coin state determination during recovery

### 4. Fix Command (`cmd_fix`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 139 bytes) with coin data, consensus data, and RAIDA tickets

**Returns:** None (modifies connection structure with fix results)

**Purpose:** Performs distributed consensus-based coin recovery by validating tickets from multiple RAIDA servers and applying healing based on network majority.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size and extracts coin count
   - Parses consensus data and RAIDA ticket information
   - Validates request structure and alignment

2. **Distributed Ticket Validation:**
   - Creates worker threads for each RAIDA server
   - Sends ticket validation requests to all other RAIDA servers
   - Collects responses from network peers
   - Handles network timeouts and connection failures

3. **Consensus Calculation:**
   - Counts votes for each coin from responding RAIDA servers
   - Requires majority consensus (> TOTAL_RAIDA_SERVERS/2 + 1)
   - Tracks which coins have sufficient consensus for healing

4. **Coin Healing with Dual Hash Support:**
   - For coins with majority consensus:
     - Constructs hash input from RAIDA number, denomination, serial number, and consensus data
     - **Legacy Clients (encryption_type < 4):** Uses MD5 hash for authentication number generation
     - **Modern Clients (encryption_type >= 4):** Uses SHA-256 hash for authentication number generation
     - Updates coin authentication number based on consensus
     - Sets MFS timestamp for healed coins
     - Marks affected pages as dirty for persistence

5. **Response Generation:**
   - Creates bitmap indicating which coins were successfully healed
   - **STATUS_ALL_PASS:** All coins healed successfully
   - **STATUS_ALL_FAIL:** No coins could be healed
   - **STATUS_MIXED:** Partial healing with detailed bitmap

**Security Features:**
- Distributed consensus prevents single-point-of-failure attacks
- Majority vote requirement ensures network agreement
- Dual hashing maintains compatibility across client versions
- Ticket-based validation ensures authenticity

**Used By:** Network healing operations, distributed coin recovery

### 5. Send Validate Ticket Job (`send_validate_ticket_job`)
**Parameters:**
- Thread argument structure containing RAIDA index, ticket ID, and connection information

**Returns:** Thread result (populated with coin data from remote RAIDA)

**Purpose:** Background thread function that communicates with other RAIDA servers to validate tickets and collect consensus data for healing operations.

**Process:**
1. **Network Connection:**
   - Establishes TCP connection to specified RAIDA server
   - Configures connection timeouts and socket options
   - Handles connection failures gracefully

2. **Protocol Communication:**
   - Constructs validate ticket request packet
   - Includes challenge-response authentication
   - Sends request with proper protocol headers

3. **Response Processing:**
   - Receives and validates response headers
   - Extracts coin data from response payload
   - Parses coin list (denomination + serial number pairs)

4. **Result Collection:**
   - Stores received coin data in thread argument structure
   - Handles parsing errors and invalid responses
   - Ensures memory cleanup on completion

**Threading Features:**
- Asynchronous execution for network communication
- Thread-safe result collection
- Proper resource cleanup on completion

**Used By:** Fix command for distributed consensus gathering

## Data Structures and Formats

### Request Formats
| Operation | Minimum Size | Structure |
|-----------|-------------|-----------|
| Get Ticket | 39 bytes | Coin Records (21 × N) + Reserved (18) |
| Validate Ticket | 23 bytes | RAIDA ID (1) + Ticket ID (4) + Reserved (18) |
| Find | 55 bytes | Coin Dual Records (37 × N) + Reserved (18) |
| Fix | 139 bytes | Coin Specs (5 × N) + Consensus Data (16) + RAIDA Tickets (4 × 25) + Reserved (18) |

### Coin Data Formats
| Format | Size | Structure |
|--------|------|-----------|
| Ticket Coin Record | 21 bytes | Denomination (1) + Serial Number (4) + Authentication Number (16) |
| Find Coin Record | 37 bytes | Denomination (1) + Serial Number (4) + Current AN (16) + Proposed AN (16) |
| Fix Coin Spec | 5 bytes | Denomination (1) + Serial Number (4) |

### Response Formats
| Operation | Response Format |
|-----------|----------------|
| Get Ticket | Validation bitmap + Ticket ID (if any coins validated) |
| Validate Ticket | Coin list (denomination + serial number pairs) |
| Find | Status array (1 byte per coin: 0x1=current AN, 0x2=proposed AN, 0x0=neither) |
| Fix | Healing bitmap indicating successfully healed coins |

### Ticket Structure
| Field | Size | Description |
|-------|------|-------------|
| Creation Time | 8 bytes | Timestamp when ticket was created |
| Ticket ID | 4 bytes | Unique ticket identifier |
| Coin Count | 4 bytes | Number of coins in ticket |
| Coin Array | Variable | Array of validated coins (denomination + serial number) |
| Claims Array | 25 bytes | Bitmap of RAIDA servers that have claimed this ticket |

## Security Considerations

### Distributed Security
- **Consensus Requirement:** Majority vote required for healing operations
- **Ticket-Based Authentication:** Prevents replay and forgery attacks
- **RAIDA Authentication:** Only valid RAIDA servers can claim tickets
- **Temporal Validation:** Tickets expire to prevent stale consensus

### Cryptographic Integrity
- **Dual Hash Support:** Maintains compatibility while providing security upgrade path
- **Challenge-Response:** Network communication includes authentication challenges
- **Secure Random Generation:** Ticket IDs generated with cryptographic randomness

### Network Security
- **Connection Timeouts:** Prevents resource exhaustion from slow peers
- **Input Validation:** All network inputs validated before processing
- **Resource Limits:** Ticket pool size limits prevent memory exhaustion

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for correct size and alignment
- **Range Validation:** RAIDA IDs, coin counts, and other parameters validated
- **Protocol Validation:** Network messages conform to expected formats

### Network Error Handling
- **Connection Failures:** Graceful handling of network connectivity issues
- **Timeout Management:** Proper timeout handling for distributed operations
- **Resource Cleanup:** Network resources freed on error conditions

### Consensus Error Handling
- **Insufficient Consensus:** Proper handling when majority consensus not reached
- **Ticket Expiration:** Cleanup of expired tickets and resources
- **Memory Management:** Proper allocation and deallocation of response buffers

## Performance Characteristics

### Distributed Operation Optimization
- **Parallel Processing:** Multiple RAIDA servers contacted simultaneously
- **Connection Pooling:** Efficient network resource management
- **Timeout Optimization:** Balanced timeouts for responsiveness vs. reliability

### Memory Management
- **Ticket Pool:** Fixed-size ticket pool prevents memory exhaustion
- **Dynamic Allocation:** Response buffers sized based on operation requirements
- **Resource Cleanup:** Automatic cleanup of expired resources

### Database Integration
- **On-Demand Cache:** Efficient use of database layer caching
- **Batch Operations:** Optimized for processing multiple coins
- **Lock Management:** Proper page locking ensures consistency

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Network Layer:** TCP/UDP communication with other RAIDA servers
- **Threading System:** Multi-threaded processing for distributed operations
- **Utilities Module:** Dual hash functions and cryptographic operations
- **Configuration Module:** RAIDA server addresses and network topology

### External Constants Required
- `TOTAL_RAIDA_SERVERS`: Network size for consensus calculations
- `RAIDA_SERVER_RCV_TIMEOUT`: Network timeout configuration
- `TICKET_POOL_SIZE`: Maximum number of concurrent tickets
- `TICKET_RELEASE_SECONDS`: Ticket expiration timeout

### Used By
- **Healing Workflows:** Primary interface for coin recovery operations
- **Network Synchronization:** Inter-RAIDA consensus and validation
- **Recovery Tools:** Automated and manual coin recovery systems
- **Diagnostic Tools:** Network health and consensus monitoring

### Cross-File Dependencies
- **Database Layer:** Page access, locking, and coin data retrieval
- **Network Layer:** Inter-RAIDA communication and protocol handling
- **Utilities Module:** Hash generation and cryptographic functions
- **Configuration Module:** RAIDA network topology and authentication
- **Protocol Layer:** Message formatting and validation

## Threading and Concurrency

### Multi-Threading Design
- **Worker Threads:** Parallel validation requests to multiple RAIDA servers
- **Thread Synchronization:** Proper coordination of distributed operations
- **Resource Safety:** Thread-safe access to shared ticket pool
- **Cleanup Management:** Proper resource cleanup across thread boundaries

### Ticket Pool Management
- **Concurrent Access:** Thread-safe ticket allocation and management
- **Expiration Handling:** Background cleanup of expired tickets
- **Resource Limits:** Prevention of resource exhaustion through pool limits

## Network Protocol Integration

### Inter-RAIDA Communication
- **Protocol Compliance:** Full compliance with RAIDA network protocol
- **Message Authentication:** Proper authentication of inter-server messages
- **Error Recovery:** Graceful handling of network and protocol errors
- **Compatibility:** Support for different RAIDA server versions

### Distributed Consensus
- **Byzantine Fault Tolerance:** Robust consensus despite server failures
- **Majority Voting:** Democratic decision-making across network
- **Quorum Requirements:** Minimum participation thresholds for valid consensus

This healing module provides essential network recovery and consensus functionality for the RAIDA network, enabling distributed coin recovery, network synchronization, and fault-tolerant operations while maintaining security and compatibility across diverse client and server versions.