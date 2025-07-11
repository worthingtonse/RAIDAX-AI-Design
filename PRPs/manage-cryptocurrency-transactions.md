
# Crossover Transaction Management Implementation (crossover)

## Module Purpose
This module implements a comprehensive indexing and management system for cryptocurrency crossover transactions between the RAIDA network and external blockchain systems. It provides transaction tracking, confirmation monitoring, exchange rate management, and automated cleanup with optimized background processing. The system handles the complete lifecycle of cross-blockchain value transfers with enhanced performance optimizations.

## Constants

| Constant Name                | Purpose                                 | Typical Value |
|-----------------------------|-----------------------------------------|---------------|
| MAX_CROSSOVER_RECORDS       | Max number of simultaneous transactions | 1000          |
| CROSSOVER_EXPIRY            | Transaction timeout in seconds          | 3600          |
| CROSSOVER_HOUSEKEEPING_PERIOD | Max sleep time for background thread   | 30            |

## crossover_index_entry

| Field           | Type           | Description                          |
|----------------|----------------|--------------------------------------|
| locker_key     | [16]byte       | Unique transaction locker identifier |
| receipt_id     | [16]byte       | Unique receipt from client           |
| currency_code  | [3]char        | Currency being used (e.g., BTC)      |
| amount         | uint64         | Transaction amount                   |
| memo           | string (<=256) | Optional text field for notes        |


## Core Functionality

### 1. Index Initialization (`init_crossover_index`)
**Parameters:** None

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Initializes the crossover transaction index system and starts the optimized background processing thread.

**Process:**
1. Initializes mutex for thread-safe index access
2. Clears all index entries to NULL state
3. Launches optimized background thread for transaction monitoring
4. Sets up dynamic sleep scheduling for efficient resource usage

**Dependencies:**
- Threading system for background processing
- Synchronization primitives for thread safety
- Logging system for initialization status

### 2. Optimized Background Thread (`crossover_thread`)
**Parameters:** Thread argument pointer (unused)

**Returns:** Thread result pointer

**Purpose:** Main background processing loop with dynamic sleep optimization based on transaction expiry times.

**Process:**
1. **Cleanup Phase:** Removes expired transactions from index
2. **Sleep Calculation:** Determines optimal sleep duration by:
   - Finding next transaction expiry time
   - Calculating time until next required wake-up
   - Using minimum of calculated time and maximum period
3. **Adaptive Timing:** Sleeps for calculated duration instead of fixed intervals
4. **Continuous Operation:** Runs until server shutdown signal

**Performance Optimization:**
- Dynamic sleep duration based on actual transaction timing
- Minimizes CPU usage during quiet periods
- Responsive to imminent transaction expirations
- Efficient resource utilization

**Dependencies:**
- System time functions for expiry calculations
- Logging system for operational status
- Global shutdown signaling

### 3. Index Housekeeping (`housekeeping_crossover_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Removes expired crossover transactions from the index to prevent memory leaks and maintain data freshness.

**Process:**
1. Acquires exclusive lock on crossover index
2. Iterates through all index entries
3. For each active entry:
   - Calculates expiry time (first_seen + CROSSOVER_EXPIRY)
   - Compares with current time
   - Removes and frees expired entries
4. Releases index lock

**Dependencies:**
- System time functions for expiry checking
- Memory management for entry cleanup
- Thread synchronization for safe access

### 4. Transaction Entry Creation (`add_crossover_index_entry`)
**Parameters:**
- Locker key (16-byte identifier)
- Currency code (3-character string)
- Transaction amount (64-bit unsigned integer)
- Sender address (32-byte address, variable size up to address_size)
- Address size (integer, actual size of sender address)
- Receipt ID (16-byte unique identifier)
- Memo text (string pointer)
- Memo length (integer)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Creates new crossover transaction entry in the index with all transaction metadata.

**Process:**
1. **Slot Allocation:** Searches for empty slot in index array
2. **Memory Allocation:** Allocates new crossover index entry structure
3. **Data Population:**
   - Copies locker key and currency code
   - Stores transaction amount and sender address
   - Copies receipt ID and memo data
   - Records creation timestamp
   - Initializes completion status to false
4. **Index Insertion:** Places entry in available slot
5. **Logging:** Records successful entry creation

**Dependencies:**
- Memory management for entry allocation
- Thread synchronization for index modification
- System time for timestamp recording
- String handling for memo processing

### 5. Transaction Lookup (`get_crossover_index_entry`)
**Parameters:**
- Locker key (16-byte identifier)

**Returns:** Pointer to crossover index entry (NULL if not found)

**Purpose:** Retrieves crossover transaction entry by locker key for status checking and processing.

**Process:**
1. Acquires shared lock on crossover index
2. Iterates through index entries
3. Compares locker key with each entry
4. Returns matching entry or NULL
5. Releases index lock

**Dependencies:**
- Thread synchronization for safe access
- Memory comparison utilities

### 6. Depository Status Checking (`check_depository`)
**Parameters:**
- Locker key (16-byte identifier)
- Currency code (3-character identifier)
- Transaction ID (32-byte blockchain transaction identifier)
- Receipt ID (16-byte receipt identifier)
- Memo text (string pointer)
- Memo length (integer)

**Returns:** Integer status code indicating transaction status

**Purpose:** Verifies blockchain transaction confirmation status through external proxy service with comprehensive validation.

**Process:**
1. **Entry Validation:** Locates crossover entry by locker key
2. **Parameter Verification:**
   - Validates receipt ID matches entry
   - Confirms currency code matches
   - Constructs request payload with all parameters
3. **Proxy Communication:**
   - Sends confirmation request to external proxy
   - Includes required confirmation count from configuration
   - Handles both success and pending responses
4. **Response Processing:**
   - Validates returned transaction amount
   - Updates confirmation count in entry
   - Sets completion status if fully confirmed
5. **Status Return:** Returns appropriate status code

**Dependencies:**
- External proxy communication system
- Configuration system for confirmation requirements
- Network utilities for request/response handling
- Data validation utilities

### 7. Exchange Rate Retrieval (`get_exchange_rate`)
**Parameters:**
- Currency code (3-character identifier)
- Exchange rate output pointer (64-bit signed integer)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Retrieves current cryptocurrency exchange rates from external price feed service.

**Process:**
1. **Currency Validation:** Verifies supported currency (currently BTC only)
2. **Proxy Request:** Sends rate request to external service
3. **Response Processing:** Extracts and validates rate data
4. **Endianness Conversion:** Converts network byte order to host order
5. **Rate Storage:** Updates output parameter with current rate

**Dependencies:**
- External proxy communication system
- Data conversion utilities for endianness
- Error handling for network operations

### 8. Cryptocurrency Withdrawal (`withdraw_from_depository`)
**Parameters:**
- Locker key (16-byte identifier)
- Target address (string, destination cryptocurrency address)
- Address size (integer, length of target address)
- Conversion cost (64-bit unsigned integer, amount to withdraw)
- Currency code (3-character identifier)
- Receipt ID (16-byte identifier)
- Memo text (string pointer)
- Memo length (integer)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Executes cryptocurrency withdrawal from depository to external address through secure proxy communication.

**Process:**
1. **Parameter Validation:**
   - Validates currency code (currently BTC only)
   - Checks target address format and size
2. **Key Retrieval:**
   - Loads cryptocurrency private key from storage
   - Validates key size and format
3. **Request Construction:**
   - Builds withdrawal request payload
   - Includes all transaction parameters
   - Adds cryptographic signatures as required
4. **Proxy Communication:**
   - Sends withdrawal request to external service
   - Handles both immediate and pending responses
5. **Result Processing:**
   - Validates withdrawal completion
   - Handles waiting status for multi-party approval

**Dependencies:**
- Cryptocurrency key management system
- External proxy communication
- Data validation and formatting utilities
- Secure memory handling for private keys

### 9. Proxy Communication (`proxy_request`)
**Parameters:**
- Command number (integer, proxy operation type)
- Request body (character buffer)
- Body size (integer, size of request data)
- Output length pointer (integer, receives response size)
- Status pointer (8-bit unsigned integer, receives operation status)

**Returns:** Character buffer pointer containing response data (NULL on failure)

**Purpose:** Handles secure communication with external proxy service using encrypted channels and authentication.

**Process:**
1. **Connection Establishment:**
   - Creates TCP socket to proxy server
   - Sets appropriate timeouts for operations
   - Establishes non-blocking connection with timeout handling
2. **Request Encryption:**
   - Generates secure nonce for encryption
   - Constructs protocol header with authentication
   - Encrypts request body using CTR mode
   - Adds protocol trailer for integrity
3. **Data Transmission:**
   - Sends encrypted request to proxy
   - Handles partial write scenarios
4. **Response Processing:**
   - Receives encrypted response header
   - Validates response status and size
   - Receives and decrypts response body
   - Validates response integrity markers
5. **Connection Cleanup:**
   - Closes network connection
   - Frees temporary buffers
   - Returns decrypted response data

**Security Features:**
- CTR mode encryption for all communications
- Secure nonce generation for each request
- Authentication through proxy key validation
- Integrity verification of responses

**Dependencies:**
- Network socket operations for TCP communication
- Cryptographic utilities for encryption/decryption
- Configuration system for proxy settings and keys
- Memory management for request/response buffers

## Data Structures and Index Management

### Crossover Index Entry Structure
**Purpose:** Stores complete transaction metadata and status information

#### Transaction Identification
- **locker_key:** 16-byte unique locker identifier
- **receipt_id:** 16-byte unique receipt identifier
- **currency_code:** 3-character currency type identifier

#### Transaction Details
- **amount:** 64-bit unsigned integer transaction value
- **sender_address:** 32-byte blockchain address (variable length up to address_size)
- **address_size:** Integer indicating actual address length
- **memo:** Variable-length text description (up to MAX_MEMO_SIZE)

#### Status and Timing
- **completed:** Boolean completion status flag
- **confirmations:** 64-bit confirmation count from blockchain
- **first_seen:** Timestamp of transaction creation

### Index Configuration
- **MAX_CROSSOVER_RECORDS:** Maximum concurrent transactions supported
- **CROSSOVER_EXPIRY:** Transaction expiry time in seconds
- **CROSSOVER_HOUSEKEEPING_PERIOD:** Maximum background thread sleep duration

### Memory Management
- **Dynamic Allocation:** Index entries allocated as needed
- **Automatic Cleanup:** Expired entries automatically freed
- **Thread-Safe Access:** All operations protected by mutex synchronization

## External Service Integration

### Proxy Service Commands
- **CMD_PROXY_WATCH_FOR_TRANSACTION:** Monitor blockchain confirmations
- **CMD_PROXY_GET_RATE:** Retrieve current exchange rates
- **CMD_PROXY_SEND_TRANSACTION:** Execute cryptocurrency withdrawals

### Request/Response Format
- **Encryption:** All communications encrypted using CTR mode
- **Authentication:** Proxy key used for request authentication
- **Integrity:** Response validation through trailer markers

### Error Handling
- **Network Errors:** Connection failures and timeouts handled gracefully
- **Protocol Errors:** Invalid responses detected and reported
- **Service Errors:** External service failures propagated appropriately

## Security and Validation

### Transaction Security
- **Unique Identifiers:** Receipt IDs prevent transaction replay
- **Value Validation:** Amount verification prevents manipulation
- **Address Validation:** Cryptocurrency address format checking

### Communication Security
- **Encrypted Channels:** All proxy communication encrypted
- **Authentication:** Proxy key prevents unauthorized access
- **Integrity Checking:** Response validation ensures data integrity

### Key Management
- **Secure Storage:** Cryptocurrency private keys protected
- **Access Control:** Key access restricted to authorized operations
- **Memory Protection:** Secure memory handling for sensitive data

## Dependencies and Integration

### Required Modules
- **Configuration System:** Proxy settings, confirmation requirements, authentication keys
- **Network Layer:** TCP socket operations, connection management
- **Cryptographic Utilities:** CTR mode encryption, secure random generation
- **Threading System:** Background processing, synchronization primitives
- **Memory Management:** Dynamic allocation, secure memory handling
- **Logging System:** Operational status, error reporting

### External Services
- **Blockchain Proxy:** Transaction monitoring and execution service
- **Price Feed Service:** Exchange rate information provider
- **Key Storage System:** Cryptocurrency private key management

### Used By
- **Crossover Commands:** Transaction initiation and status checking
- **Administrative Tools:** Transaction monitoring and management
- **Exchange Services:** Rate checking and withdrawal operations

## Performance Optimizations

### Dynamic Sleep Scheduling
- **Adaptive Timing:** Sleep duration based on actual transaction expiry times
- **Resource Efficiency:** Minimizes CPU usage during quiet periods
- **Responsive Processing:** Quick response to imminent transaction events

### Index Management
- **Linear Search:** Simple array-based index for moderate transaction volumes
- **Automatic Cleanup:** Expired entries removed without manual intervention
- **Memory Efficiency:** Minimal memory overhead per transaction

### Network Operations
- **Connection Reuse:** Efficient socket management for proxy communication
- **Timeout Handling:** Prevents hanging on unresponsive external services
- **Error Recovery:** Graceful handling of network failures

## Threading and Concurrency
- **Background Processing:** Dedicated thread for transaction monitoring
- **Thread-Safe Access:** Mutex protection for all index operations
- **Lock Ordering:** Consistent locking order prevents deadlocks
- **Atomic Operations:** Safe concurrent access to shared data structures

## status and error code
| Code Name                 |  Meaning                                                          |
| ------------------------- |  ---------------------------------------------------------------  |
| `STATUS_SUCCESS`          |   Transaction confirmed successfully                              |
| `STATUS_TX_SEEN`          |   Transaction detected but not yet confirmed                      |
| `STATUS_WAITING`          |   Proxy is waiting for responses from other RAIDA servers         |
| `ERROR_NO_ENTRY`          |   No matching crossover entry found in the index                  |
| `ERROR_INVALID_PARAMETER` |   One or more input parameters were invalid                       |
| `ERROR_AMOUNT_MISMATCH`   |   Blockchain amount does not match expected transaction amount    |
| `ERROR_NO_PRIVATE_KEY`    |   Required private key for the currency is missing                |
| `ERROR_INTERNAL`          |   Internal error such as memory allocation failure                |
| `ERROR_PROXY_CONNECT`     |   Unable to establish a TCP connection with the proxy server      |
| `ERROR_PROXY`             |   Proxy returned an invalid, malformed, or unprocessable response |
| `ERROR_INDEX_FULL`        |   The crossover index is full (no available slot)                 |


This module provides the foundation for secure, efficient cross-blockchain value transfer operations, enabling seamless integration between the RAIDA network and external cryptocurrency systems.
