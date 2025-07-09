# Language-Agnostic Specification: Healing Commands Implementation

## 1. Module Purpose
This module implements the healing commands for the CloudCoin RAIDA system. It provides coin recovery and authentication verification through a distributed ticket-based system that enables cross-RAIDA server validation for damaged or lost coin authentication data.

## 2. System Architecture Overview

### 2.1 Core Components
- **Ticket-Based Recovery**: Memory pool of tickets for multi-phase healing operations
- **Distributed Validation**: Cross-RAIDA server communication for consensus
- **Fine-Grained Locking**: Per-ticket mutex system for concurrent access
- **Multi-Threaded Processing**: Parallel validation requests to RAIDA servers
- **Consensus Algorithm**: Majority vote system for coin authenticity verification

### 2.2 Healing Process Flow
- **Ticket Creation**: Initial authentication and ticket allocation
- **Ticket Validation**: Cross-server verification and claim tracking
- **Coin Discovery**: Find current vs proposed authentication states
- **Coin Recovery**: Consensus-based authentication number restoration

## 3. System Constants and Data Structures

### 3.1 Ticket System Constants
```
TICKET_POOL_SIZE = 512              // Maximum concurrent tickets
MAX_COINS_PER_TICKET = 4096         // Maximum coins per ticket
TICKET_RELEASE_SECONDS = 300        // Ticket expiration time (5 minutes)
TOTAL_RAIDA_SERVERS = 25           // Number of RAIDA servers
RAIDA_SERVER_RCV_TIMEOUT = 32      // Network timeout in seconds
```

### 3.2 Enhanced Ticket Entry Structure
```
ticket_entry_structure:
    created_at: timestamp_type              // Creation timestamp
    ticket: unsigned_32bit_integer          // Unique ticket identifier
    coins: array[MAX_COINS_PER_TICKET] of coin_structure
    claims: array[TOTAL_RAIDA_SERVERS] of character    // Claim tracking by RAIDA
    num_coins: unsigned_32bit_integer       // Number of coins in ticket
    mtx: mutex_type                         // Per-ticket fine-grained mutex
```

### 3.3 Validation Thread Arguments
```
validate_ticket_arg_structure:
    raida_idx: integer                      // Target RAIDA server index
    ticket: unsigned_32bit_integer          // Ticket to validate
    ci: pointer_to_connection_info          // Original connection context
    rv_coins: pointer_to_coin_array         // Returned coin data
    rv_num_coins: integer                   // Number of returned coins
```

### 3.4 Coin Counter Structure
```
coin_counter_structure:
    coin: coin_structure                    // Coin identification
    cnt: integer                           // Vote count from RAIDA servers
```

## 4. Core Command Implementations

### 4.1. cmd_get_ticket
**Purpose**: Creates healing ticket by validating coins and allocating ticket entry.

**Request Format**:
```
[16 bytes Challenge Header]
[21 bytes per coin: DN + 4-byte SN + 16-byte AN]
[2 bytes EOF trailer]
Minimum size: 39 bytes (16 + 21 + 2)
```

**Validation Steps**:
- Verify request size >= 39 bytes
- Calculate coin count: (body_size - 18) / 21
- Verify coin data length is divisible by 21

**Processing Logic**:
1. **Memory Allocation**:
   - Allocate response bitmap: (total_coins / 8) + 1 + 4 bytes
   - Additional 4 bytes for ticket number storage

2. **Coin Authentication**:
   - For each coin:
     - Extract denomination and serial number
     - Lock database page containing coin
     - Compare stored AN with provided AN
     - If authentic and no ticket allocated yet:
       - Call get_free_ticket_slot() (returns locked ticket)
       - Add coin to ticket entry
       - Set success bit in bitmap
     - Unlock page

3. **Response Generation**:
   - All authentic: STATUS_ALL_PASS, return ticket number (4 bytes)
   - All failed: STATUS_ALL_FAIL, no ticket created
   - Mixed results: STATUS_MIXED, return bitmap + ticket number

4. **Resource Management**:
   - Unlock ticket entry after all operations
   - Track pass/fail counts for status determination

### 4.2. cmd_validate_ticket
**Purpose**: Validates ticket authenticity and marks claims by RAIDA servers.

**Request Format**:
```
[16 bytes Challenge Header]
[1 byte RAIDA Index]
[4 bytes Ticket Number]
[2 bytes EOF trailer]
Total size: exactly 23 bytes
```

**Validation Steps**:
- Verify request size == 23 bytes
- Validate RAIDA index < TOTAL_RAIDA_SERVERS
- Extract ticket number from request

**Processing Logic**:
1. **Ticket Retrieval**:
   - Call get_ticket_entry(ticket_number) (returns locked ticket)
   - If not found: return ERROR_NO_TICKET_FOUND

2. **Claim Validation**:
   - Check if RAIDA has already claimed ticket
   - If already claimed: return ERROR_TICKET_CLAIMED_ALREADY

3. **Response Generation**:
   - Allocate response buffer: num_coins * 5 bytes
   - For each coin in ticket:
     - Write denomination (1 byte)
     - Write serial number (4 bytes, big-endian format)

4. **Claim Recording**:
   - Set claims[raida_index] = 1
   - Return STATUS_SUCCESS

5. **Resource Management**:
   - Unlock ticket entry after all operations (including error paths)

### 4.3. cmd_find
**Purpose**: Determines coin authentication state by comparing current and proposed ANs.

**Request Format**:
```
[16 bytes Challenge Header]
[37 bytes per coin: DN + 4-byte SN + 16-byte AN + 16-byte PN]
[2 bytes EOF trailer]
Minimum size: 55 bytes (16 + 37 + 2)
```

**Validation Steps**:
- Verify request size >= 55 bytes
- Calculate coin count: (body_size - 18) / 37
- Verify coin data length is divisible by 37

**Processing Logic**:
1. **Memory Allocation**:
   - Allocate response buffer: total_coins bytes (1 byte per coin result)

2. **Coin State Analysis**:
   - For each coin:
     - Extract denomination, serial number, current AN, proposed AN
     - Lock database page containing coin
     - Compare stored AN with current AN:
       - If match: Set response[i] = 0x1 (AN matches)
     - Compare stored AN with proposed AN:
       - If match: Set response[i] = 0x2 (PAN matches)
     - If neither: Set response[i] = 0x0 (neither matches)
     - Unlock page

3. **Status Determination**:
   - All AN matches: STATUS_FIND_ALL_AN
   - All PAN matches: STATUS_FIND_ALL_PAN  
   - All neither: STATUS_FIND_NEITHER
   - Mixed results: STATUS_FIND_MIXED (include response array)

### 4.4. cmd_fix
**Purpose**: Performs distributed coin recovery using cross-RAIDA consensus.

**Request Format**:
```
[16 bytes Challenge Header]
[5 bytes per coin: DN + 4-byte SN]
[16 bytes PG (Proposed Group identifier)]
[25 * 4 bytes: Ticket numbers from each RAIDA]
[2 bytes EOF trailer]
Minimum size: 139 bytes (16 + 5 + 16 + 100 + 2)
```

**Validation Steps**:
- Verify request size >= 139 bytes
- Calculate coin count: (body_size - 134) / 5
- Verify coin data length is divisible by 5

**Complex Processing Logic**:

1. **Initialization Phase**:
   - Allocate response bitmap: (total_coins / 8) + 1 bytes
   - Allocate coin counters array for vote tracking
   - Initialize coin counter structures with coin data

2. **Distributed Validation Phase**:
   - For each RAIDA server (0 to TOTAL_RAIDA_SERVERS-1):
     - Extract ticket number for this RAIDA
     - Create thread argument structure
     - Launch thread: send_validate_ticket_job()

3. **Thread Coordination**:
   - Wait for all threads to complete using pthread_join
   - Process results from each RAIDA server
   - Match returned coins with requested coins
   - Increment vote counters for validated coins

4. **Consensus Processing**:
   - For each coin:
     - Check if votes > (TOTAL_RAIDA_SERVERS / 2) + 1 (majority)
     - If majority reached:
       - Calculate new AN using MD5 hash
       - Input: RAIDA_number + denomination + SN + PG (22 bytes total)
       - Lock database page
       - Update stored AN with calculated value
       - Set MFS byte
       - Add page to dirty queue
       - Set success bit in response bitmap
       - Unlock page

5. **Response Generation**:
   - All successful: STATUS_ALL_PASS
   - All failed: STATUS_ALL_FAIL
   - Mixed: STATUS_MIXED (include bitmap)

## 5. Network Communication Functions

### 5.1. send_validate_ticket_job(thread_argument)
**Purpose**: Thread function for cross-RAIDA ticket validation.

**Thread Execution Flow**:

1. **Connection Setup**:
   - Extract target RAIDA host and port from configuration
   - Create TCP socket with non-blocking mode
   - Set receive timeout to RAIDA_SERVER_RCV_TIMEOUT
   - Establish connection using select() for timeout handling

2. **Request Construction**:
   - Build 32-byte request header:
     - Router version, split ID, target RAIDA ID
     - Command group 2, command 50 (validate_ticket)
     - Coin ID from original connection
     - Body length: 16 + 1 + 4 + 2 = 23 bytes
   - Build request body:
     - 16-byte challenge (sequential bytes 0-11 + CRC32)
     - 1-byte source RAIDA number
     - 4-byte ticket number
     - 2-byte trailer (0x3e, 0x3e)

3. **Response Processing**:
   - Read 32-byte response header
   - Validate RAIDA ID and status code
   - Extract body size from header
   - Read response body containing coin data
   - Validate trailer bytes
   - Parse coin data: denomination + serial number per 5-byte record

4. **Result Storage**:
   - Allocate array for returned coins
   - Store coin data in thread argument structure
   - Validate each coin against database for existence

5. **Error Handling**:
   - Network timeouts and connection failures
   - Invalid response format handling
   - Resource cleanup on all error paths

## 6. Enhanced Ticket Management

### 6.1 Fine-Grained Locking System
- **Per-Ticket Mutexes**: Each ticket entry has individual mutex
- **Lock Acquisition**: get_free_ticket_slot() returns locked ticket
- **Lock Management**: Caller responsible for unlocking via unlock_ticket_entry()
- **Non-Blocking Access**: get_free_ticket_slot() uses trylock for efficiency

### 6.2 Ticket Lifecycle Management

#### 6.2.1 init_ticket_storage()
- Initialize each ticket entry with creation timestamp 0
- Initialize per-ticket mutex for each entry
- Return error if any mutex initialization fails

#### 6.2.2 check_tickets()
- Iterate through all ticket entries with individual locking
- Lock each ticket, check expiration, unlock immediately
- Free expired tickets by resetting creation timestamp

#### 6.2.3 get_free_ticket_slot()
- Use pthread_mutex_trylock() to avoid blocking
- Find first available slot (creation timestamp 0)
- Initialize ticket with random number and current timestamp
- Return locked ticket entry to caller

#### 6.2.4 get_ticket_entry(ticket_number)
- Search all tickets for matching ticket number
- Lock and return matching ticket entry
- Caller must unlock after use

## 7. Database Integration Requirements

### 7.1 Page Management Protocol
For each database access:
1. **Page Retrieval**: get_page_by_sn_lock(denomination, serial_number)
2. **Data Access**: Calculate record offset: (sn % RECORDS_PER_PAGE) * 17
3. **AN Comparison**: Compare bytes 0-15 of record with provided AN
4. **Modification**: Update AN and MFS byte for successful fixes
5. **Persistence**: add_page_to_dirty_queue(page) for modified pages
6. **Release**: unlock_page(page)

### 7.2 MD5 Hash Calculation for Fix Command
- Input data: RAIDA_number (1) + denomination (1) + SN (4) + PG (16) = 22 bytes
- Use md5ilen() function for hash calculation
- Replace stored AN with calculated MD5 hash
- Set MFS byte to current modification sequence

## 8. Error Handling and Recovery

### 8.1 Network Error Management
- Connection timeouts handled with select() and timeout values
- Socket errors logged with specific RAIDA server context
- Failed RAIDA connections continue processing with remaining servers
- Thread synchronization ensures all threads complete before processing

### 8.2 Resource Management
- All allocated memory freed on error paths
- Ticket entries unlocked on all exit conditions
- Thread cleanup handled by pthread_join operations
- Socket resources properly closed on all error conditions

### 8.3 Consensus Validation
- Majority vote required for coin recovery (> 50% + 1 RAIDA servers)
- Individual coin failures don't affect other coins in batch
- Partial success tracked with bitmap responses
- Invalid responses from RAIDA servers logged but don't block processing

## 9. Performance Considerations

### 9.1 Concurrency Optimizations
- Fine-grained per-ticket locking reduces contention
- Non-blocking ticket allocation with trylock
- Parallel RAIDA server communication reduces latency
- Individual page locking minimizes database contention

### 9.2 Memory Efficiency
- Response buffers sized based on actual coin counts
- Thread-local storage for network communication
- Temporary arrays freed immediately after use
- Bitmap responses minimize network overhead

### 9.3 Network Optimization
- Parallel thread execution for RAIDA communication
- Connection timeouts prevent hanging operations
- Non-blocking socket operations during connection phase
- Efficient request/response format minimizes bandwidth

This specification provides complete implementation guidance for the CloudCoin healing command system while remaining language-agnostic and accurately reflecting the actual implementation requirements.