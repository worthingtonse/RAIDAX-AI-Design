# Executive Command Handlers (cmd_executive)

## Module Purpose
This module implements administrative commands for coin creation and management, providing executive-level operations for coin lifecycle management. It includes functions for getting available serial numbers, creating new coins, freeing coins, deleting coins, and comprehensive coin enumeration, all updated with free pages bitmap integration for optimal performance.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_AVAILABLE_COINS` | 1029 | Maximum number of coins returned per denomination in availability queries |

## Error Codes
| Constant | Description |
|----------|-------------|
| `ERROR_INVALID_PACKET_LENGTH` | Request payload size is incorrect for the operation |
| `ERROR_ADMIN_AUTH` | Administrative authentication failed |
| `ERROR_MEMORY_ALLOC` | Failed to allocate memory for response buffer |
| `ERROR_COINS_NOT_DIV` | Coin data size not properly divisible by record size |
| `ERROR_PAGE_IS_NOT_RESERVED` | Required page is not reserved by the requesting session |
| `ERROR_INVALID_SN_OR_DENOMINATION` | Invalid coin serial number or denomination provided |
| `ERROR_FILE_NOT_EXIST` | Required data file does not exist |
| `ERROR_FILESYSTEM` | File system operation failed |

## Status Codes
| Constant | Description |
|----------|-------------|
| `STATUS_SUCCESS` | Operation completed successfully |
| `STATUS_ALL_PASS` | All coins in operation passed validation |
| `STATUS_ALL_FAIL` | All coins in operation failed validation |
| `STATUS_MIXED` | Some coins passed, others failed (partial success) |

## Core Functionality

### 1. Get Available SNs (`cmd_get_available_sns`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Administrative function that finds available serial numbers across all denominations with page reservation support for executive operations.

**Process:**
1. **Request Validation:**
   - Validates exact request size (54 bytes: 16CH + 4SI + 16AU + 16DN + 2EOF)
   - Extracts session ID for page reservation tracking
   - Validates administrative authentication key

2. **Administrative Authentication:**
   - Compares provided admin key with configured admin_key
   - Returns ERROR_ADMIN_AUTH if authentication fails
   - Ensures only authorized administrators can access

3. **Response Buffer Allocation:**
   - Allocates large buffer for comprehensive results
   - Buffer size: MAX_AVAILABLE_COINS * 10 * TOTAL_DENOMINATIONS
   - Handles worst-case scenario for all denominations

4. **Denomination Processing:**
   - Iterates through all denominations (MIN_DENOMINATION to MAX_DENOMINATION)
   - Checks denomination request flags in payload
   - Skips denominations not requested by client

5. **Page-by-Page Analysis:**
   - For each requested denomination:
     - Loads each page using get_page_by_sn_lock
     - Checks if page is already reserved by another session
     - Reserves page for current session ID if available
     - Scans page for available coins (MFS == 0)

6. **Range and Individual Coin Detection:**
   - **Range Detection:** Identifies contiguous ranges of available coins
   - **Individual Coins:** Tracks single available coins
   - **Optimization:** Groups consecutive coins into ranges for efficiency
   - **Limit Enforcement:** Stops when MAX_AVAILABLE_COINS reached

7. **Response Format:**
   - For each denomination with available coins:
     - Denomination byte
     - Number of ranges (1 byte)
     - Number of individual coins (1 byte)
     - Range data: start_sn, end_sn pairs (8 bytes each)
     - Individual coin serial numbers (4 bytes each)

**Executive Features:**
- **Page Reservation:** Automatically reserves pages for subsequent operations
- **Batch Operations:** Efficient handling of large coin sets
- **Range Optimization:** Minimizes response size through range encoding
- **Session Tracking:** Maintains session context for multi-step operations

**Used By:** Administrative tools, coin creation workflows, system management

**Dependencies:** Database layer, authentication system, session management

### 2. Create Coins (`cmd_create_coins`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Creates new coins with generated authentication numbers, supporting both legacy and modern hashing algorithms based on client encryption type.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (43 bytes)
   - Extracts session ID and validates administrative authentication
   - Calculates coin count: (body_size - 38) / 5

2. **Administrative Authentication:**
   - Verifies admin key matches configuration
   - Ensures only authorized administrators can create coins

3. **Dual Hashing Support:**
   - **Legacy Clients (encryption_type < 4):** Uses MD5-based generate_an_hash_legacy
   - **Modern Clients (encryption_type >= 4):** Uses SHA-256-based generate_an_hash
   - **Hash Input:** Combines RAIDA number, serial number, and admin key

4. **Coin Creation Process:**
   - For each coin:
     - Extracts denomination and serial number
     - Loads page and verifies session reservation
     - Generates hash input string with deterministic components
     - Applies appropriate hash algorithm based on client version
     - **Returns Old AN:** Captures existing authentication number for response
     - **Sets New AN:** Updates coin with generated authentication number
     - Sets MFS to current timestamp
     - Marks page as dirty for persistence

5. **Bitmap Integration:**
   - **NEW:** Calls update_free_pages_bitmap(den, sn, 0) for each created coin
   - Marks coins as not free in the in-memory bitmap
   - Maintains perfect synchronization between coin data and bitmap

6. **Response Construction:**
   - Returns old authentication numbers for all created coins
   - Response size: 16 * total_coins bytes
   - Enables clients to verify coin creation results

**Security Features:**
- **Administrative Control:** Requires admin key authentication
- **Session Verification:** Verifies page reservations match session
- **Deterministic Generation:** Reproducible authentication number generation
- **Cryptographic Security:** Strong hash algorithms prevent prediction

**Used By:** Administrative coin creation, system initialization, coin management

**Dependencies:** Database layer, cryptographic functions, bitmap system

### 3. Free Coins (`cmd_free_coins`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Marks coins as available again by setting their MFS to zero, making them available for future allocation.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (43 bytes)
   - Authenticates using admin key
   - Calculates coin count: (body_size - 38) / 5

2. **Coin Liberation Process:**
   - For each coin:
     - Extracts denomination and serial number
     - Loads page using get_page_by_sn_lock
     - Sets MFS byte to 0 (marks as free)
     - Marks page as dirty for persistence
     - **Bitmap Update:** Calls update_free_pages_bitmap(den, sn, 1)

3. **Consistency Maintenance:**
   - Updates both coin data and bitmap atomically
   - Ensures freed coins become immediately available
   - Maintains system-wide consistency

**Administrative Use:**
- **Coin Pool Management:** Returns coins to available pool
- **Error Recovery:** Frees coins stuck in invalid states
- **System Maintenance:** Bulk coin status management

**Used By:** Administrative tools, system maintenance, error recovery

**Dependencies:** Database layer, bitmap system, authentication

### 4. Delete Coins (`cmd_delete_coins`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Verifies coin ownership and then frees coins, providing authenticated coin deletion with audit trail.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (55 bytes)
   - Authenticates using admin key (first 16 bytes of payload)
   - Calculates coin count: (body_size - 34) / 21

2. **Authentication and Deletion:**
   - For each coin:
     - Extracts denomination, serial number, and authentication number
     - Loads page and compares stored AN with provided AN
     - If authentic:
       - Sets success bit in response buffer
       - Sets MFS to 0 (frees the coin)
       - Marks page as dirty
       - **Bitmap Update:** Updates bitmap to mark as free
     - Tracks pass/fail counts for result classification

3. **Response Generation:**
   - Returns bit-packed results showing which coins were successfully deleted
   - **STATUS_ALL_PASS:** All coins authenticated and deleted
   - **STATUS_ALL_FAIL:** No coins could be authenticated
   - **STATUS_MIXED:** Some coins deleted, returns bit-packed results

**Security and Audit:**
- **Proof of Ownership:** Requires authentication number for deletion
- **Administrative Override:** Admin key provides deletion authority
- **Audit Trail:** Logs successful and failed deletion attempts
- **Atomic Operations:** Each coin deletion is atomic

**Used By:** Administrative coin management, cleanup operations, audit functions

**Dependencies:** Database layer, authentication system, bitmap management

### 5. Get All SNs (`cmd_get_all_sns`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Comprehensive enumeration of all coins in a denomination, returning complete bitmap of coin status for administrative analysis.

**Process:**
1. **Request Validation:**
   - Validates exact request size (35 bytes)
   - Authenticates using admin key
   - Extracts requested denomination

2. **Denomination Validation:**
   - Validates denomination is within supported range
   - Ensures request is for valid denomination

3. **Bitmap Construction:**
   - Calculates total bitmap size: TOTAL_PAGES * RECORDS_PER_PAGE / 8
   - Allocates response buffer: bitmap_size + 5 bytes
   - Response format: 1-byte denomination + 4-byte size + bitmap data

4. **Direct File Scanning:**
   - **Performance Optimization:** Bypasses page cache for bulk operation
   - Directly reads page files from disk
   - For each page file:
     - Constructs file path using hierarchical structure
     - Reads complete page data
     - Examines MFS byte for each coin record

5. **Bitmap Population:**
   - For each coin with MFS != 0 (not free):
     - Calculates absolute serial number
     - Sets corresponding bit in response bitmap
     - Uses bit-packed format for efficient response

6. **Response Assembly:**
   - Returns: denomination + bitmap_size + complete bitmap
   - Bitmap represents every possible coin in denomination
   - Efficient representation of entire denomination state

**Administrative Applications:**
- **System Analysis:** Complete view of denomination utilization
- **Capacity Planning:** Understanding of available coin space
- **Data Migration:** Bulk data export for system migrations
- **Audit Operations:** Comprehensive coin status verification

**Performance Considerations:**
- **Direct I/O:** Bypasses cache for bulk operations
- **Sequential Access:** Optimized for sequential file reading
- **Memory Efficient:** Bit-packed response minimizes memory usage
- **Scalable Design:** Handles large denominations efficiently

**Used By:** Administrative analysis tools, system monitoring, data export utilities

**Dependencies:** File system access, denomination utilities, authentication

## Administrative Security Model

### Authentication Requirements
- **Admin Key Verification:** All operations require valid administrative key
- **Two-Factor Authentication:** Admin key + operation-specific validation
- **Session-Based Security:** Session IDs provide additional security layer
- **Audit Logging:** All administrative operations logged for security

### Authorization Levels
- **Executive Access:** Full coin lifecycle management capabilities
- **System Administration:** Complete denomination management
- **Audit Authority:** Read-only comprehensive system analysis
- **Emergency Operations:** Special session handling for system recovery

### Security Boundaries
- **Administrative Isolation:** Administrative operations isolated from user operations
- **Resource Protection:** Administrative operations respect system resource limits
- **Data Integrity:** All operations maintain data consistency
- **Recovery Safety:** Operations designed for safe system recovery

## Page Reservation System Integration

### Session Management
- **Automatic Reservation:** Available coins automatically reserved for sessions
- **Timeout Handling:** Reservations expire after configured timeout
- **Conflict Resolution:** Handles reservation conflicts gracefully
- **Resource Cleanup:** Automatic cleanup of expired reservations

### Multi-Step Operations
- **Operation Continuity:** Reservations enable multi-step administrative workflows
- **State Preservation:** Session state maintained across operation steps
- **Error Recovery:** Failed operations release reservations properly
- **Atomic Transactions:** Complex operations maintain atomicity

### Concurrency Control
- **Exclusive Access:** Reserved pages prevent concurrent modification
- **Deadlock Prevention:** Reservation ordering prevents deadlocks
- **Performance Optimization:** Reservations reduce contention
- **Resource Fairness:** Fair allocation of reserved resources

## Free Pages Bitmap Integration

### Real-Time Synchronization
- **Immediate Updates:** Bitmap updated simultaneously with coin data
- **Consistency Guarantee:** Perfect synchronization between systems
- **Atomic Operations:** Bitmap and coin data updated atomically
- **Recovery Capability:** Bitmap can be reconstructed from coin data

### Performance Benefits
- **Instant Availability:** Free coin discovery without disk scanning
- **Memory Efficiency:** Minimal memory overhead for maximum performance
- **Scalable Queries:** Performance independent of total coin count
- **Cache Optimization:** Reduces database cache pressure

### Administrative Advantages
- **Real-Time Analytics:** Instant denomination utilization statistics
- **Capacity Planning:** Immediate availability information
- **System Monitoring:** Real-time coin pool status
- **Automated Management:** Enables automated coin pool management

## Dual Hashing Architecture

### Client Compatibility
- **Legacy Support:** MD5 hashing for encryption_type < 4
- **Modern Security:** SHA-256 hashing for encryption_type >= 4
- **Smooth Migration:** Clients can upgrade at their own pace
- **Version Detection:** Automatic detection of client capabilities

### Security Evolution
- **Cryptographic Strength:** SHA-256 provides superior security
- **Hash Collision Resistance:** Modern algorithms prevent collision attacks
- **Future Proofing:** Architecture supports additional hash algorithms
- **Performance Optimization:** Hardware acceleration when available

### Implementation Details
- **Hash Input Construction:** Deterministic input generation for reproducibility
- **Salt Integration:** RAIDA number and admin key provide unique salting
- **Length Handling:** Proper handling of variable-length inputs
- **Error Handling:** Graceful handling of hash generation failures

## Performance Characteristics

### Response Time Optimization
- **Database Cache:** Leverages on-demand page cache for performance
- **Bitmap Speed:** Sub-millisecond bitmap operations
- **Bulk Operations:** Optimized handling of large coin sets
- **Efficient Algorithms:** Range detection and bit manipulation optimizations

### Memory Management
- **Dynamic Allocation:** Memory allocated based on actual needs
- **Efficient Storage:** Bit-packed responses minimize memory usage
- **Cache Integration:** Works efficiently with database page cache
- **Resource Cleanup:** Proper cleanup prevents memory leaks

### Scalability Features
- **Linear Performance:** Performance scales linearly with operation size
- **Concurrent Operations:** Multiple administrative operations can proceed
- **Resource Bounds:** All operations have predictable resource usage
- **Load Distribution:** Even distribution of processing load

## Error Handling and Recovery

### Request Validation
- **Comprehensive Validation:** All request parameters validated
- **Size Verification:** Request sizes strictly enforced
- **Format Checking:** Request format validation prevents errors
- **Range Validation:** All numeric parameters range-checked

### Operation Safety
- **Atomic Operations:** Administrative operations are atomic
- **Rollback Capability:** Failed operations leave consistent state
- **Error Propagation:** Clear error codes and messages
- **State Verification:** Post-operation state verification

### System Recovery
- **Data Consistency:** Operations maintain data consistency
- **Recovery Procedures:** Clear recovery procedures for failures  
- **Backup Integration:** Compatible with backup and recovery systems
- **Audit Trail:** Complete audit trail for recovery verification

## Monitoring and Diagnostics

### Operation Metrics
- **Performance Tracking:** Response times and throughput measurement
- **Success Rates:** Success/failure ratios for all operations
- **Resource Usage:** Memory and CPU usage monitoring
- **Error Rates:** Detailed error classification and tracking

### System Health
- **Coin Pool Status:** Real-time coin availability monitoring
- **Reservation Health:** Reservation system status and utilization
- **Cache Performance:** Database cache hit rates and efficiency
- **Bitmap Consistency:** Bitmap synchronization verification

### Administrative Reporting
- **Usage Statistics:** Comprehensive usage analytics
- **Capacity Reports:** Denomination capacity and utilization
- **Performance Analysis:** Detailed performance breakdowns
- **Security Audits:** Administrative operation audit reports

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache and persistence
- **Authentication System:** Administrative key verification
- **Bitmap System:** In-memory free coin tracking
- **Cryptographic Functions:** Hash generation for coin creation
- **Session Management:** Page reservation and session tracking

### Used By
- **Administrative Tools:** Primary interface for system administration
- **Monitoring Systems:** System health and performance monitoring
- **Backup Systems:** Data export and system backup operations
- **Development Tools:** Development and testing utilities

### Cross-File Dependencies
- **Database Module:** Page access and coin data management
- **Configuration Module:** Administrative keys and system parameters
- **Utilities Module:** Cryptographic functions and data conversion
- **Authentication Module:** Administrative security verification
- **Session Module:** Reservation management and session tracking

This executive command module provides comprehensive administrative control over the coin system with advanced features including dual hashing support, real-time bitmap integration, sophisticated reservation management, and complete administrative security, enabling efficient system administration and management operations.