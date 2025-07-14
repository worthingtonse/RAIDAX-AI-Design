# Healing Command Handlers (cmd_healing.c)

## Module Purpose
This module implements the healing and recovery operations for the RAIDA network, providing mechanisms to recover coins that may have been lost or corrupted due to network issues, server failures, or synchronization problems. It includes ticket-based authentication, coin discovery, and distributed consensus-based recovery operations.

## Core Functionality

### 1. Ticket Acquisition (`cmd_get_ticket`)
**Parameters:**
- Connection information structure containing request data
- Input: Variable-length payload (minimum 39 bytes) with coin authentication data

**Returns:** None (modifies connection structure with ticket information and authentication results)

**Purpose:** Verifies coin authenticity and issues a ticket for authentic coins, enabling subsequent healing operations across the RAIDA network.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (39 bytes)
   - Calculates coin count from payload size (21 bytes per coin record)
   - Validates coin data alignment and format

2. **Coin Authentication:**
   - For each coin in the request:
     - Extracts denomination, serial number, and authentication number
     - Retrieves coin page using on-demand cache system
     - Compares provided authentication number with stored value
     - Tracks authentic versus failed coins

3. **Ticket Allocation:**
   - Allocates free ticket slot from ticket memory pool
   - Stores authenticated coins in ticket structure
   - Generates unique ticket identifier
   - Initializes claim tracking for all RAIDA servers

4. **Response Generation:**
   - Creates bitmap indicating coin authentication results
   - Includes ticket identifier for successful operations
   - Provides status based on authentication results:
     - **STATUS_ALL_PASS:** All coins authentic, ticket issued
     - **STATUS_ALL_FAIL:** No coins authentic, no ticket
     - **STATUS_MIXED:** Partial success, ticket issued with bitmap

**Security Features:**
- Authentication number verification before ticket issuance
- Ticket-based authorization for subsequent healing operations
- Unique ticket identifiers prevent replay attacks
- Time-limited ticket validity

**Input Format:**
- Variable: Coin authentication records (21 bytes each: 1 byte denomination + 4 bytes serial + 16 bytes authentication)
- 2 bytes: End-of-frame marker

**Output Format:**
- Variable: Authentication result bitmap (1 bit per coin)
- 4 bytes: Ticket identifier (for successful operations)

### 2. Ticket Validation (`cmd_validate_ticket`)
**Parameters:**
- Connection information structure
- Input: 23-byte payload with RAIDA server ID and ticket identifier

**Returns:** None (modifies connection structure with ticket coin information)

**Purpose:** Allows other RAIDA servers to claim and validate tickets, enabling distributed healing operations across the network.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (23 bytes)
   - Extracts requesting RAIDA server ID and ticket identifier
   - Validates RAIDA server ID within acceptable range

2. **Ticket Verification:**
   - Locates ticket entry using ticket identifier
   - Verifies ticket exists and is valid
   - Checks if ticket has already been claimed by requesting server
   - Prevents duplicate claims from same server

3. **Ticket Information Retrieval:**
   - Retrieves coin list from ticket structure
   - Prepares response with coin denomination and serial number data
   - Marks ticket as claimed by requesting server
   - Maintains claim tracking for audit purposes

4. **Response Generation:**
   - Returns list of coins associated with ticket
   - Provides coin identification information for healing operations
   - Updates ticket claim status

**Security Features:**
- Server identity verification
- Duplicate claim prevention
- Ticket validity checking
- Audit trail through claim tracking

**Input Format:**
- 1 byte: Requesting RAIDA server ID
- 4 bytes: Ticket identifier
- 18 bytes: Reserved/padding

**Output Format:**
- Variable: Coin list (5 bytes each: 1 byte denomination + 4 bytes serial number)

### 3. Coin Discovery (`cmd_find`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) with coin authentication data

**Returns:** None (modifies connection structure with discovery results)

**Purpose:** Checks if coins match either current authentication numbers or proposed new authentication numbers, enabling coin location during healing operations.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (55 bytes)
   - Calculates coin count from payload size (37 bytes per coin record)
   - Validates coin data alignment and format

2. **Dual Authentication Check:**
   - For each coin in the request:
     - Extracts denomination, serial number, current AN, and proposed AN
     - Retrieves coin page using on-demand cache system
     - Compares stored authentication number with current AN
     - If no match, compares stored authentication number with proposed AN
     - Records match type for response

3. **Result Classification:**
   - Tracks coins matching current authentication numbers
   - Tracks coins matching proposed authentication numbers
   - Counts coins with no matches
   - Generates detailed response for mixed results

4. **Response Generation:**
   - **STATUS_FIND_ALL_AN:** All coins match current authentication numbers
   - **STATUS_FIND_ALL_PAN:** All coins match proposed authentication numbers
   - **STATUS_FIND_NEITHER:** No coins match either authentication number
   - **STATUS_FIND_MIXED:** Mixed results with detailed per-coin response

**Discovery Features:**
- Dual authentication number checking
- Detailed match type reporting
- Batch processing for multiple coins
- Efficient page access through caching

**Input Format:**
- Variable: Coin discovery records (37 bytes each: 1 byte denomination + 4 bytes serial + 16 bytes current AN + 16 bytes proposed AN)
- 2 bytes: End-of-frame marker

**Output Format:**
- Variable: Match type indicators (1 byte each: 0x1 for current AN, 0x2 for proposed AN, 0x0 for no match)

### 4. Coin Recovery (`cmd_fix`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 139 bytes) with coin list and consensus data

**Returns:** None (modifies connection structure with recovery results)

**Purpose:** Recovers coins using distributed consensus from other RAIDA servers, updating authentication numbers based on majority agreement.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (139 bytes)
   - Calculates coin count from payload size (5 bytes per coin)
   - Extracts consensus data and ticket information

2. **Distributed Consensus Collection:**
   - Launches parallel threads to contact other RAIDA servers
   - Sends ticket validation requests to all servers
   - Collects responses with coin information
   - Maintains timeout and error handling for network operations

3. **Consensus Analysis:**
   - Counts votes for each coin from responding servers
   - Requires majority consensus (more than half of servers)
   - Validates coin existence across the network
   - Determines which coins have sufficient consensus

4. **Authentication Number Update:**
   - For coins with sufficient consensus:
     - Generates new authentication number using cryptographic hash
     - Updates coin record with new authentication number
     - Sets MFS timestamp to current month
     - Marks database page as dirty for persistence

5. **Response Generation:**
   - Creates bitmap indicating successful recovery for each coin
   - Provides status based on recovery results:
     - **STATUS_ALL_PASS:** All coins successfully recovered
     - **STATUS_ALL_FAIL:** No coins had sufficient consensus
     - **STATUS_MIXED:** Partial recovery with detailed bitmap

**Consensus Features:**
- Distributed consensus across RAIDA network
- Majority vote requirements for coin recovery
- Parallel server communication for efficiency
- Cryptographically secure authentication number generation

**Input Format:**
- Variable: Coin identifiers (5 bytes each: 1 byte denomination + 4 bytes serial number)
- 16 bytes: Consensus data (PG - proposed guidance)
- 100 bytes: Ticket identifiers from other servers (4 bytes each × 25 servers)
- 2 bytes: End-of-frame marker

**Output Format:**
- Variable: Recovery result bitmap (1 bit per coin)

### 5. Network Ticket Validation Thread (`send_validate_ticket_job`)
**Parameters:**
- Thread argument structure containing RAIDA server index, ticket ID, and response storage

**Returns:** Thread result (modifies argument structure with response data)

**Purpose:** Executes in separate thread to validate tickets with other RAIDA servers, enabling parallel consensus collection for healing operations.

**Process:**
1. **Network Connection:**
   - Establishes TCP connection to specified RAIDA server
   - Applies receive timeout for network reliability
   - Handles connection failures gracefully

2. **Protocol Communication:**
   - Constructs validate_ticket command packet
   - Includes challenge/response authentication
   - Sends request with proper formatting and trailers

3. **Response Processing:**
   - Receives and validates response header
   - Extracts coin information from response body
   - Parses coin list into structured format
   - Stores results in thread argument structure

4. **Error Handling:**
   - Handles network timeouts and connection failures
   - Validates response format and content
   - Provides graceful degradation for failed servers

**Threading Features:**
- Parallel execution for multiple server contacts
- Thread-safe result storage
- Network timeout handling
- Graceful failure recovery

## Data Structures and Formats

### Ticket Management
- **Ticket Pool:** Fixed-size pool of ticket entries with per-slot locking
- **Ticket Entry:** Contains creation time, unique ID, coin list, and claim tracking
- **Claim Tracking:** Bitmap indicating which servers have claimed the ticket
- **Ticket Timeout:** Automatic expiration after configured time period

### Coin Record Formats
- **Authentication Record:** 1 byte denomination + 4 bytes serial + 16 bytes authentication
- **Discovery Record:** 1 byte denomination + 4 bytes serial + 16 bytes current AN + 16 bytes proposed AN
- **Recovery Record:** 1 byte denomination + 4 bytes serial number
- **Consensus Data:** 16-byte proposed guidance for authentication number generation

### Response Formats
- **Ticket Response:** Authentication bitmap + ticket identifier
- **Validation Response:** Coin list with denomination and serial number data
- **Discovery Response:** Match type indicators for each coin
- **Recovery Response:** Success/failure bitmap for coin recovery operations

### Network Communication
- **Command Packets:** Standardized request format with headers and trailers
- **Response Packets:** Standardized response format with status and data
- **Threading Arguments:** Structure for passing data between threads
- **Timeout Handling:** Configurable timeouts for network operations

## Security Considerations

### Authentication Security
- **Ticket-Based Authorization:** Tickets required for healing operations
- **Consensus Verification:** Majority vote required for coin recovery
- **Cryptographic Authentication:** Secure authentication number generation
- **Time-Limited Access:** Tickets expire after configured timeout

### Data Integrity
- **Page Locking:** Database page locking ensures consistent coin state
- **MFS Timestamps:** All coin changes timestamped for audit trail
- **Dirty Page Tracking:** Modified data marked for reliable persistence
- **Atomic Operations:** Recovery operations either succeed completely or fail entirely

### Network Security
- **Server Identity Verification:** RAIDA server IDs validated
- **Duplicate Claim Prevention:** Tickets can only be claimed once per server
- **Timeout Protection:** Network operations bounded by timeouts
- **Error Handling:** Graceful degradation for failed servers

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for expected length
- **Alignment Validation:** Coin data properly aligned to record boundaries
- **Server ID Validation:** RAIDA server IDs within acceptable range
- **Ticket Validation:** Ticket identifiers validated against active tickets

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_COINS_NOT_DIV`: Coin data not properly aligned
- `ERROR_WRONG_RAIDA`: Invalid RAIDA server ID
- `ERROR_NO_TICKET_SLOT`: No available ticket slots
- `ERROR_NO_TICKET_FOUND`: Ticket not found or expired
- `ERROR_TICKET_CLAIMED_ALREADY`: Ticket already claimed by server
- `ERROR_MEMORY_ALLOC`: Memory allocation failure

### Recovery Mechanisms
- **Ticket Cleanup:** Expired tickets automatically released
- **Resource Cleanup:** Memory and page locks released on error conditions
- **Network Resilience:** Failed server connections handled gracefully
- **State Consistency:** Failed operations leave coin state unchanged

## Performance Characteristics

### Ticket Management
- **Fine-Grained Locking:** Per-slot mutexes for concurrent access
- **Non-Blocking Access:** Trylock operations prevent blocking
- **Automatic Cleanup:** Expired tickets released automatically
- **Memory Pool:** Fixed-size pool prevents memory fragmentation

### Network Operations
- **Parallel Processing:** Multiple server contacts execute simultaneously
- **Timeout Management:** Bounded execution time for network operations
- **Connection Reuse:** Efficient TCP connection management
- **Error Recovery:** Graceful handling of network failures

### Consensus Operations
- **Majority Voting:** Efficient consensus determination
- **Batch Processing:** Multiple coins processed in single operations
- **Cache Integration:** Efficient page access through on-demand cache
- **Resource Management:** Proper cleanup ensures no resource leaks

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Network Layer:** TCP connection management and communication
- **Threading System:** Parallel thread execution for consensus operations
- **Cryptographic Utilities:** Secure authentication number generation
- **Configuration System:** Server identification and network parameters

### External Constants Required
- `TOTAL_RAIDA_SERVERS`: Total number of servers in network
- `TICKET_POOL_SIZE`: Maximum number of concurrent tickets
- `MAX_COINS_PER_TICKET`: Maximum coins per ticket
- `TICKET_RELEASE_SECONDS`: Ticket expiration timeout
- `RAIDA_SERVER_RCV_TIMEOUT`: Network receive timeout
- `STATUS_*`: Operation result status codes
- `ERROR_*`: Error condition definitions

### Used By
- **Client Applications:** Primary interface for coin recovery operations
- **Network Services:** Inter-server communication for healing
- **Administrative Tools:** Coin recovery and network maintenance
- **Monitoring Systems:** Ticket usage and healing operation tracking

## Threading and Concurrency
- **Ticket Locking:** Per-slot mutexes for concurrent ticket access
- **Page Locking:** Thread-safe access to coin data through database layer
- **Network Threads:** Parallel execution for server communication
- **Resource Safety:** Proper cleanup ensures no resource leaks

## Network Protocol
- **Command Format:** Standardized request/response protocol
- **Challenge/Response:** Authentication mechanism for server communication
- **Timeout Handling:** Configurable timeouts for network reliability
- **Error Recovery:** Graceful handling of communication failures

## Healing Process Flow
1. **Ticket Acquisition:** Client obtains ticket for authentic coins
2. **Distributed Validation:** Other servers validate ticket claims
3. **Consensus Collection:** Parallel collection of server responses
4. **Majority Determination:** Analysis of consensus for each coin
5. **Recovery Execution:** Update authentication numbers based on consensus
6. **Result Reporting:** Detailed results returned to client

This healing command module provides essential recovery capabilities for the RAIDA network, enabling distributed consensus-based coin recovery while maintaining cryptographic security and network reliability.