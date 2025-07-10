# Specification: RAIDAX Authentication Commands Implementation (cmd_auth.c)

## 1. Module Purpose
This implementation file provides cryptocurrency authentication and ownership verification commands for the RAIDAX system, part of the CloudCoinConsortium project. It implements coin detection, XOR sum validation, proof of ownership operations, and multi-shard support for different coin types including legacy CloudCoins and CloudCoins 2.0.

## 2. System Architecture Overview

### 2.1 Implementation Components
- **Coin Detection Operations**: Authentication verification for individual coins and coin batches
- **XOR Sum Validation**: Cryptographic proof of ownership through XOR sum calculations
- **Proof of Ownership (POWN)**: Ownership verification with authentication number updates
- **Multi-Shard Support**: Support for different coin types through external shard-specific functions
- **Bitmap Result Generation**: Efficient result tracking for batch operations
- **Statistics Integration**: Operation counting and value tracking

### 2.2 Security Model
- **Authentication Verification**: Cryptographic comparison of authentication numbers
- **Ownership Proof**: XOR sum validation for batch ownership verification
- **Page-based Security**: Thread-safe page access with locking mechanisms
- **Atomic Operations**: Transaction-like operations for authentication number updates
- **Result Integrity**: Bitmap-based result tracking for partial success scenarios

### 2.3 External Function Dependencies
- **Database Functions**: Calls existing page management functions (get_page_by_sn_lock, unlock_page, add_page_to_dirty_queue)
- **Legacy Support Functions**: Calls external functions (legacy_detect) for CloudCoin compatibility
- **CloudCoin 2.0 Functions**: Calls external functions (cc2_detect) for next-generation coin support
- **Utility Functions**: Calls data conversion functions (get_sn, get_mfs, get_den_value)
- **Statistics Functions**: Calls external functions (inc_stat) for operation tracking
- **Index Functions**: Calls external function (update_index) for search index maintenance
- **Memory Functions**: Uses standard memory allocation and manipulation functions

## 3. System Constants and Configuration

### 3.1 Request Size Constants
```
DETECT_MIN_SIZE = 39 bytes             // 16 challenge + 21 coin data + 2 EOF
DETECT_SUM_MIN_SIZE = 39 bytes         // 16 challenge + 5 coin + 16 sum + 2 EOF
POWN_MIN_SIZE = 55 bytes               // 16 challenge + 37 coin data + 2 EOF
POWN_SUM_MIN_SIZE = 55 bytes           // 16 challenge + 5 coin + 16 sum + 16 new + 2 EOF
```

### 3.2 Data Structure Constants
```
COIN_DETECT_RECORD_SIZE = 21 bytes     // 1 denomination + 4 serial + 16 authentication
COIN_SN_RECORD_SIZE = 5 bytes          // 1 denomination + 4 serial number
POWN_RECORD_SIZE = 37 bytes            // 1 denomination + 4 serial + 16 old auth + 16 new auth
AUTHENTICATION_NUMBER_SIZE = 16 bytes   // Standard authentication number size
XOR_SUM_SIZE = 16 bytes                // XOR sum validation size
```

### 3.3 Shard Type Constants
```
SHARD_CLOUDCOIN = legacy CloudCoin identifier
SHARD_SUPERCOIN = CloudCoin 2.0 identifier
SHARD_DEFAULT = standard RAIDA coin processing
```

### 3.4 Statistics Field Constants
```
POWN_FIELD_IDX = statistics index for POWN operation count
POWN_VALUE_FIELD_IDX = statistics index for POWN operation value
```

## 4. Core Command Implementations

### 4.1 Detect Command Implementation
**Purpose**: Authenticate coins by comparing stored and provided authentication numbers.

**Function Name**: cmd_detect

**Request Validation**:
1. **Minimum Size Check**: Verify request size at least 39 bytes
2. **Record Count Calculation**: Calculate number of coins as (body_size - 18) / 21
3. **Data Alignment Check**: Verify remaining data length is divisible by 21

**Processing Implementation**:
1. **Response Buffer Allocation**: Use standard memory functions to allocate bitmap buffer for results
2. **Buffer Initialization**: Initialize bitmap buffer to zero (all failed) using memory functions
3. **Shard Type Processing**:
   - **Legacy CloudCoin Shard**: Call external function legacy_detect() for CloudCoin compatibility
   - **SuperCoin Shard**: Call external function cc2_detect() for CloudCoin 2.0 processing
   - **Default Shard**: Implement standard RAIDA coin authentication processing
4. **Standard Authentication Loop** (for default shard):
   - Extract denomination and serial number using internal parsing
   - Call external database function get_page_by_sn_lock() to access coin page
   - Compare stored authentication number with provided authentication using memory comparison
   - Set appropriate bit in result bitmap for successful authentication
   - Call external database function unlock_page() to release page lock
5. **Result Classification**:
   - **All Pass**: All coins successfully authenticated
   - **All Fail**: No coins successfully authenticated  
   - **Mixed Results**: Partial success with bitmap output

**Bitmap Implementation**:
- Calculate bit position as coin_index % 8
- Calculate byte position as coin_index / 8
- Set bit using bitwise OR operation for successful authentications

### 4.2 Detect Sum Command Implementation
**Purpose**: Validate coin ownership through XOR sum cryptographic proof.

**Function Name**: cmd_detect_sum

**Request Validation**:
1. **Minimum Size Check**: Verify request size at least 39 bytes
2. **Record Count Calculation**: Calculate number of coins as (body_size - 34) / 5
3. **Data Alignment Check**: Verify remaining data length is divisible by 5

**Processing Implementation**:
1. **XOR Accumulator Initialization**: Initialize 16-byte XOR buffer to zero using memory functions
2. **Coin Processing Loop**:
   - Extract denomination and serial number using internal parsing
   - Call external database function get_page_by_sn_lock() to access coin page
   - XOR stored authentication number into accumulator using bitwise operations
   - Call external database function unlock_page() to release page lock
3. **Sum Validation**: Compare calculated XOR sum with provided sum using memory comparison
4. **Result Determination**:
   - **All Pass**: XOR sums match, proving ownership of all coins
   - **All Fail**: XOR sums do not match, indicating counterfeit or missing coins

**Cryptographic Properties**:
- XOR sum serves as proof of ownership for entire coin batch
- Cannot be calculated without access to all authentication numbers
- Provides efficient batch validation without individual coin verification

### 4.3 POWN Command Implementation
**Purpose**: Proof of ownership with authentication number update for individual coins.

**Function Name**: cmd_pown

**Request Validation**:
1. **Minimum Size Check**: Verify request size at least 55 bytes
2. **Record Count Calculation**: Calculate number of coins as (body_size - 18) / 37
3. **Data Alignment Check**: Verify remaining data length is divisible by 37

**Processing Implementation**:
1. **Months From Start**: Call external utility function get_mfs() for current time value
2. **Response Buffer Allocation**: Use standard memory functions to allocate bitmap buffer for results
3. **Buffer Initialization**: Initialize bitmap buffer to zero using memory functions
4. **Coin Processing Loop**:
   - Extract denomination, serial number, old authentication, and new authentication using internal parsing
   - Call external database function get_page_by_sn_lock() to access coin page
   - Compare stored authentication number with provided old authentication using memory comparison
   - **On Successful Authentication**:
     - Set bit in result bitmap for successful verification
     - Copy new authentication number to coin record using memory functions
     - Set months-from-start status using external utility function result
     - Call external database function add_page_to_dirty_queue() for persistence
     - Call external statistics function inc_stat() for operation counting
     - Call external statistics function inc_stat() with denomination value for value tracking
   - Call external database function unlock_page() to release page lock
5. **Index Update**: Call external function update_index() to maintain search indexes
6. **Result Classification**: Same as detect command (All Pass/All Fail/Mixed)

**Statistics Integration**:
- Count successful POWN operations using external statistics system
- Track total value of POWN operations using denomination values

### 4.4 POWN Sum Command Implementation
**Purpose**: Batch proof of ownership with XOR sum validation and authentication number updates.

**Function Name**: cmd_pown_sum

**Request Validation**:
1. **Minimum Size Check**: Verify request size at least 55 bytes
2. **Record Count Calculation**: Calculate number of coins as (body_size - 50) / 5
3. **Data Alignment Check**: Verify remaining data length is divisible by 5

**Processing Implementation**:
1. **Months From Start**: Call external utility function get_mfs() for current time value
2. **XOR Sum Validation**:
   - Initialize 16-byte XOR buffer to zero using memory functions
   - For each coin: extract denomination and serial, access page, XOR authentication number
   - Compare calculated XOR sum with provided sum using memory comparison
3. **Batch Authentication Update** (if XOR sum valid):
   - For each coin: access page again, XOR new authentication data into stored authentication
   - Set months-from-start status for each coin
   - Call external database function add_page_to_dirty_queue() for each modified page
   - Call external statistics functions for operation and value tracking
4. **Index Update**: Call external function update_index() to maintain search indexes
5. **Result Determination**:
   - **All Pass**: XOR sum valid, all authentications updated
   - **All Fail**: XOR sum invalid, no changes made

**Security Features**:
- Atomic batch operation: either all coins updated or none
- XOR sum validation prevents partial coin set attacks
- Efficient batch processing with single validation step

## 5. Error Handling and Security Implementation

### 5.1 Request Validation
- **Size Validation**: Exact minimum size requirements for all commands
- **Data Alignment**: Verify coin count calculations align with request data
- **Parameter Bounds**: Validate all extracted parameters within acceptable ranges
- **Memory Allocation**: Verify successful response buffer allocation

### 5.2 Authentication Security
- **Constant-Time Comparison**: Use secure memory comparison for authentication numbers
- **Page Locking**: Thread-safe access to coin data through database page system
- **Atomic Updates**: Ensure authentication updates occur atomically
- **Error Isolation**: Handle individual coin failures without affecting others

### 5.3 Cryptographic Security
- **XOR Sum Integrity**: Cryptographic proof of ownership for batch operations
- **Authentication Verification**: Secure comparison of authentication numbers
- **Replay Protection**: Time-based months-from-start values prevent replay attacks
- **Partial Success Handling**: Bitmap results for mixed authentication scenarios

## 6. Multi-Shard Integration

### 6.1 Shard Type Detection
- **Shard Identifier**: Use connection shard_id to determine coin type
- **Legacy Support**: Route CloudCoin requests to external legacy_detect() function
- **Next Generation**: Route CloudCoin 2.0 requests to external cc2_detect() function
- **Default Processing**: Handle standard RAIDA coins with internal logic

### 6.2 External Shard Functions
- **Legacy CloudCoin**: Call external function legacy_detect(payload, total_coins, &p, &f, output)
- **CloudCoin 2.0**: Call external function cc2_detect(payload, total_coins, &p, &f, output)
- **Parameter Passing**: Pass payload, coin count, pass/fail counters, and result bitmap
- **Status Handling**: Handle return status codes from external shard functions

### 6.3 Result Integration
- **Unified Results**: All shard types produce same result format
- **Pass/Fail Counting**: Consistent counting across all shard implementations
- **Bitmap Format**: Standard bitmap format for all authentication results
- **Status Codes**: Unified status code handling for all shard types

## 7. Integration Requirements

### 7.1 Database Integration
- **Page Management**: Call external functions get_page_by_sn_lock(), unlock_page() for coin access
- **Data Persistence**: Call external function add_page_to_dirty_queue() for modified pages
- **Record Access**: Calculate coin record offsets within pages using internal logic
- **Thread Safety**: Proper page locking for concurrent access protection

### 7.2 Utility Integration
- **Data Conversion**: Call external function get_sn() for serial number extraction
- **Time Management**: Call external function get_mfs() for months-from-start calculation
- **Value Calculation**: Call external function get_den_value() for denomination value lookup
- **Memory Operations**: Use standard memory functions for data manipulation

### 7.3 Statistics Integration
- **Operation Counting**: Call external function inc_stat() for POWN operation tracking
- **Value Tracking**: Call external function inc_stat() with denomination values for financial tracking
- **Performance Metrics**: Integrate with system-wide statistics collection
- **Audit Trail**: Provide operation counts for system monitoring

### 7.4 Index Maintenance
- **Search Index**: Call external function update_index() after authentication updates
- **Data Consistency**: Ensure search indexes reflect authentication changes
- **Performance Optimization**: Maintain indexes for efficient coin lookups
- **System Integration**: Coordinate with system-wide indexing mechanisms

## 8. Performance Considerations

### 8.1 Memory Management
- **Dynamic Allocation**: Use standard memory functions for response buffer allocation
- **Buffer Sizing**: Calculate exact buffer sizes for different coin counts
- **Memory Efficiency**: Minimize memory usage for large coin batches
- **Resource Cleanup**: Automatic cleanup through connection management system

### 8.2 Database Access Optimization
- **Page Lock Duration**: Minimize database page lock time
- **Efficient Lookups**: Use optimized coin lookup by denomination and serial number
- **Batch Operations**: Process multiple coins efficiently within single operations
- **Resource Management**: Proper page unlocking and cleanup

### 8.3 Cryptographic Performance
- **XOR Operations**: Efficient XOR calculations for sum validation
- **Memory Comparison**: Optimized memory comparison for authentication verification
- **Batch Processing**: Process multiple authentications efficiently
- **Algorithm Efficiency**: Use efficient algorithms for bitmap manipulation

## 9. Result Format and Status Codes

### 9.1 Success Status Codes
- **STATUS_ALL_PASS**: All coins successfully authenticated or processed
- **STATUS_ALL_FAIL**: No coins successfully authenticated or processed
- **STATUS_MIXED**: Partial success with bitmap indicating individual results

### 9.2 Error Status Codes
- **ERROR_INVALID_PACKET_LENGTH**: Request size validation failure
- **ERROR_COINS_NOT_DIV**: Coin count calculation alignment failure
- **ERROR_INVALID_SN_OR_DENOMINATION**: Invalid coin parameters
- **ERROR_MEMORY_ALLOC**: Response buffer allocation failure
- **ERROR_INTERNAL**: Internal processing error

### 9.3 Result Format
- **All Pass/All Fail**: Status code only, no additional data
- **Mixed Results**: Bitmap indicating success (1) or failure (0) for each coin
- **Bitmap Size**: (total_coins / 8) + 1 bytes for mixed results
- **Bit Ordering**: Least significant bit represents first coin

This specification provides complete implementation guidance for RAIDAX authentication commands while clearly distinguishing between external function calls and internal implementation, accurately reflecting the multi-shard support, cryptographic validation, and database integration essential for secure cryptocurrency authentication operations.