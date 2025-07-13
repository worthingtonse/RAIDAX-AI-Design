<!-- # Specification: Change Commands Implementation (cmd_change.c)

## 1. Module Purpose
This implementation file provides cryptocurrency denomination change operations for the RAIDAX system, part of the CloudCoinConsortium project. It implements break and join operations for splitting higher denomination coins into smaller denominations and combining smaller denominations into higher value coins, with session-based reservations and comprehensive authentication verification.

## 2. System Architecture Overview

### 2.1 Implementation Components
- **Change Availability Discovery**: Finding available coins for denomination change operations
- **Break Operations**: Splitting higher denomination coins into multiple smaller denomination coins
- **Join Operations**: Combining multiple smaller denomination coins into higher denomination coins
- **Session-based Reservations**: Page reservation system for atomic multi-coin operations
- **Authentication Verification**: Cryptographic verification of coin ownership before operations
- **Rate Limiting**: IP-based rate limiting for change operation requests

### 2.2 Security Model
- **Authentication Requirements**: Ownership verification through authentication number validation
- **Session Management**: Session-based page reservations for transaction consistency
- **Rate Limiting**: IP hash table rate limiting to prevent abuse
- **Atomic Operations**: Transaction-like operations for multi-coin changes
- **Page Locking**: Thread-safe page access with reservation mechanisms

## 3. System Dependencies and Configuration

### 3.1 Standard Library Dependencies
- **Input/Output Operations**: Standard I/O functions for data processing
- **String Operations**: String manipulation and memory operations
- **Memory Management**: Dynamic memory allocation and buffer management
- **Integer Operations**: Standard integer type definitions and operations
- **Random Generation**: Random number generation for coin invalidation

### 3.2 Project Dependencies
- **Protocol Module**: Communication protocol definitions and connection structures
- **Logging Module**: Debug and error logging capabilities
- **Commands Module**: Command processing framework integration
- **Database Module**: Page-based storage system and locking mechanisms
- **Hash Table Module**: IP-based rate limiting hash table operations
- **Configuration Module**: System configuration management
- **Utility Module**: Data conversion and time calculation functions

## 4. Configuration Constants and Limits

### 4.1 Operation Limits
- **Maximum Change Coins**: 64 coins maximum for change availability queries
- **Fixed Coin Count**: 10 coins involved in break and join operations
- **Operation Types**: Break (split) and join (combine) operation codes
- **Denomination Relationships**: Adjacent denomination level requirements

### 4.2 Operation Codes
- **Break Operation**: Code 0x1 for splitting higher denomination into lower
- **Join Operation**: Code 0x2 for combining lower denominations into higher
- **Operation Validation**: Strict validation of operation code parameters

## 5. Change Availability Discovery Implementation

### 5.1 Get Available Change Serial Numbers Purpose
**Function Objective**: Discover available coins for denomination change operations with session reservation.

**Operation Types**: Support both break and join operations with appropriate denomination targeting.

**Session Management**: Reserve pages for atomic change operations using session identifiers.

### 5.2 Request Validation Implementation
**Purpose**: Validate change availability request parameters and operation types.

**Validation Process**:
1. **Fixed Size Validation**: Verify request meets exact 24-byte requirement
2. **Session Extraction**: Extract session identifier for page reservation
3. **Operation Validation**: Verify operation code is valid break or join
4. **Denomination Validation**: Ensure denomination within valid system range
5. **Parameter Consistency**: Verify parameter combinations are valid

**Request Structure**:
- **Session Identifier**: 4-byte session ID for page reservation
- **Operation Code**: 1-byte operation type (break or join)
- **Denomination**: 1-byte target denomination for change operation
- **Protocol Overhead**: Challenge header and EOF trailer

### 5.3 Denomination Target Calculation Implementation
**Purpose**: Calculate target denomination for coin discovery based on operation type.

**Target Calculation**:
1. **Break Operation**: Target denomination one level lower than requested
2. **Join Operation**: Target denomination one level higher than requested
3. **Range Validation**: Ensure target denomination within valid system limits
4. **Availability Search**: Search for available coins in target denomination

**Operation Logic**:
- **Break Target**: Search lower denomination for coins to receive from break
- **Join Target**: Search higher denomination for available slots after join
- **Denomination Arithmetic**: Simple increment/decrement of denomination level
- **Boundary Checking**: Prevent denomination calculations outside valid range

### 5.4 Page Scanning and Reservation Implementation
**Purpose**: Scan pages for available coins with session-based reservation for atomic operations.

**Scanning Process**:
1. **Page Iteration**: Scan all pages for target denomination
2. **Reservation Check**: Verify pages not reserved by other sessions
3. **Page Reservation**: Reserve pages using provided session identifier
4. **Record Scanning**: Scan individual records within pages for availability
5. **Limit Enforcement**: Enforce maximum available coins limit
6. **Result Collection**: Collect available serial numbers for response

**Page Management**:
- **Lock Acquisition**: Use existing page locking functions for thread-safe access
- **Reservation Validation**: Call existing page management functions to check reservations
- **Session Assignment**: Use existing page reservation functions to assign to current session
- **Resource Cleanup**: Call existing page management functions for proper unlocking

## 6. Break Operation Implementation

### 6.1 Break Command Purpose
**Function Objective**: Split higher denomination coin into multiple lower denomination coins.

**Operation Requirements**: Authentication of source coin and session validation for target coins.

**Result**: Source coin invalidated, multiple lower denomination coins created.

### 6.2 Break Request Validation Implementation
**Purpose**: Validate break operation request parameters and structure.

**Validation Process**:
1. **Fixed Size Validation**: Verify request meets exact 253-byte requirement
2. **Session Extraction**: Extract session identifier for operation consistency
3. **Source Coin Validation**: Validate source coin denomination and serial number
4. **Target Coin Validation**: Validate 10 target coins with proper denomination
5. **Rate Limiting**: Apply IP-based rate limiting for operation requests

**Request Structure**:
- **Session Identifier**: 4-byte session ID from availability query
- **Source Coin**: 1 denomination + 4 serial number + 16 authentication number
- **Target Coins**: 10 coins, each with 1 denomination + 4 serial number + 16 authentication number
- **Total Size**: 253 bytes including protocol overhead

### 6.3 Source Coin Authentication Implementation
**Purpose**: Verify ownership of source coin before break operation.

**Authentication Process**:
1. **Page Access**: Access page containing source coin data
2. **Authentication Extraction**: Extract stored authentication number from page
3. **Comparison Operation**: Compare stored authentication with provided authentication
4. **Ownership Verification**: Verify client owns source coin before proceeding
5. **Access Control**: Prevent unauthorized break operations

**Security Measures**:
- **Constant-Time Comparison**: Use secure memory comparison for authentication
- **Page Locking**: Ensure thread-safe access during authentication
- **Error Handling**: Handle authentication failures securely
- **Resource Management**: Proper page unlocking on all code paths

### 6.4 Target Coin Creation Implementation
**Purpose**: Create multiple lower denomination coins from break operation.

**Creation Process**:
1. **Denomination Verification**: Verify target coins are one denomination lower
2. **Page Reservation Check**: Verify pages reserved by current session
3. **Authentication Assignment**: Assign provided authentication numbers to target coins
4. **Status Setting**: Set months-from-start status for coin activation
5. **Page Marking**: Mark pages as dirty for persistence

**Coin Management**:
- **Sequential Processing**: Process all 10 target coins sequentially
- **Session Verification**: Verify page reservations match current session using existing page management functions
- **Memory Operations**: Use standard memory functions for authentication assignment
- **Status Management**: Proper status byte assignment for new coins
- **Persistence Integration**: Call external dirty queue function to mark pages for persistence

### 6.5 Source Coin Invalidation Implementation
**Purpose**: Invalidate source coin after successful target coin creation.

**Invalidation Process**:
1. **Random Generation**: Use standard library random functions for authentication number generation
2. **Authentication Replacement**: Replace original authentication with random data using memory functions
3. **Status Reset**: Set months-from-start status to zero (free)
4. **Page Persistence**: Call external dirty queue function to mark page for persistence
5. **Operation Completion**: Complete break operation successfully

**Security Considerations**:
- **Random Seeding**: Use time and RAIDA number for random seed
- **Secure Invalidation**: Ensure original authentication cannot be recovered
- **Atomic Completion**: Ensure invalidation occurs only after target creation
- **Persistence Guarantee**: Ensure changes persisted to storage

## 7. Join Operation Implementation

### 7.1 Join Command Purpose
**Function Objective**: Combine multiple lower denomination coins into single higher denomination coin.

**Operation Requirements**: Authentication of all source coins and session validation for target coin.

**Result**: Multiple source coins invalidated, single higher denomination coin created.

### 7.2 Join Request Validation Implementation
**Purpose**: Validate join operation request parameters and structure.

**Validation Process**:
1. **Fixed Size Validation**: Verify request meets exact 253-byte requirement
2. **Session Extraction**: Extract session identifier for operation consistency
3. **Target Coin Validation**: Validate target coin denomination and serial number
4. **Source Coin Validation**: Validate 10 source coins with proper denomination
5. **Denomination Consistency**: Verify source coins are one denomination lower

**Request Structure**:
- **Session Identifier**: 4-byte session ID from availability query
- **Target Coin**: 1 denomination + 4 serial number + 16 authentication number
- **Source Coins**: 10 coins, each with 1 denomination + 4 serial number + 16 authentication number
- **Size Consistency**: Same 253-byte structure as break operation

### 7.3 Multi-Coin Authentication Implementation
**Purpose**: Verify ownership of all source coins before join operation.

**Authentication Process**:
1. **Sequential Verification**: Verify authentication for each of 10 source coins
2. **Page Access**: Access page for each source coin individually
3. **Authentication Comparison**: Compare stored and provided authentication numbers
4. **Complete Verification**: Require all coins authenticated before proceeding
5. **Failure Handling**: Stop operation if any coin fails authentication

**Security Requirements**:
- **All-or-Nothing**: All coins must authenticate or operation fails
- **Individual Verification**: Each coin verified independently
- **Session Consistency**: Verify operation consistency throughout process
- **Error Isolation**: Handle individual coin failures without information disclosure

### 7.4 Source Coin Invalidation Implementation
**Purpose**: Invalidate all source coins after successful authentication.

**Invalidation Process**:
1. **Sequential Processing**: Process all 10 source coins for invalidation
2. **Page Access**: Use existing page management functions to access each source coin page
3. **Status Reset**: Set months-from-start status to zero for each coin
4. **Page Persistence**: Call external dirty queue function to mark all modified pages
5. **Resource Cleanup**: Use existing page management functions for proper unlocking

**Operation Safety**:
- **Atomic Invalidation**: Invalidate all source coins atomically
- **Consistent State**: Maintain consistent state throughout operation
- **Error Recovery**: Handle errors during invalidation process
- **Resource Management**: Ensure proper cleanup on all code paths

### 7.5 Target Coin Creation Implementation
**Purpose**: Create single higher denomination coin from join operation.

**Creation Process**:
1. **Page Access**: Access page for target coin creation
2. **Session Verification**: Verify page reserved by current session
3. **Authentication Assignment**: Assign provided authentication number to target coin
4. **Status Setting**: Set months-from-start status to zero (available)
5. **Operation Completion**: Complete join operation successfully

**Target Management**:
- **Single Coin Creation**: Create one higher denomination coin
- **Authentication Handling**: Use provided authentication number
- **Status Management**: Proper status assignment for created coin
- **Persistence**: Ensure target coin persisted to storage

## 8. Error Handling and Security Implementation

### 8.1 Rate Limiting Implementation
**Purpose**: Prevent abuse of change operations through IP-based rate limiting.

**Rate Limiting Process**:
- **IP Hash Table**: Use hash table to track request rates per IP address
- **Request Counting**: Count requests within time windows
- **Limit Enforcement**: Enforce maximum request rates per IP
- **Error Response**: Return rate limit error when threshold exceeded

### 8.2 Session Management Implementation
**Purpose**: Ensure session consistency across change operations.

**Session Validation**:
- **Session Tracking**: Track session identifiers across operations
- **Reservation Consistency**: Verify page reservations match sessions
- **Operation Atomicity**: Ensure atomic operations within sessions
- **Session Cleanup**: Handle session cleanup and timeout

### 8.3 Error Recovery Implementation
**Purpose**: Handle errors gracefully while maintaining system consistency.

**Error Categories**:
- **Authentication Errors**: Source coin authentication failures
- **Session Errors**: Session mismatch or reservation conflicts
- **Parameter Errors**: Invalid denominations or coin parameters
- **Resource Errors**: Page access or memory allocation failures
- **Rate Limit Errors**: Request rate threshold exceeded

**Recovery Strategy**:
- **Partial Operation Rollback**: Rollback partial operations on errors
- **Resource Cleanup**: Ensure proper cleanup on all error paths
- **State Consistency**: Maintain consistent state despite errors
- **Error Reporting**: Provide meaningful error codes for different conditions

## 9. Performance and Resource Management

### 9.1 Operation Efficiency
- **Batch Processing**: Process multiple coins efficiently within operations
- **Page Access Optimization**: Minimize page lock duration
- **Memory Management**: Efficient buffer allocation and cleanup
- **Session Reuse**: Reuse sessions across related operations

### 9.2 Concurrency Management
- **Page Locking**: Proper page locking for thread safety
- **Session Isolation**: Isolate session-based operations
- **Resource Contention**: Handle resource contention appropriately
- **Deadlock Prevention**: Prevent deadlock through proper lock ordering

### 9.3 Scalability Considerations
- **Rate Limiting**: Scale rate limiting with system load
- **Session Management**: Handle large numbers of concurrent sessions
- **Memory Usage**: Scale memory usage with operation complexity
- **Page Management**: Efficient page access for high-volume operations

## 10. Integration Requirements

### 10.1 Protocol Integration
- **Command Status**: Use standardized protocol status codes
- **Response Format**: Generate protocol-compatible responses
- **Connection Management**: Integrate with connection structures
- **Session Coordination**: Coordinate with protocol session management

### 10.2 Storage Integration
- **Page System**: Integrate with page-based storage system
- **Dirty Queue**: Use dirty queue for data persistence
- **Lock Coordination**: Coordinate with page locking mechanisms
- **Consistency**: Maintain storage consistency across operations

### 10.3 Security Integration
- **Rate Limiting**: Integrate with system-wide rate limiting
- **Authentication**: Coordinate with authentication frameworks
- **Audit Logging**: Provide audit trail for change operations
- **Access Control**: Respect system access control policies

This specification provides complete implementation guidance for the RAIDAX change commands while emphasizing the critical denomination change operations, session-based consistency, multi-coin authentication, and atomic transaction requirements essential for secure cryptocurrency denomination conversion operations. -->



# Change-Making Commands Implementation (cmd_change)

## Module Purpose
This module implements denomination conversion operations for the RAIDA network, enabling users to break larger denomination coins into smaller ones or join smaller coins into larger denominations. It provides secure change-making functionality with session-based page reservation and comprehensive validation to maintain the economic integrity of coin transformations.

## Core Functionality

## constants taken from protocol and config definition


#### Change Operation Input Format (253 bytes)

| Field                        | Offset       | Size     | Description                                      |
|-----------------------------|--------------|----------|--------------------------------------------------|
| `Session ID`                | 0            | 4        | Identifies the requesting client/session         |
| `Target Denomination`       | 4            | 1        | Denomination of coin to break or create          |
| `Target Serial Number`      | 5            | 4        | SN of coin to break (BREAK) or to be created (JOIN) |
| `Target AN (Auth Number)`   | 9            | 16       | Auth number for coin being broken or created     |
| `Smaller Coin Blocks`       | 25           | 21 × 10  | Each of the 10 smaller coins:                    |
|                             |              |          | - 1 byte: Denomination                           |
|                             |              |          | - 4 bytes: Serial Number                         |
|                             |              |          | - 16 bytes: New Auth Number                      |

#### Coin Block Format (21 bytes per coin × 10)

| Field              | Offset | Size | Description                       |
|--------------------|--------|------|-----------------------------------|
| `Denomination`     | 0      | 1    | Denomination of this coin         |
| `Serial Number`    | 1      | 4    | Absolute serial number            |
| `Auth Number (AN)` | 5      | 16   | Ownership proof (e.g., PAN, BAN)  |


#### MFS (Most-Frequently-Used Timestamp Byte)

- Stored in each coin record at byte offset 16 (i.e., `record[16]`)
- If `MFS = 0`, coin is considered free (not in circulation)
- When assigning a coin: `MFS = current_mfs()`

#### Authentication Number Management

| Operation                 | Description                                                            |
|---------------------------|------------------------------------------------------------------------|
| `Source Coin Validation` | Compare provided AN with stored AN in page to verify ownership          |
| `Create New Coin`         | Assign new AN from payload and set MFS                                 |
| `Destroy Coin` (BREAK)    | Overwrite AN with MD5-hashed pseudorandom data, set MFS = 0            |

**Hashing Function:** `MD5(input_string)`  
**Input for MD5:** `random_hex(16)` built from seeded `rand()` (e.g., `sprintf("%08x%08x")`)


#### Page Reservation & Locking (Session Lifecycle)

| Phase         | Purpose                                                    |
|---------------|------------------------------------------------------------|
| **Scan**      | Reserve pages that contain available SNs                   |
| **Validate**  | Ensure caller still holds page reservation before write    |
| **Commit**    | Modify coins only on pages reserved by session ID          |
| **Expire**    | Reservation released after timeout (handled outside scope) |


### 1. Available Serial Numbers Query (`cmd_get_available_change_sns`)
**Parameters:**
- Connection information structure
- Input: 24-byte payload containing session ID, operation type, and target denomination

**Returns:** None (modifies connection structure with available serial numbers)

**Purpose:** Finds available coin slots for change-making operations by scanning the target denomination for unused coin positions.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (24 bytes)
   - Extracts session ID for page reservation tracking
   - Validates operation type (OP_BREAK or OP_JOIN)
   - Validates target denomination within acceptable range

2. **Target Denomination Logic:**
   - **Break Operation:** Searches denomination-1 (smaller coins needed)
   - **Join Operation:** Searches denomination+1 (larger coin needed)
   - Ensures target denomination is within valid range

3. **Available Coin Discovery:**
   - Iterates through all possible pages for target denomination
   - Uses on-demand page cache for efficient access
   - Skips pages that are already reserved by other sessions
   - Reserves discovered pages with provided session ID

4. **Coin Slot Identification:**
   - For each page, scans all coin records
   - Identifies available slots where MFS = 0 (not in circulation)
   - Calculates absolute serial numbers from page and record positions
   - Limits results to MAX_CHANGE_COINS to prevent excessive responses

5. **Response Construction:**
   - Returns denomination byte followed by available serial numbers
   - Each serial number encoded as 4-byte big-endian value
   - Total response size varies based on available coins found

**Session Management:**
- Pages reserved during scan to prevent concurrent allocation
- Session ID tracks which client reserved which pages
- Reservations automatically expire after timeout period

**Performance Considerations:**
- On-demand cache may trigger disk reads for unused pages
- Trade-off between complete availability scanning and performance
- Page reservation prevents race conditions during change operations

### 2. Break Operation (`cmd_break`)
**Parameters:**
- Connection information structure
- Input: 253-byte payload containing session data, source coin, and target coin specifications

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Breaks a single larger denomination coin into 10 smaller denomination coins with value conservation.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (253 bytes)
   - Extracts session ID, source coin denomination and serial number
   - Validates source denomination within acceptable range
   - Checks rate limiting to prevent abuse

2. **Source Coin Authentication:**
   - Retrieves source coin page using on-demand cache
   - Verifies provided authentication number matches stored value
   - Ensures caller owns the coin being broken
   - Rejects operation if authentication fails

3. **Target Coin Validation:**
   - Validates all 10 target coins have correct denomination (source-1)
   - Ensures all target coin pages are reserved by this session
   - Verifies no unauthorized access to target coin slots
   - Confirms proper session-based allocation

4. **Ownership Transfer:**
   - For each of the 10 smaller coins:
     - Updates authentication number with provided value
     - Sets MFS timestamp to current time
     - Marks page as dirty for persistence
   - Creates 10 smaller coins with combined value equal to source

5. **Source Coin Destruction:**
   - Generates pseudo-random new authentication number for source coin
   - Uses MD5 hash of random data for cryptographic randomness
   - Sets MFS to 0 to mark coin as not in circulation
   - Effectively destroys the larger coin

**Value Conservation:**
- 1 large coin (denomination N) = 10 smaller coins (denomination N-1)
- Mathematical relationship: 10^N = 10 × 10^(N-1)
- Maintains total value in circulation

**Security Features:**
- Session-based page reservation prevents unauthorized access
- Authentication verification ensures ownership
- Rate limiting prevents automated attacks
- Atomic operation ensures consistency

### 3. Join Operation (`cmd_join`)
**Parameters:**
- Connection information structure  
- Input: 253-byte payload containing session data, target coin, and source coin specifications

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Joins 10 smaller denomination coins into a single larger denomination coin with value conservation.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (253 bytes)
   - Extracts session ID, target coin denomination and serial number
   - Validates target denomination within acceptable range

2. **Target Coin Validation:**
   - Verifies target coin page is reserved by this session
   - Ensures authorized access to destination coin slot
   - Validates session consistency for join operation

3. **Source Coin Authentication:**
   - For each of the 10 smaller coins:
     - Validates denomination equals target-1
     - Retrieves coin data using on-demand cache
     - Verifies authentication number matches stored value
     - Ensures caller owns all coins being joined
   - Rejects entire operation if any coin fails authentication

4. **Source Coin Destruction:**
   - For each authenticated smaller coin:
     - Sets MFS to 0 to mark as not in circulation
     - Marks page as dirty for persistence
   - Removes 10 smaller coins from circulation

5. **Target Coin Creation:**
   - Updates target coin authentication number with provided value
   - Sets MFS timestamp to current time
   - Marks page as dirty for persistence
   - Creates single larger coin with combined value

**Value Conservation:**
- 10 smaller coins (denomination N-1) = 1 large coin (denomination N)  
- Mathematical relationship: 10 × 10^(N-1) = 10^N
- Maintains total value in circulation

**Atomic Operation Guarantee:**
- All source coins validated before any modifications
- Either all operations succeed or none succeed
- Prevents partial state corruption

## Session-Based Reservation System

### Page Reservation Mechanism
- **Session Tracking:** Each page can be reserved by specific session ID
- **Timeout Management:** Reservations automatically expire after configured period
- **Conflict Prevention:** Prevents concurrent allocation of same coin slots
- **Resource Protection:** Ensures exclusive access during change operations

### Reservation Lifecycle
1. **Discovery Phase:** Pages reserved during available coin scanning
2. **Validation Phase:** Reserved status verified before coin operations
3. **Execution Phase:** Coins modified only on reserved pages
4. **Completion Phase:** Reservations released after operation completion

### Security Implications
- **Authorization:** Only session holder can modify reserved coins
- **Integrity:** Prevents race conditions in multi-user environment  
- **Audit Trail:** Session tracking provides operation accountability
- **Resource Management:** Automatic cleanup prevents resource leaks

## Operation Types and Constants

### Operation Type Definitions
- **OP_BREAK (0x1):** Break larger coin into 10 smaller coins
- **OP_JOIN (0x2):** Join 10 smaller coins into larger coin

### Configuration Constants
- **MAX_CHANGE_COINS:** 64 - Maximum coins returned per availability query
- Prevents excessive response sizes and resource consumption

### Denomination Relationships
- **Break Target:** source_denomination - 1
- **Join Target:** source_denomination + 1
- **Value Ratio:** 1:10 relationship between adjacent denominations

## Data Structures and Formats

### Input Data Formats
- **Availability Query:** 24 bytes (4-byte session + 1-byte operation + 1-byte denomination + padding)
- **Change Operation:** 253 bytes (session + source coin + 10 target coins with authentication)
- **Coin Specification:** 21 bytes (1-byte denomination + 4-byte serial + 16-byte authentication)

### Output Data Formats  
- **Available Coins:** Variable size (1-byte denomination + 4-byte serial numbers)
- **Operation Results:** Status codes indicating success/failure
- **Error Responses:** Appropriate error codes for various failure conditions

### Authentication Data
- **Current AN:** 16-byte authentication number proving current ownership
- **Proposed AN:** 16-byte authentication number for new ownership
- **Random AN:** 16-byte cryptographically random value for coin destruction

## Error Handling and Validation

### Input Validation
- **Size Validation:** All operations require exact payload sizes
- **Denomination Validation:** Target denominations must be within valid range
- **Session Validation:** Session IDs must be consistent across related operations
- **Alignment Validation:** Coin data must be properly aligned to record boundaries

### Security Validation
- **Authentication Validation:** All coin ownership must be proven before modification
- **Reservation Validation:** All target coins must be properly reserved
- **Rate Limiting:** Break operations subject to IP-based rate limiting
- **Session Consistency:** Operations must use consistent session identifiers

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_INVALID_PARAMETER`: Invalid operation type or denomination
- `ERROR_INVALID_SN_OR_DENOMINATION`: Invalid coin reference
- `ERROR_PAGE_IS_NOT_RESERVED`: Attempt to access unreserved page
- `ERROR_REQUEST_RATE`: Rate limit exceeded for break operations
- `STATUS_ALL_FAIL`: Authentication failure for source coins

### Recovery Mechanisms
- **Atomic Operations:** Failed operations leave system state unchanged
- **Resource Cleanup:** Page locks and reservations properly released
- **Error Reporting:** Detailed error codes aid in troubleshooting

## Security Considerations

### Economic Security
- **Value Conservation:** Mathematical verification ensures no value creation/destruction
- **Atomic Operations:** Prevents partial state that could duplicate or destroy value
- **Audit Trail:** All operations logged with timestamps and session tracking

### Access Control
- **Ownership Verification:** All coin modifications require proof of ownership
- **Session Authorization:** Page reservations prevent unauthorized access
- **Rate Limiting:** Automated attack prevention through request rate control

### Cryptographic Security  
- **Random Generation:** Coin destruction uses cryptographically secure randomness
- **Authentication Numbers:** 16-byte values provide adequate cryptographic strength
- **MD5 Hashing:** Used for generating pseudo-random authentication numbers

## Performance Characteristics

### On-Demand Cache Integration
- **Cache Efficiency:** Change operations benefit from page caching
- **Memory Usage:** Only accessed pages loaded into memory
- **I/O Patterns:** Sequential page access for availability scanning

### Operation Complexity
- **Break Operation:** O(1) source validation + O(10) target updates
- **Join Operation:** O(10) source validation + O(1) target creation
- **Availability Query:** O(pages × records) for complete denomination scan

### Resource Management
- **Page Locking:** Efficient fine-grained locking for concurrent access
- **Memory Allocation:** Dynamic allocation for variable-sized responses
- **Session Tracking:** Minimal overhead for reservation management

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache with reservation system
- **Authentication System:** Coin ownership verification
- **Rate Limiting:** IP-based request rate control (hash table)
- **Cryptographic Utilities:** MD5 hashing, random number generation
- **Configuration System:** Server parameters and denomination limits

### External Constants Required
- `RECORDS_PER_PAGE`: Database page organization
- `TOTAL_PAGES`: Maximum pages per denomination
- `MIN_DENOMINATION` / `MAX_DENOMINATION`: Valid denomination range
- `ERROR_*`: Protocol error definitions

### Used By
- **Client Applications:** Denomination conversion operations
- **Trading Systems:** Change-making for exact value transactions
- **Economic Tools:** Value rebalancing across denominations

## Threading and Concurrency
- **Page-Level Locking:** Individual page locks enable concurrent operations
- **Session Isolation:** Reservations prevent interference between sessions
- **Atomic Updates:** Database page modifications are atomic
- **Resource Safety:** Proper cleanup prevents resource leaks

**Authentication Number Management**
- Each coin record stores a 16-byte authentication number (AN)
- ANs are updated using secure cryptographic processes:
  - Pseudorandom ANs are generated using a random function and hash (e.g., MD5 or SHA-256)
  - For source coin destruction, a new random AN is assigned and marked MFS = 0


This change-making module provides essential economic functionality for the RAIDA network, enabling flexible denomination management while maintaining strict value conservation and security properties.