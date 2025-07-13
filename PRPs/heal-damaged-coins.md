# Healing Commands Implementation (cmd_healing)

## Module Purpose
This module implements the network healing system for the RAIDA network, providing coin recovery, ticket-based coordination, and distributed consensus mechanisms. It enables automatic recovery of coins that may have inconsistent state across RAIDA servers through collaborative verification and repair operations with enhanced concurrency through fine-grained ticket locking.

## Core Functionality

### 1. Ticket Issuance (`cmd_get_ticket`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 39 bytes) containing coin authentication data

**Returns:** None (modifies connection structure with ticket information and authentication results)

**Purpose:** Verifies coin authenticity and issues healing tickets for authentic coins that can be used in distributed healing operations.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (39 bytes)
   - Calculates coin count from payload size (21 bytes per coin record)
   - Validates coin data alignment to prevent parsing errors

2. **Coin Authentication:**
   - For each coin in the request:
     - Extracts denomination and serial number (5 bytes)
     - Retrieves coin page using on-demand cache system
     - Compares provided authentication number (16 bytes) with stored value
     - Builds bitmap of authenticated coins

3. **Enhanced Ticket Management:**
   - **OPTIMIZATION:** Uses get_free_ticket_slot() which returns pre-locked ticket
   - Allocates ticket only when first authentic coin found
   - Populates ticket with all authenticated coins up to MAX_COINS_PER_TICKET
   - **NEW:** Uses unlock_ticket_entry() for proper fine-grained unlocking

4. **Response Generation:**
   - **STATUS_ALL_PASS:** All coins authentic, returns 4-byte ticket ID
   - **STATUS_ALL_FAIL:** No coins authentic, no ticket issued
   - **STATUS_MIXED:** Partial authentication with bitmap + ticket ID

**Concurrency Enhancement:**
- Per-ticket locking eliminates global ticket pool contention
- Non-blocking ticket allocation improves performance under load
- Proper lock management prevents deadlocks and resource leaks

### 2. Ticket Validation (`cmd_validate_ticket`)
**Parameters:**
- Connection information structure
- Input: 23-byte payload containing RAIDA ID and ticket number

**Returns:** None (modifies connection structure with ticket coin list)

**Purpose:** Allows other RAIDA servers to claim and validate healing tickets for distributed consensus operations.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (23 bytes)
   - Extracts claiming RAIDA ID and validates range (0-24)
   - Extracts ticket number for lookup

2. **Enhanced Ticket Access:**
   - **OPTIMIZATION:** get_ticket_entry() now returns locked ticket entry
   - Validates ticket exists and is active
   - Checks if ticket already claimed by requesting RAIDA
   - **NEW:** Must call unlock_ticket_entry() even on error conditions

3. **Ticket Processing:**
   - Allocates response buffer for coin list (5 bytes per coin)
   - Copies all coins from ticket to response buffer
   - Marks ticket as claimed by requesting RAIDA server
   - Sets success status for valid ticket claim

4. **Concurrency Improvements:**
   - **Fine-grained locking:** Only locks specific ticket, not entire pool
   - **Automatic unlocking:** Proper unlock on all code paths
   - **Thread safety:** Safe concurrent access to different tickets

**Inter-Server Coordination:**
- Enables distributed healing consensus across RAIDA network
- Prevents double-claiming through per-RAIDA tracking
- Supports complex multi-server healing operations

### 3. Coin Discovery (`cmd_find`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) containing coin data with current and proposed authentication numbers

**Returns:** None (modifies connection structure with discovery results)

**Purpose:** Searches for coins and determines whether they match current authentication numbers, proposed authentication numbers, or neither.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (55 bytes)
   - Validates coin data alignment (37 bytes per coin record)
   - Each record contains denomination + serial + current AN + proposed AN

2. **Coin Discovery Logic:**
   - For each coin specification:
     - Retrieves coin data using on-demand cache
     - Compares stored authentication number with current AN
     - If no match, compares with proposed AN
     - Records match type for each coin

3. **Match Classification:**
   - **0x1:** Coin matches current authentication number
   - **0x2:** Coin matches proposed authentication number  
   - **0x0:** Coin matches neither (not found or different owner)

4. **Response Generation:**
   - **STATUS_FIND_ALL_AN:** All coins match current authentication numbers
   - **STATUS_FIND_ALL_PAN:** All coins match proposed authentication numbers
   - **STATUS_FIND_NEITHER:** No coins match either authentication number
   - **STATUS_FIND_MIXED:** Mixed results with detailed per-coin response

**Healing Applications:**
- Identifies coins needing authentication number updates
- Supports consensus-based coin state determination
- Enables targeted healing of specific coin inconsistencies

### 4. Distributed Coin Repair (`cmd_fix`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 139 bytes) containing coin list, proposed group, and ticket array

**Returns:** None (modifies connection structure with repair results)

**Purpose:** Repairs coin authentication numbers based on distributed consensus from multiple RAIDA servers using ticket validation.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (139 bytes)
   - Validates coin data alignment (5 bytes per coin)
   - Extracts proposed group data and ticket array

2. **Distributed Consensus Collection:**
   - Creates worker threads for each RAIDA server (25 threads)
   - Each thread validates corresponding ticket with remote RAIDA
   - Uses send_validate_ticket_job for network communication
   - Collects coin lists from successful ticket validations

3. **Consensus Analysis:**
   - Counts how many RAIDA servers confirm each coin
   - Applies majority rule: coin repaired if confirmed by >12 servers
   - Uses (TOTAL_RAIDA_SERVERS / 2) + 1 as consensus threshold
   - Accumulates vote counts for each coin across all responses

4. **Authentication Number Repair:**
   - For coins with sufficient consensus:
     - Generates new authentication number using MD5 hash
     - Combines RAIDA ID, denomination, serial number, and proposed group
     - Updates coin with new authentication number and MFS timestamp
     - Marks database page as dirty for persistence

5. **Response Generation:**
   - Sets bitmap indicating which coins were successfully repaired
   - Status based on overall repair success rates
   - Provides detailed results for partial repair operations

**Distributed Processing:**
- **Concurrent Validation:** All 25 RAIDA servers contacted simultaneously
- **Thread Management:** One thread per RAIDA server for parallel processing
- **Network Resilience:** Handles individual server failures gracefully
- **Consensus Algorithm:** Majority voting determines repair decisions

### 5. Inter-Server Ticket Validation (`send_validate_ticket_job`)
**Parameters:**
- Thread argument structure containing RAIDA index, ticket number, and connection info

**Returns:** Thread result pointer

**Purpose:** Worker thread function that validates tickets with remote RAIDA servers for distributed consensus.

**Process:**
1. **Network Connection:**
   - Creates TCP socket to target RAIDA server
   - Sets receive timeout for reliable operation
   - Establishes connection using configured server addresses

2. **Protocol Communication:**
   - Constructs properly formatted request packet
   - Uses HEALING command group (2) and validate_ticket command (50)
   - Includes challenge data with CRC32 validation
   - Sends RAIDA number and ticket ID in request body

3. **Response Processing:**
   - Receives response header and validates status
   - Processes response body containing coin list
   - Converts network data to internal coin structures
   - Allocates result array for coin data

4. **Error Handling:**
   - Handles network timeouts and connection failures
   - Validates response packet structure and contents
   - Provides graceful degradation for unreachable servers
   - Proper resource cleanup on all code paths

**Network Protocol:**
- **Standard RAIDA Protocol:** Uses established inter-server communication format
- **Authentication:** Includes server identification and challenge validation
- **Reliability:** Timeout handling for network resilience
- **Data Validation:** CRC and size validation for received data

## Enhanced Ticket Management System

### Fine-Grained Locking Architecture
- **Per-Ticket Mutexes:** Each ticket has individual mutex for concurrent access
- **Non-Blocking Allocation:** get_free_ticket_slot() uses trylock for performance
- **Automatic Cleanup:** Expired tickets automatically freed by background process
- **Resource Efficiency:** Eliminates global lock contention

### Ticket Lifecycle Management
1. **Allocation:** get_free_ticket_slot() returns locked, initialized ticket
2. **Population:** Authenticated coins added to ticket structure
3. **Usage:** Other RAIDA servers validate and claim ticket
4. **Expiration:** Automatic cleanup after TICKET_RELEASE_SECONDS
5. **Unlocking:** unlock_ticket_entry() releases ticket for other operations

### Concurrency Benefits
- **Improved Throughput:** Multiple healing operations concurrent
- **Reduced Latency:** No blocking on global ticket pool access
- **Better Scalability:** Performance scales with number of tickets
- **Deadlock Prevention:** Consistent locking order prevents deadlocks

## concurrency model
 Concurrency Model
Each distributed operation (like ticket validation) is processed in parallel using a thread pool or concurrent task model.

## Each worker thread:
Communicates with one RAIDA server
Waits for a response or times out
Returns coin validation results

## Ticket Locking:
Each ticket has an associated exclusive lock.
Threads must acquire the lock before modifying ticket contents.
Locks must be released on all code paths, including error conditions.


## Distributed Consensus Algorithm

### Majority Voting System
- **Threshold:** Majority consensus requires >50% of RAIDA servers (13+ out of 25)
- **Fault Tolerance:** System tolerates up to 12 server failures
- **Byzantine Resilience:** Majority voting provides Byzantine fault tolerance
- **Deterministic Results:** Same inputs produce same consensus outcomes

### Consensus Collection Process
1. **Parallel Querying:** All 25 RAIDA servers queried simultaneously
2. **Response Aggregation:** Successful responses counted for each coin
3. **Threshold Application:** Coins with sufficient votes marked for repair
4. **Atomic Repair:** All consensus-approved repairs applied atomically

### Network Resilience
- **Timeout Handling:** Individual server timeouts don't block operation
- **Partial Results:** Operation succeeds with subset of responding servers
- **Error Isolation:** Network errors isolated to individual server threads
- **Graceful Degradation:** System continues operating with reduced server count

## Security and Validation

### Authentication Security
- **Ownership Proof:** All healing operations require proof of coin ownership
- **Consensus Validation:** Repairs only applied with sufficient consensus
- **Server Authentication:** Inter-server communication includes challenge validation
- **Ticket Security:** Tickets prevent unauthorized coin repair attempts

### Data Integrity
- **CRC Validation:** All network communications include integrity checking
- **Atomic Operations:** Database updates are atomic and consistent
- **Version Control:** MFS timestamps track all authentication number changes
- **Audit Trail:** All healing operations logged for security monitoring

### Attack Prevention
- **Consensus Requirements:** Prevents single-server manipulation
- **Ticket Expiration:** Prevents stale ticket replay attacks
- **Server Validation:** Only authorized RAIDA servers participate in consensus
- **Rate Limiting:** Healing operations subject to appropriate rate controls

## Error Handling and Recovery
## error codes

| Symbolic Code                  | Meaning                                    | Code (Suggested Value) |
| ------------------------------ | ------------------------------------------ | ---------------------- |
| `ERROR_INVALID_PACKET_LENGTH`  | Input request size too small/malformed     | 0xE0                   |
| `ERROR_MEMORY_ALLOC`           | Memory allocation failed                   | 0xE1                   |
| `ERROR_COINS_NOT_DIV`          | Coin payload is not divisible as expected  | 0xE2                   |
| `ERROR_NO_TICKET_SLOT`         | No available ticket slot                   | 0xE3                   |
| `ERROR_NO_TICKET_FOUND`        | Ticket not found                           | 0xE4                   |
| `ERROR_TICKET_CLAIMED_ALREADY` | Ticket already claimed by RAIDA            | 0xE5                   |
| `ERROR_WRONG_RAIDA`            | RAIDA ID out of range                      | 0xE6                   |


### Network Error Handling
- **Connection Failures:** Individual server failures don't prevent operation
- **Timeout Management:** Appropriate timeouts 