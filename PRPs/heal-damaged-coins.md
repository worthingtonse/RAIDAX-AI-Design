# Healing Command Handlers (cmd_healing)

## Module Purpose
This module implements healing and recovery commands for distributed consensus and coin repair operations. It provides ticket-based authentication, coin finding, and fix operations with distributed validation across RAIDA servers. The module includes dual hashing support and free pages bitmap integration for modern system compatibility.

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

### 1. Get Ticket Command (`cmd_get_ticket`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Verifies coins and issues a ticket for authentic ones, enabling distributed healing operations across RAIDA servers.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (39 bytes)
   - Calculates coin count: (body_size - 18) / 21
   - Each coin requires: 1 byte denomination + 4 bytes SN + 16 bytes AN

2. **Response Buffer Allocation:**
   - Allocates bit-packed buffer: (total_coins / 8) + 1 + 4 bytes
   - Additional 4 bytes for ticket ID if any coins pass
   - Initializes buffer to zeros (fail by default)

3. **Coin Authentication:**
   - For each coin:
     - Loads page using get_page_by_sn_lock
     - Compares stored authentication number with provided value
     - If authentic:
       - Allocates ticket entry on first success using get_free_ticket_slot
       - Adds coin to ticket entry (up to MAX_COINS_PER_TICKET)
       - Sets success bit in response buffer

4. **Ticket Management:**
   - **Ticket Allocation:** Requests ticket slot from ticket pool
   - **Coin Association:** Associates authentic coins with ticket
   - **Resource Management:** Handles ticket slot exhaustion gracefully
   - **Thread Safety:** Uses proper locking for ticket operations

5. **Result Classification:**
   - **STATUS_ALL_PASS:** All coins authentic, returns ticket ID
   - **STATUS_ALL_FAIL:** No coins authentic, no ticket issued
   - **STATUS_MIXED:** Some authentic, returns bit-packed results + ticket ID

**Distributed Features:**
- **Cross-RAIDA Authentication:** Enables other RAIDA servers to validate ticket
- **Batch Processing:** Single ticket covers multiple authenticated coins
- **Resource Management:** Efficient ticket pool management
- **Concurrency Safe:** Thread-safe ticket allocation and management

**Used By:** Healing operations, distributed consensus, cross-RAIDA validation

**Dependencies:** Database layer, ticket management system, memory allocation

### 2. Validate Ticket Command (`cmd_validate_ticket`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Allows another RAIDA server to claim a ticket and retrieve associated coin information.

**Process:**
1. **Request Validation:**
   - Validates exact request size (23 bytes)
   - Extracts RAIDA index and ticket ID
   - Validates RAIDA index is within valid range (0 to TOTAL_RAIDA_SERVERS-1)

2. **Ticket Lookup:**
   - Searches ticket pool using get_ticket_entry
   - Returns error if ticket not found
   - Acquires exclusive lock on ticket entry

3. **Claim Verification:**
   - Checks if requesting RAIDA has already claimed ticket
   - Prevents double-claiming by same RAIDA server
   - Maintains claim state for all RAIDA servers

4. **Coin Data Response:**
   - Allocates response buffer: num_coins * 5 bytes
   - For each coin in ticket:
     - Returns denomination (1 byte) + serial number (4 bytes)
   - Provides complete coin identification for requester

5. **Claim Recording:**
   - Marks ticket as claimed by requesting RAIDA
   - Updates claim tracking for distributed consensus
   - Returns success status with coin data

**Inter-RAIDA Communication:**
- **Distributed Claims:** Tracks which RAIDA servers have claimed ticket
- **Consensus Building:** Enables distributed validation across network
- **Resource Sharing:** Allows efficient sharing of authentication data
- **Network Efficiency:** Reduces redundant authentication operations

**Used By:** Inter-RAIDA communication, distributed healing, consensus operations

**Dependencies:** Ticket management system, RAIDA network validation

### 3. Find Command (`cmd_find`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Checks if coins match either their current authentication number or a proposed authentication number, supporting healing operations.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (55 bytes)
   - Calculates coin count: (body_size - 18) / 37
   - Each coin requires: 1 byte den + 4 bytes SN + 16 bytes current AN + 16 bytes proposed AN

2. **Dual Authentication Check:**
   - For each coin:
     - Loads page using get_page_by_sn_lock
     - **Current AN Check:** Compares with stored authentication number
     - **Proposed AN Check:** Compares with proposed authentication number
     - Records result type in response buffer

3. **Result Encoding:**
   - **0x1:** Coin matches current authentication number
   - **0x2:** Coin matches proposed authentication number
   - **0x0:** Coin matches neither (failed)

4. **Result Classification:**
   - **STATUS_FIND_ALL_AN:** All coins match current authentication numbers
   - **STATUS_FIND_ALL_PAN:** All coins match proposed authentication numbers
   - **STATUS_FIND_NEITHER:** No coins match any authentication numbers
   - **STATUS_FIND_MIXED:** Mixed results, returns detailed response

**Healing Support:**
- **State Detection:** Identifies which coins need healing
- **Consensus Preparation:** Prepares data for distributed consensus
- **Authentication Verification:** Verifies coin ownership claims
- **Recovery Planning:** Identifies coins requiring recovery operations

**Used By:** Healing operations, coin recovery, consensus building

**Dependencies:** Database layer, authentication verification

### 4. Fix Command (`cmd_fix`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Fixes coins' authentication numbers based on distributed consensus from other RAIDA servers, implementing the core healing mechanism.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (139 bytes)
   - Calculates coin count: (body_size - 134) / 5
   - Extracts PG (proof GUID) and ticket array from other RAIDA servers

2. **Distributed Ticket Validation:**
   - **Parallel Processing:** Creates threads for each RAIDA server
   - **Ticket Requests:** Sends validate_ticket requests to all RAIDA servers
   - **Consensus Collection:** Collects coin validation data from network
   - **Thread Management:** Waits for all threads to complete

3. **Consensus Calculation:**
   - For each coin:
     - Counts how many RAIDA servers validated the coin
     - Requires majority consensus: > (TOTAL_RAIDA_SERVERS / 2) + 1
     - Only proceeds with fix if consensus achieved

4. **Coin Fixing Process:**
   - For coins with consensus:
     - **Dual Hashing Support:** Chooses hash algorithm based on client version
       - **Legacy (encryption_type < 4):** Uses generate_an_hash_legacy (MD5)
       - **Modern (encryption_type >= 4):** Uses generate_an_hash (SHA-256)
     - **Hash Input:** Combines RAIDA number, denomination, serial number, and PG
     - Updates coin with new authentication number
     - Sets MFS to current timestamp
     - Marks page as dirty for persistence
     - **Bitmap Update:** Marks coin as not free in bitmap

5. **Network Communication:**
   - **TCP Connections:** Establishes secure connections to peer RAIDA servers
   - **Protocol Compliance:** Uses proper RAIDA network protocol
   - **Timeout Handling:** Appropriate timeouts for network operations
   - **Error Recovery:** Handles network failures gracefully

**Distributed Consensus:**
- **Byzantine Fault Tolerance:** Works correctly despite server failures
- **Majority Rule:** Requires network majority for coin fixes
- **Cryptographic Security:** Strong hash generation prevents tampering
- **Network Coordination:** Coordinates healing across entire RAIDA network

**Used By:** Healing operations, network consensus, coin recovery

**Dependencies:** Database layer, network communication, threading system, consensus protocols

### 5. Send Validate Ticket Job (`send_validate_ticket_job`)
**Parameters:**
- Thread argument containing RAIDA index, ticket ID, and connection info

**Returns:** Thread result with coin validation data

**Purpose:** Worker thread function that communicates with other RAIDA servers to validate tickets in distributed healing operations.

**Process:**
1. **Network Connection:**
   - Creates TCP socket for reliable communication
   - Sets receive timeout for responsive operations
   - Connects to target RAIDA server using configuration

2. **Protocol Message Construction:**
   - **Header Construction:** Builds proper RAIDA protocol header
   - **Challenge Generation:** Creates authentication challenge
   - **CRC Validation:** Calculates CRC32 for data integrity
   - **Body Assembly:** Assembles RAIDA index and ticket ID

3. **Request Transmission:**
   - Sends complete protocol message to peer RAIDA
   - Handles transmission errors gracefully
   - Maintains proper protocol compliance

4. **Response Processing:**
   - Receives response header and validates status
   - Extracts response body size from header
   - Receives complete coin validation data
   - Parses coin list from response

5. **Result Preparation:**
   - Allocates memory for coin array
   - Extracts denomination and serial number for each coin
   - Returns validated coin list to main healing thread

**Network Protocol:**
- **RAIDA Compliance:** Follows standard RAIDA inter-server protocol
- **Security Features:** Challenge-response authentication
- **Error Handling:** Comprehensive error detection and recovery
- **Resource Management:** Proper cleanup of network resources

**Used By:** Fix command threading, distributed validation, network healing

**Dependencies:** Network sockets, RAIDA protocol, memory management

## Ticket Management System

### Ticket Pool Architecture
- **Resource Pool:** Fixed-size pool of ticket entries
- **Thread Safety:** Mutex-protected allocation and deallocation
- **Efficient Allocation:** Fast ticket slot allocation
- **Automatic Cleanup:** Expired tickets automatically cleaned up

### Ticket Lifecycle
- **Allocation:** Tickets allocated on first authentic coin
- **Population:** Coins added to ticket as authentication succeeds
- **Distribution:** Tickets distributed to requesting RAIDA servers
- **Expiration:** Tickets expire after configured timeout
- **Cleanup:** Automatic cleanup of expired resources

### Distributed Claims
- **Claim Tracking:** Tracks which RAIDA servers have claimed tickets
- **Prevent Duplication:** Prevents duplicate claims by same server
- **Consensus Support:** Enables distributed consensus operations
- **Resource Sharing:** Efficient sharing across RAIDA network

## Dual Hashing Integration

### Algorithm Selection
- **Client-Driven:** Hash algorithm based on client encryption type
- **Legacy Support:** MD5 hashing for older clients
- **Modern Security:** SHA-256 hashing for newer clients
- **Seamless Integration:** Transparent algorithm selection

### Hash Input Construction
- **Deterministic Generation:** Reproducible hash inputs
- **Network Consensus:** Same hash inputs across RAIDA servers
- **Cryptographic Security:** Strong randomness and uniqueness
- **Protocol Compliance:** Compatible with healing protocols

## Free Pages Bitmap Integration

### Real-Time Updates
- **Immediate Synchronization:** Bitmap updated with coin fixes
- **Consistency Guarantee:** Perfect sync between coin data and bitmap
- **Atomic Operations:** Coin and bitmap updates are atomic
- **Recovery Safety:** System remains consistent during failures

### Performance Benefits
- **Instant Availability:** Fixed coins immediately available in queries
- **Memory Efficiency:** Minimal overhead for maximum performance
- **Scalable Operations:** Performance independent of coin count
- **Cache Optimization:** Reduces database cache pressure

## Network Communication

### Inter-RAIDA Protocol
- **Standard Compliance:** Follows RAIDA network protocols
- **Secure Communication:** Challenge-response authentication
- **Reliable Transport:** TCP for guaranteed delivery
- **Error Recovery:** Comprehensive error handling

### Threading Architecture
- **Parallel Processing:** Simultaneous communication with all RAIDA servers
- **Thread Safety:** Safe concurrent network operations
- **Resource Management:** Proper thread lifecycle management
- **Performance Optimization:** Minimizes healing operation time

### Timeout Management
- **Responsive Operations:** Appropriate timeouts for network calls
- **Failure Detection:** Quick detection of non-responsive servers
- **Graceful Degradation:** Continues operation despite failures
- **Network Resilience:** Robust operation in unreliable network conditions

## Error Handling and Recovery

### Network Error Handling
- **Connection Failures:** Graceful handling of network issues
- **Timeout Recovery:** Proper handling of network timeouts
- **Protocol Errors:** Recovery from protocol violations
- **Resource Cleanup:** Comprehensive cleanup on failures

### Consensus Error Handling
- **Insufficient Consensus:** Handles cases with inadequate network agreement
- **Conflicting Data:** Resolution of conflicting validation results
- **Byzantine Failures:** Tolerance of arbitrary server failures
- **Recovery Procedures:** Clear procedures for consensus failures

### Data Integrity
- **Validation Verification:** Comprehensive validation of received data
- **Checksum Verification:** CRC validation for data integrity
- **Protocol Compliance:** Strict adherence to protocol specifications
- **State Consistency:** Maintains consistent state across operations

## Dependencies and Integration

### Required Modules
- **Database Layer:** Coin data access and modification
- **Network Layer:** Inter-RAIDA communication
- **Threading System:** Parallel processing capabilities
- **Ticket Management:** Distributed authentication tickets
- **Cryptographic Functions:** Hash generation for healing

### Used By
- **Healing Operations:** Primary interface for coin recovery
- **Network Consensus:** Distributed agreement mechanisms
- **Recovery Procedures:** System recovery and repair operations
- **Administrative Tools:** Healing management and monitoring

### Cross-File Dependencies
- **Database Module:** Page access and coin data management
- **Network Module:** RAIDA server communication
- **Utilities Module:** Cryptographic functions and data conversion
- **Configuration Module:** RAIDA server addresses and network settings
- **Ticket Module:** Distributed authentication ticket management

This healing command module provides comprehensive distributed healing and recovery capabilities with advanced consensus mechanisms, dual hashing support, network communication protocols, and bitmap integration for maintaining system integrity across the entire RAIDA network.