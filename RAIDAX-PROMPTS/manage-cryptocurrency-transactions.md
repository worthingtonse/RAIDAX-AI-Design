# Language-Agnostic Specification: Crossover Implementation

## 1. Module Purpose
This module implements the crossover system for the CloudCoin RAIDA network, enabling cross-network cryptocurrency transactions and exchange operations. It manages a time-based index of pending transactions, provides optimized background processing with dynamic sleep scheduling, and handles secure communication with external proxy services for cryptocurrency operations.

## 2. System Architecture Overview

### 2.1 Core Components
- **Time-Based Transaction Index**: Memory-based tracking of pending crossover transactions
- **Optimized Background Processing**: Dynamic sleep scheduling based on transaction expiry times
- **Secure Proxy Communication**: Encrypted communication with external cryptocurrency services
- **Exchange Rate Management**: Real-time cryptocurrency exchange rate retrieval
- **Transaction Validation**: Multi-step verification process for cross-network operations

### 2.2 Processing Flow
- **Transaction Registration**: Add pending transactions to time-indexed system
- **Dynamic Monitoring**: Background thread with intelligent sleep scheduling
- **Proxy Integration**: Secure communication for transaction verification and execution
- **Automatic Cleanup**: Time-based expiry and removal of stale transactions

## 3. System Constants and Configuration

### 3.1 Index Management Constants
```
MAX_CROSSOVER_RECORDS = maximum concurrent crossover transactions
CROSSOVER_EXPIRY = transaction expiry time in seconds
CROSSOVER_HOUSEKEEPING_PERIOD = maximum background thread sleep duration
MAX_MEMO_SIZE = maximum memo field size in bytes
```

### 3.2 Cryptocurrency Constants
```
BTC_KEY_MAX_SIZE = 3000 bytes          // Maximum cryptocurrency key size
RESPONSE_HEADER_SIZE = 32 bytes        // Proxy response header size
REQUEST_HEADER_SIZE = 32 bytes         // Proxy request header size
RAIDA_SERVER_RCV_TIMEOUT = timeout for proxy communication
```

### 3.3 Proxy Command Constants
```
CMD_PROXY_WATCH_FOR_TRANSACTION = monitor transaction status
CMD_PROXY_GET_RATE = retrieve exchange rates
CMD_PROXY_SEND_TRANSACTION = execute transaction
CROSSOVER = command group identifier for proxy operations
```

### 3.4 Status Response Codes
```
STATUS_SUCCESS = successful operation
STATUS_WAITING = operation pending confirmation
STATUS_TX_SEEN = transaction detected but not confirmed
ERROR_NO_ENTRY = crossover entry not found
ERROR_INVALID_PARAMETER = invalid input parameters
ERROR_AMOUNT_MISMATCH = transaction amount verification failure
ERROR_NO_PRIVATE_KEY = cryptocurrency key not available
ERROR_INTERNAL = unexpected system error
ERROR_PROXY_CONNECT = proxy communication failure
ERROR_PROXY = proxy operation error
```

## 4. Data Structures

### 4.1 Crossover Index Entry Structure
Each crossover transaction entry must contain:
- **Completion Status**: Boolean flag indicating transaction completion
- **Amount**: 64-bit unsigned integer for transaction value
- **Locker Key**: 16-byte identifier linking to coin locker
- **Currency Code**: 3-byte cryptocurrency identifier (e.g., "BTC")
- **Sender Address**: 32-byte cryptocurrency address
- **Receipt ID**: 16-byte unique transaction identifier
- **Memo**: Variable-length text field (up to MAX_MEMO_SIZE)
- **Address Size**: Integer indicating actual address length used
- **First Seen**: Timestamp for transaction creation and expiry calculation
- **Confirmations**: Count of blockchain confirmations received

### 4.2 Global Index Structure
```
crossover_index: array[MAX_CROSSOVER_RECORDS] of pointer_to_crossover_entry
crossover_mutex: mutex_type for thread-safe access
```

## 5. Core Index Operations

### 5.1 init_crossover_index()
**Purpose**: Initialize crossover system and launch background processing thread.

**Initialization Steps**:
1. **Mutex Initialization**: Create mutex for thread-safe index access
2. **Index Clearing**: Set all index entries to NULL
3. **Thread Launch**: Start background housekeeping thread
4. **Error Handling**: Return appropriate error codes for failures

### 5.2 add_crossover_index_entry(parameters)
**Purpose**: Add new crossover transaction to the index.

**Parameters**:
- locker_key: 16-byte locker identifier
- currency_code: 3-byte currency identifier
- amount: 64-bit transaction amount
- sender_address: 32-byte sender address
- address_size: actual address length
- receipt_id: 16-byte receipt identifier
- memo: variable-length memo text
- memo_length: memo text length

**Processing Logic**:
1. **Slot Allocation**: Find first available index slot
2. **Memory Allocation**: Allocate crossover entry structure
3. **Data Population**: Copy all provided parameters to entry
4. **Timestamp Setting**: Record current time as first_seen
5. **Index Registration**: Place entry in allocated slot
6. **Thread Safety**: Use mutex protection throughout

**Return Values**:
- 0: Success
- -1: Memory allocation failure
- -2: Index full (no available slots)

### 5.3 get_crossover_index_entry(locker_key)
**Purpose**: Retrieve crossover entry by locker key.

**Processing Logic**:
1. **Index Search**: Linear search through all active entries
2. **Key Comparison**: Compare 16-byte locker keys for exact match
3. **Thread Safety**: Use mutex protection during search
4. **Return Management**: Return pointer to found entry or NULL

## 6. Optimized Background Processing

### 6.1 crossover_thread()
**Purpose**: Background thread with dynamic sleep scheduling for efficient processing.

**Optimization Algorithm**:
1. **Immediate Cleanup**: Remove any expired transactions
2. **Expiry Calculation**: Find next transaction expiry time
3. **Dynamic Sleep**: Calculate sleep duration based on closest expiry
4. **Maximum Limit**: Cap sleep duration at CROSSOVER_HOUSEKEEPING_PERIOD
5. **Responsive Wakeup**: Sleep for minimal time if transactions already expired

**Sleep Duration Logic**:
- If next_expiry > current_time: sleep = next_expiry - current_time
- If next_expiry <= current_time: sleep = 1 second (immediate processing)
- If no active transactions: sleep = CROSSOVER_HOUSEKEEPING_PERIOD
- Maximum sleep duration capped at CROSSOVER_HOUSEKEEPING_PERIOD

### 6.2 housekeeping_crossover_index()
**Purpose**: Remove expired transactions from the index.

**Processing Logic**:
1. **Current Time**: Get current timestamp
2. **Index Iteration**: Check each active entry for expiry
3. **Expiry Check**: Compare (first_seen + CROSSOVER_EXPIRY) with current time
4. **Resource Cleanup**: Free expired entries and clear index slots
5. **Thread Safety**: Use mutex protection during cleanup

## 7. Cryptocurrency Operations

### 7.1 check_depository(parameters)
**Purpose**: Verify cryptocurrency transaction status and confirmations.

**Parameters**:
- locker_key: 16-byte locker identifier
- currency_code: 3-byte currency identifier
- transaction_id: 32-byte blockchain transaction ID
- receipt_id: 16-byte receipt identifier
- memo: variable-length memo text
- memo_length: memo text length

**Processing Logic**:
1. **Entry Retrieval**: Get crossover entry by locker key
2. **Parameter Validation**: Verify receipt ID and currency code match
3. **Request Construction**: Build proxy request with transaction details
4. **Proxy Communication**: Send watch transaction request to proxy
5. **Response Processing**: Parse amount and confirmation count
6. **Status Update**: Update entry with confirmation information

**Return Handling**:
- STATUS_SUCCESS: Transaction confirmed
- STATUS_TX_SEEN: Transaction detected but not confirmed
- ERROR_NO_ENTRY: Crossover entry not found
- ERROR_INVALID_PARAMETER: Parameter validation failure
- ERROR_AMOUNT_MISMATCH: Transaction amount doesn't match

### 7.2 get_exchange_rate(currency_code, exchange_rate_output)
**Purpose**: Retrieve current cryptocurrency exchange rate.

**Processing Logic**:
1. **Currency Validation**: Verify currency code (currently supports "BTC")
2. **Proxy Request**: Send rate request to proxy service
3. **Response Processing**: Extract 64-bit exchange rate value
4. **Byte Order**: Handle big-endian to host byte order conversion
5. **Output Setting**: Set exchange rate output parameter

### 7.3 withdraw_from_depository(parameters)
**Purpose**: Execute cryptocurrency withdrawal transaction.

**Parameters**:
- locker_key: 16-byte locker identifier
- target_address: destination cryptocurrency address
- address_size: target address length
- conversion_cost: 64-bit withdrawal amount
- currency_code: 3-byte currency identifier
- receipt_id: 16-byte receipt identifier
- memo: variable-length memo text
- memo_length: memo text length

**Processing Logic**:
1. **Currency Validation**: Verify supported currency type
2. **Key Retrieval**: Get cryptocurrency private key using get_crypto_key()
3. **Key Size Validation**: Ensure key size within limits
4. **Request Construction**: Build withdrawal request with all parameters
5. **Proxy Communication**: Send transaction request to proxy
6. **Response Handling**: Process success or waiting status

**Security Considerations**:
- Private key retrieved from secure filesystem
- Key memory freed immediately after use
- Encrypted communication with proxy service

## 8. Secure Proxy Communication

### 8.1 proxy_request(command, body, body_size, output_length, status)
**Purpose**: Handle secure communication with external proxy services.

**Parameters**:
- command_no: integer command identifier
- body: pointer to request body data
- body_size: size of request body
- output_length: pointer for response length output
- status: pointer for status code output

**Connection Management**:
1. **Socket Creation**: Create TCP socket for proxy communication
2. **Non-Blocking Connect**: Use non-blocking connection with timeout
3. **Connection Validation**: Verify connection establishment with select()
4. **Blocking Mode**: Switch to blocking mode for data transfer

**Request Construction**:
1. **Header Building**: Construct 32-byte request header
2. **Encryption Setup**: Use AES encryption with proxy key
3. **Body Encryption**: Encrypt request body and challenge
4. **Trailer Addition**: Add 2-byte trailer (0x3e, 0x3e)

**Response Processing**:
1. **Header Reception**: Read 32-byte response header
2. **Status Extraction**: Get operation status from header
3. **Body Reception**: Read variable-length response body
4. **Trailer Validation**: Verify 2-byte trailer presence
5. **Body Decryption**: Decrypt response body content

**Error Handling**:
- Connection timeout: Return ERROR_PROXY_CONNECT
- Invalid response: Return appropriate error code
- Memory allocation failure: Clean up and return NULL
- Network errors: Close socket and return error

## 9. Memory Management

### 9.1 Dynamic Allocation Strategy
- **Index Entries**: Allocated on-demand for active transactions
- **Response Buffers**: Sized based on actual response length
- **Proxy Requests**: Temporary allocation for request construction
- **Cryptocurrency Keys**: Temporary allocation with immediate cleanup

### 9.2 Resource Cleanup Requirements
- **Entry Expiry**: Free expired crossover entries automatically
- **Network Resources**: Close sockets on all exit paths
- **Memory Buffers**: Free all allocated buffers on error conditions
- **Security Data**: Immediately free sensitive data like private keys

### 9.3 Thread Safety
- **Mutex Protection**: All index operations use mutex synchronization
- **Atomic Operations**: Entry allocation and deallocation are atomic
- **Resource Sharing**: Safe sharing of read-only configuration data

## 10. Error Handling and Recovery

### 10.1 Network Error Management
- **Connection Failures**: Retry logic not implemented, return appropriate errors
- **Timeout Handling**: Use select() for non-blocking connection management
- **Socket Errors**: Log specific error messages and clean up resources
- **Proxy Unavailability**: Return ERROR_PROXY_CONNECT for connection issues

### 10.2 Transaction Error Management
- **Invalid Parameters**: Validate all input parameters before processing
- **Missing Entries**: Return ERROR_NO_ENTRY for unfound transactions
- **Amount Mismatches**: Verify transaction amounts match expectations
- **Confirmation Delays**: Handle STATUS_TX_SEEN for pending confirmations

### 10.3 System Error Management
- **Memory Exhaustion**: Return appropriate error codes for allocation failures
- **Index Overflow**: Return -2 when index reaches capacity
- **Thread Failures**: Log thread creation errors and return system errors
- **Key Management**: Handle missing or invalid cryptocurrency keys

## 11. Integration Requirements

### 11.1 Configuration Dependencies
- **Proxy Settings**: Access to proxy_addr, proxy_port, proxy_key
- **Network Settings**: Access to coin_id, raida_no configuration
- **Security Settings**: Access to btc_confirmations threshold
- **Filesystem Access**: Integration with get_crypto_key() function

### 11.2 Cryptographic Dependencies
- **AES Encryption**: crypt_ctr() function for proxy communication
- **CRC Calculation**: crc32b() function for request integrity
- **Byte Order**: swap_uint64() for network byte order conversion
- **Network Utilities**: put_u32() for data serialization

### 11.3 Network Dependencies
- **Socket Management**: set_nonblocking(), set_blocking() functions
- **Address Resolution**: inet_pton() for IP address conversion
- **Timeout Handling**: setsockopt() for socket timeout configuration
- **Connection Management**: Standard TCP socket operations

## 12. Performance Considerations

### 12.1 Optimization Features
- **Dynamic Sleep**: Background thread sleeps based on actual transaction expiry times
- **Minimal Locking**: Mutex usage limited to critical sections
- **Efficient Search**: Linear search acceptable for reasonable record limits
- **Resource Reuse**: Index slots reused after transaction expiry

### 12.2 Scalability Factors
- **Index Size**: MAX_CROSSOVER_RECORDS determines concurrent transaction limit
- **Memory Usage**: Each entry consumes fixed memory footprint
- **Thread Overhead**: Single background thread minimizes resource usage
- **Network Efficiency**: Persistent connections not implemented (single-use)

### 12.3 Response Time Optimization
- **Immediate Processing**: Expired transactions processed immediately
- **Responsive Scheduling**: Background thread wakes up when needed
- **Network Timeouts**: Configurable timeouts prevent hanging operations
- **Error Fast-Path**: Quick return for common error conditions

This specification provides complete implementation guidance for the CloudCoin crossover system while remaining language-agnostic and accurately reflecting the optimized time-based processing architecture.