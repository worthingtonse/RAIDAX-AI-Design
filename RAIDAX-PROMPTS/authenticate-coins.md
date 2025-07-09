#  Specification: Authentication Commands Implementation

## 1. Module Purpose
This module implements the core authentication commands for the CloudCoin RAIDA system. It provides coin validation (detect) and ownership transfer (pown) operations for multiple coin formats including legacy CloudCoins, CloudCoins 2.0, and standard RAIDA coins. All commands support both individual coin processing and batch sum-based operations for efficiency.

## 2. System Architecture Overview

### 2.1 Core Components
- **Multi-Format Support**: Legacy CloudCoin, CloudCoin 2.0, and standard RAIDA coin formats
- **Batch Processing**: Individual coin validation and sum-based batch operations
- **Database Integration**: Direct page-level database access with dirty queue optimization
- **Statistics Tracking**: Performance and operation metrics for monitoring
- **Index Synchronization**: Automatic index updates after ownership changes

### 2.2 Command Categories
- **Detection Commands**: Validate coin authenticity without ownership change
- **Possession Commands**: Validate and transfer coin ownership
- **Sum Operations**: Batch processing using XOR checksums for efficiency
- **Legacy Support**: Backward compatibility with older coin formats

## 3. System Constants and Data Structures

### 3.1 Request Format Constants
```
DETECT_MIN_SIZE = 39 bytes        // Minimum detect request size
DETECT_SUM_MIN_SIZE = 39 bytes    // Minimum detect_sum request size
POWN_MIN_SIZE = 55 bytes          // Minimum pown request size
POWN_SUM_MIN_SIZE = 55 bytes      // Minimum pown_sum request size
DETECT_COIN_SIZE = 21 bytes       // Per-coin data in detect (DN + SN + AN)
DETECT_SUM_COIN_SIZE = 5 bytes    // Per-coin data in detect_sum (DN + SN)
POWN_COIN_SIZE = 37 bytes         // Per-coin data in pown (DN + SN + AN + PN)
POWN_SUM_COIN_SIZE = 5 bytes      // Per-coin data in pown_sum (DN + SN)
```

### 3.2 Status Response Codes
```
STATUS_ALL_PASS = all operations successful
STATUS_ALL_FAIL = all operations failed
STATUS_MIXED = partial success (bitmap response included)
STATUS_SUCCESS = operation completed successfully
ERROR_INVALID_PACKET_LENGTH = malformed request size
ERROR_COINS_NOT_DIV = coin data not properly aligned
ERROR_INVALID_SN_OR_DENOMINATION = invalid coin identifier
ERROR_MEMORY_ALLOC = memory allocation failure
ERROR_INTERNAL = unexpected system error
```

### 3.3 Shard Type Support
```
SHARD_CLOUDCOIN = legacy CloudCoin format
SHARD_SUPERCOIN = CloudCoin 2.0 format
Other values = standard RAIDA format
```

## 4. Core Command Implementations

### 4.1. cmd_detect
**Purpose**: Validates coin authenticity without transferring ownership.

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
1. **Shard-Specific Processing**:
   - SHARD_CLOUDCOIN: Delegate to legacy_detect function
   - SHARD_SUPERCOIN: Delegate to cc2_detect function
   - Other shards: Process using standard RAIDA format

2. **Standard RAIDA Processing**:
   - For each coin:
     - Extract denomination (1 byte) and serial number (4 bytes)
     - Lock database page containing the coin
     - Compare stored AN with provided AN (16 bytes)
     - Set success bit in response bitmap if match
     - Unlock page

3. **Response Generation**:
   - Allocate bitmap: (total_coins / 8) + 1 bytes
   - Set bit N to 1 if coin N is authentic, 0 if not
   - Track pass/fail counts for status determination

**Response Handling**:
- All authentic: STATUS_ALL_PASS (no bitmap)
- All failed: STATUS_ALL_FAIL (no bitmap)
- Mixed results: STATUS_MIXED (include bitmap in response)

### 4.2. cmd_detect_sum
**Purpose**: Validates multiple coins using XOR checksum for efficiency.

**Request Format**:
```
[16 bytes Challenge Header]
[5 bytes per coin: DN + 4-byte SN]
[16 bytes Sum (XOR of all ANs)]
[2 bytes EOF trailer]
Minimum size: 39 bytes (16 + 5 + 16 + 2)
```

**Validation Steps**:
- Verify request size >= 39 bytes
- Calculate coin count: (body_size - 34) / 5
- Verify coin data length is divisible by 5

**Processing Logic**:
1. **XOR Calculation**:
   - Initialize XOR buffer to all zeros (16 bytes)
   - For each coin:
     - Extract denomination and serial number
     - Lock database page containing the coin
     - XOR stored AN into running XOR buffer
     - Unlock page

2. **Authentication Verification**:
   - Compare calculated XOR with provided sum
   - If match: All coins authentic (STATUS_ALL_PASS)
   - If no match: One or more coins counterfeit (STATUS_ALL_FAIL)

**Efficiency Benefits**:
- Single comparison instead of individual coin responses
- Reduced network traffic for large coin sets
- Maintains security through cryptographic checksum

### 4.3. cmd_pown
**Purpose**: Validates coins and transfers ownership by updating ANs.

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
1. **Authentication and Update**:
   - For each coin:
     - Extract denomination, serial number, current AN, and proposed AN
     - Lock database page containing the coin
     - Compare stored AN with provided AN
     - If authentic:
       - Update stored AN to proposed AN (16 bytes)
       - Set MFS byte (coin modification flag)
       - Add page to dirty queue for persistence
       - Increment statistics counters
       - Set success bit in response bitmap
     - Unlock page

2. **Response Generation**:
   - Same bitmap logic as cmd_detect
   - Track successful ownership transfers

**Critical Operations**:
- Database page modifications must be atomic
- Dirty queue ensures asynchronous persistence
- Statistics tracking for monitoring
- Index update after all modifications

### 4.4. cmd_pown_sum
**Purpose**: Batch ownership transfer using XOR-based authentication.

**Request Format**:
```
[16 bytes Challenge Header]
[5 bytes per coin: DN + 4-byte SN]
[16 bytes Sum (XOR of current ANs)]
[16 bytes AD (XOR mask for new ANs)]
[2 bytes EOF trailer]
Minimum size: 55 bytes (16 + 5 + 16 + 16 + 2)
```

**Validation Steps**:
- Verify request size >= 55 bytes
- Calculate coin count: (body_size - 50) / 5
- Verify coin data length is divisible by 5

**Processing Logic**:
1. **Authentication Phase**:
   - Calculate XOR of all current ANs (same as detect_sum)
   - Compare with provided sum
   - If mismatch: Return STATUS_ALL_FAIL immediately

2. **Ownership Transfer Phase** (only if authenticated):
   - For each coin:
     - Lock database page
     - XOR current AN with provided AD mask
     - Update stored AN with result
     - Set MFS byte
     - Add page to dirty queue
     - Increment statistics
     - Unlock page

**Batch Efficiency**:
- Single authentication check for entire batch
- Atomic all-or-nothing ownership transfer
- Reduced computational overhead for large sets

## 5. Database Integration Requirements

### 5.1 Page Management Protocol
For each coin access:
1. **Page Retrieval**: get_page_by_sn_lock(denomination, serial_number)
2. **Data Access**: Calculate record offset: (sn % RECORDS_PER_PAGE) * 17
3. **Modification**: Update AN (bytes 0-15) and MFS (byte 16) as needed
4. **Persistence**: add_page_to_dirty_queue(page) for modified pages
5. **Release**: unlock_page(page)

### 5.2 Record Structure
Each coin record in database pages:
- Bytes 0-15: Authenticity Number (AN)
- Byte 16: Modification Flag Sequence (MFS)
- Total record size: 17 bytes

### 5.3 Error Handling
- Invalid serial numbers or denominations return ERROR_INVALID_SN_OR_DENOMINATION
- Page lock failures treated as internal errors
- Partial batch failures continue processing remaining coins

## 6. Legacy Format Support

### 6.1 CloudCoin Legacy (SHARD_CLOUDCOIN)
- Delegates processing to legacy_detect function
- Maintains compatibility with original CloudCoin format
- Returns same bitmap response format

### 6.2 CloudCoin 2.0 (SHARD_SUPERCOIN)
- Delegates processing to cc2_detect function
- Supports enhanced CloudCoin 2.0 features
- Maintains standard response format

### 6.3 Integration Points
- External functions: legacy_detect, cc2_detect
- Status code compatibility across all formats
- Unified response handling regardless of shard type

## 7. Statistics and Monitoring

### 7.1 Performance Metrics
After each successful ownership transfer:
- inc_stat(POWN_FIELD_IDX, 1): Increment operation counter
- inc_stat(POWN_VALUE_FIELD_IDX, denomination_value): Track value transferred

### 7.2 Statistical Categories
- Operation counts by command type
- Total value processed by denomination
- Success/failure ratios for monitoring
- Performance timing data

## 8. Memory Management

### 8.1 Response Buffer Allocation
- Bitmap size: (total_coins / 8) + 1 bytes
- Allocated for mixed-result responses only
- Freed automatically by protocol layer
- Error path cleanup required for allocation failures

### 8.2 Memory Safety
- All allocation failures return ERROR_MEMORY_ALLOC
- Partial operation cleanup on errors
- No memory leaks on any execution path

## 9. Index Synchronization

### 9.1 Automatic Updates
- update_index() called after pown operations
- Ensures in-memory index reflects database changes
- Critical for locker system consistency

### 9.2 Update Timing
- Called only after successful database modifications
- Synchronous update to maintain consistency
- May impact performance for large batch operations

## 10. Error Recovery and Consistency

### 10.1 Atomic Operations
- Individual coin operations are atomic
- Batch operations continue on individual coin failures
- Database consistency maintained across all scenarios

### 10.2 Failure Handling
- Page lock failures: Return internal error
- Invalid coin data: Continue with remaining coins
- Memory allocation failures: Clean up and return error
- Network interruptions: Database remains consistent

### 10.3 Consistency Guarantees
- Database modifications always precede index updates
- Dirty queue ensures eventual persistence
- No partial ownership transfers within single coin
- Bitmap responses accurately reflect actual results

## 11. Performance Considerations

### 11.1 Batch Optimization
- Sum-based commands reduce network overhead
- Single authentication check for entire batch
- Dirty queue batches database writes

### 11.2 Database Access Patterns
- Page-level locking minimizes contention
- Sequential coin processing within pages
- Asynchronous persistence via dirty queue

### 11.3 Memory Efficiency
- Bitmap responses minimize bandwidth
- Stack allocation for temporary variables
- Dynamic allocation only for response buffers

This specification provides complete implementation guidance for the CloudCoin authentication command system while remaining language-agnostic and accurately reflecting the actual implementation requirements.