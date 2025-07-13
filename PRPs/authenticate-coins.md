# Authentication Commands Implementation (cmd_auth)

## Module Purpose
This module implements the core authentication and proof-of-ownership commands for the RAIDA network. It provides coin verification, authenticity checking, ownership transfer operations, and supports both individual coin operations and efficient batch operations using cryptographic sum verification. The module handles multi-shard operations supporting legacy CloudCoin and SuperCoin systems alongside the current RAIDA system.

## status codes 
| Constant          | Description                                                   |
| ----------------- | ------------------------------------------------------------- |
| `STATUS_ALL_PASS` | All coins passed validation (fully authentic or updated)      |
| `STATUS_ALL_FAIL` | No coins passed validation                                    |
| `STATUS_MIXED`    | Some coins passed, others failed (partial success)            |
| `STATUS_SUCCESS`  | General success code used by legacy systems (`legacy_detect`) |

## coin record format 
| Format Name            | Size (bytes) | Structure Breakdown                                      |
| ---------------------- | ------------ | -------------------------------------------------------- |
| `Detection Record`     | 21           | 1-byte Den + 4-byte SN + 16-byte Auth Number             |
| `Ownership Record`     | 37           | 1-byte Den + 4-byte SN + 16-byte Old AN + 16-byte New AN |
| `Detection Sum Record` | 5            | 1-byte Den + 4-byte SN                                   |
| `Ownership Sum Record` | 5            | 1-byte Den + 4-byte SN                                   |
| `XOR Block` / `Delta`  | 16           | 128-bit XOR sum or delta value                           |

## database/page constraint 
| Constant           | Description                                                         |
| ------------------ | ------------------------------------------------------------------- |
| `RECORDS_PER_PAGE` | Number of coin entries per page; used to compute coin's page offset |
| `page->data`       | Flat array storing records (each 17 bytes: 16-byte AN + 1-byte MFS) |
| `page->is_dirty`   | Flag marking page as changed; triggers disk persistence later       |

## statistical indexes 
| Constant               | Description                                      |
| ---------------------- | ------------------------------------------------ |
| `POWN_FIELD_IDX`       | Tracks count of successful ownership changes     |
| `POWN_VALUE_FIELD_IDX` | Tracks total value (by denomination) transferred |

## Core Functionality

### 1. Individual Coin Detection (`cmd_detect`)
**Parameters:**
- Connection information structure containing request data
- Input: Variable-length payload (minimum 39 bytes) containing coin authentication data

**Returns:** None (modifies connection structure with detection results)

**Purpose:** Verifies the authenticity of a list of coins by comparing their provided authentication numbers against stored values.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (39 bytes minimum)
   - Calculates coin count from payload size (21 bytes per coin record)
   - Validates coin data alignment

2. **Shard-Specific Processing:**
   - **SHARD_CLOUDCOIN:** Uses legacy_detect for CloudCoin v1 system
   - **SHARD_SUPERCOIN:** Uses cc2_detect for CloudCoin v2 system
   - **Current System:** Processes using on-demand page cache

3. **Current System Authentication Logic:**
   - For each coin in the request:
     - Extracts denomination and serial number (5 bytes)
     - Retrieves coin page using on-demand cache system
     - Compares provided authentication number (16 bytes) with stored value
     - Sets bit in response bitmap for authentic coins
     - Counts passed and failed coins

4. **Response Generation:**
   - **STATUS_ALL_PASS:** All coins authentic
   - **STATUS_ALL_FAIL:** No coins authentic  
   - **STATUS_MIXED:** Partial success with detailed bitmap response

**Input Format:** Each coin record contains:
- 1 byte: Denomination identifier
- 4 bytes: Serial number
- 16 bytes: Authentication number to verify

**Output Format:** Bitmap where each bit indicates coin authenticity (1 = authentic, 0 = not authentic)

**Dependencies:**
- Database layer for on-demand page access
- Legacy system interfaces for CloudCoin/SuperCoin detection
- Statistics system for operation tracking

### 2. Batch Authentication with Sum Verification (`cmd_detect_sum`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 39 bytes) with coin list and XOR sum

**Returns:** None (modifies connection structure with verification result)

**Purpose:** Efficiently verifies authenticity of coin batches using cryptographic XOR sum instead of individual comparisons.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size and coin alignment
   - Calculates coin count (5 bytes per coin: 1 byte denomination + 4 bytes serial number)

2. **Cryptographic Verification:**
   - For each coin in the batch:
     - Retrieves coin data using on-demand page cache
     - XORs the stored authentication number into running sum
   - Compares computed XOR sum with provided sum (16 bytes)

3. **Result Determination:**
   - **Identical sums:** All coins authentic (STATUS_ALL_PASS)
   - **Different sums:** Batch not authentic (STATUS_ALL_FAIL)

**Security Features:**
- XOR sum verification proves possession of all coins in batch
- Prevents selective authentication attacks
- Efficient batch processing for large coin collections

**Performance Benefits:**
- Single pass through coin data
- No individual coin result tracking
- Reduced response size (single status vs. bitmap)

### 3. Individual Coin Ownership Transfer (`cmd_pown`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) with coin data and new authentication numbers

**Returns:** None (modifies connection structure with ownership transfer results)

**Purpose:** Takes ownership of coins by verifying current authentication numbers and updating to new values.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size and coin record alignment
   - Each coin record is 37 bytes (5 bytes coin ID + 16 bytes current AN + 16 bytes new AN)

2. **Ownership Transfer Logic:**
   - For each coin:
     - Verifies current authentication number matches stored value
     - If authentic, updates authentication number to proposed new value
     - Sets MFS (Months From Start) timestamp
     - Marks database page as dirty for persistence
     - Updates statistics for successful ownership transfers

3. **Response Generation:**
   - Bitmap indicating success/failure for each coin
   - Status based on overall operation results

**Security Features:**
- Requires proof of current ownership before transfer
- Atomic operation per coin (either succeeds completely or fails)
- Audit trail through MFS timestamps and statistics

**Used By:**
- Client authentication and ownership establishment
- Coin transfer operations between users
- Recovery operations after healing

### 4. Batch Ownership Transfer with Delta (`cmd_pown_sum`)
**Parameters:**
- Connection information structure  
- Input: Variable-length payload (minimum 55 bytes) with coin list, sum verification, and delta

**Returns:** None (modifies connection structure with batch transfer result)

**Purpose:** Efficiently transfers ownership of coin batches using XOR sum verification and delta transformation.

**Process:**
1. **Batch Verification Phase:**
   - Validates coin list alignment (5 bytes per coin)
   - Computes XOR sum of all current authentication numbers
   - Compares with provided sum to verify batch ownership

2. **Batch Transformation Phase:**
   - If verification succeeds, applies delta (XOR) to all coins:
     - For each coin, XORs stored authentication number with provided delta
     - Updates MFS timestamp for all modified coins
     - Marks all affected pages as dirty
     - Updates statistics for batch operation

3. **Atomic Batch Operation:**
   - **Verification Success:** All coins transformed (STATUS_ALL_PASS)
   - **Verification Failure:** No coins modified (STATUS_ALL_FAIL)

**Cryptographic Security:**
- XOR sum proves ownership of entire batch
- Delta transformation allows efficient batch updates
- All-or-nothing operation prevents partial state corruption

**Performance Advantages:**
- Single verification pass followed by single transformation pass
- Efficient for large batches of related coins
- Reduced network overhead compared to individual operations

## Multi-Shard Support

### Legacy System Integration
- **CloudCoin v1 (SHARD_CLOUDCOIN):** Complete integration with legacy detection system
- **SuperCoin (SHARD_SUPERCOIN):** Support for CloudCoin v2 detection protocols
- **Current RAIDA:** Native on-demand cache-based processing

### Shard Detection Logic
- Commands automatically route to appropriate shard handlers
- Legacy system results integrated seamlessly with current protocol
- Error handling consistent across all shard types

## Data Structures and Formats

### Coin Record Formats
- **Detection Record:** 21 bytes (1 + 4 + 16: denomination + serial + authentication)
- **Sum Detection Record:** 5 bytes (1 + 4: denomination + serial)  
- **Ownership Record:** 37 bytes (1 + 4 + 16 + 16: denomination + serial + current AN + new AN)
- **Sum Ownership Record:** 5 bytes (1 + 4: denomination + serial)

### Response Formats
- **Individual Results:** Bitmap with one bit per coin
- **Batch Results:** Single status code for entire operation
- **Mixed Results:** Bitmap plus operation count for partial success

### Cryptographic Elements
- **Authentication Numbers:** 16-byte unique identifiers proving coin ownership
- **XOR Sums:** 16-byte cryptographic verification of batch ownership
- **Delta Values:** 16-byte transformation applied to authentication numbers

## Security Considerations

### Authentication Security
- **Proof of Ownership:** All operations require demonstration of current coin ownership
- **Cryptographic Verification:** XOR sums provide tamper-evident batch verification
- **Atomic Operations:** Ownership transfers either succeed completely or fail entirely

### Data Integrity
- **Page Locking:** Database page locking ensures consistent coin state during operations
- **MFS Timestamps:** All ownership changes timestamped for audit trail
- **Dirty Page Tracking:** Modified data marked for reliable persistence

### Attack Prevention
- **Selective Authentication:** Batch operations prevent cherry-picking authentic coins
- **Replay Protection:** Authentication numbers change with each ownership transfer
- **State Validation:** All coin references validated before processing

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for minimum and alignment requirements
- **Coin Validation:** Denomination and serial number range checking
- **Authentication Validation:** Authentication number format verification

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size or alignment
- `ERROR_COINS_NOT_DIV`: Coin data not properly aligned to record boundaries
- `ERROR_INVALID_SN_OR_DENOMINATION`: Invalid coin reference
- `ERROR_MEMORY_ALLOC`: Memory allocation failure for response buffers
- `ERROR_INTERNAL`: Internal processing error during ownership transfer


### Recovery Mechanisms
- **Partial Success Handling:** Mixed operations report individual coin results
- **Resource Cleanup:** Memory and page locks released on error conditions
- **State Consistency:** Failed operations leave coin state unchanged

## Performance Characteristics

### On-Demand Cache Integration
- **Cache Efficiency:** Frequently accessed coins remain in memory
- **Memory Conservation:** Only accessed pages loaded into cache
- **I/O Optimization:** Batch operations minimize page cache pressure

### Batch Operation Benefits
- **Reduced Network Overhead:** Single request for multiple coins
- **Improved Throughput:** Batch cryptographic operations
- **Lower Latency:** Fewer round trips for large operations

### Statistics Integration
- **Operation Counting:** All successful ownership transfers tracked
- **Value Tracking:** Total value of transferred coins recorded
- **Performance Monitoring:** Operation timing and success rates tracked

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Legacy Systems:** CloudCoin v1 and v2 detection interfaces
- **Cryptographic Utilities:** XOR operations, hash functions
- **Statistics System:** Operation counting and value tracking
- **Configuration System:** Server identification and shard configuration

### External Constants Required
- `RECORDS_PER_PAGE`: Database page organization constant
- `SHARD_*`: Shard identification constants
- `STATUS_*`: Operation result status codes
- `ERROR_*`: Error condition definitions

### Used By
- **Client Authentication:** Primary interface for proving coin ownership
- **Healing Operations:** Coin verification during recovery procedures
- **Trading Systems:** Ownership verification before transfers
- **Administrative Tools:** Bulk coin management operations

## Threading and Concurrency
- **Page Locking:** Thread-safe access to coin data through database layer
- **Atomic Operations:** Individual coin operations are atomic
- **Batch Consistency:** Batch operations maintain consistency across multiple coins
- **Resource Safety:** Proper cleanup ensures no resource leaks

This authentication module provides the fundamental security foundation for the RAIDA network, enabling secure coin ownership verification and transfer operations while supporting legacy system compatibility and optimized batch processing.