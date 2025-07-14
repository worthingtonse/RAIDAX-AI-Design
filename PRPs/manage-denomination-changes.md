
# Change-Making Command Handlers (cmd_change.c)

## Module Purpose
This module implements the core change-making operations for the RAIDA network, allowing users to break larger denomination coins into smaller ones and join smaller coins into larger denominations. The module provides essential functionality for coin denomination management, enabling flexible value manipulation while maintaining cryptographic security and transactional integrity.

## Core Functionality

### 1. Available Change Serial Numbers Query (`cmd_get_available_change_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: 24-byte payload with session ID, operation type, and denomination

**Returns:** None (modifies connection structure with available serial numbers)

**Purpose:** Finds available coin slots for change-making operations by scanning through denomination pages to locate unused coins.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (24 bytes)
   - Extracts session ID (4 bytes), operation type (1 byte), and denomination (1 byte)
   - Validates operation type (BREAK or JOIN operations only)
   - Validates denomination within acceptable range

2. **Target Denomination Calculation:**
   - For BREAK operations: searches for coins one denomination smaller
   - For JOIN operations: searches for coins one denomination larger

3. **Page Scanning Process:**
   - Iterates through all possible pages for the target denomination
   - Uses on-demand page cache system for efficient memory management
   - Skips reserved pages (checks reservation status and timing)
   - Reserves available pages with session ID to prevent conflicts

4. **Coin Slot Discovery:**
   - Scans each page for unused coin slots (MFS value of 0)
   - Collects serial numbers of available coins
   - Limits results to maximum change coins threshold
   - Maintains count of discovered available slots

**Input Format:**
- 4 bytes: Session ID for page reservation
- 1 byte: Operation type (BREAK=1, JOIN=2)
- 1 byte: Source denomination
- 18 bytes: Reserved/padding

**Output Format:**
- 1 byte: Target denomination
- Variable length: List of available serial numbers (4 bytes each)

**Dependencies:**
- Database layer for on-demand page access and locking
- Page reservation system for session management
- Configuration system for page and record constants

### 2. Coin Breaking Operation (`cmd_break`)
**Parameters:**
- Connection information structure
- Input: 253-byte payload with session ID, source coin, and target coins

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Breaks a single larger denomination coin into 10 smaller denomination coins, destroying the original and creating new ones.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (253 bytes)
   - Extracts session ID, source coin data, and target coin list
   - Validates source coin denomination within acceptable range
   - Applies rate limiting to prevent abuse

2. **Source Coin Authentication:**
   - Retrieves source coin page using denomination and serial number
   - Compares provided authentication number with stored value
   - Ensures coin is authentic before proceeding with break operation

3. **Target Coin Ownership:**
   - Processes list of 10 target coins (smaller denomination)
   - Validates each target coin denomination (must be source denomination - 1)
   - Verifies target coin pages are reserved by requesting session
   - Updates each target coin with provided authentication number
   - Sets MFS timestamp to current month for all target coins

4. **Source Coin Destruction:**
   - Generates new secure authentication number using cryptographic hash
   - Replaces source coin authentication number with secure random value
   - Marks source coin as free (MFS = 0) to make it available again
   - Marks source coin page as dirty for persistence

**Security Features:**
- Cryptographically secure random number generation for destroyed coins
- Authentication number verification before breaking
- Session-based page reservation prevents conflicts
- Rate limiting prevents excessive breaking operations

**Input Format:**
- 4 bytes: Session ID
- 1 byte: Source coin denomination
- 4 bytes: Source coin serial number
- 16 bytes: Source coin authentication number
- 210 bytes: 10 target coins (21 bytes each: 1 byte denomination + 4 bytes serial + 16 bytes authentication)

**Output Format:**
- Status code indicating success or failure reason

### 3. Coin Joining Operation (`cmd_join`)
**Parameters:**
- Connection information structure
- Input: 253-byte payload with session ID, target coin, and source coins

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Joins 10 smaller denomination coins into a single larger denomination coin, destroying the originals and creating a new one.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (253 bytes)
   - Extracts session ID, target coin data, and source coin list
   - Validates target coin denomination within acceptable range

2. **Target Coin Page Verification:**
   - Retrieves target coin page using denomination and serial number
   - Verifies page is reserved by requesting session
   - Ensures target coin slot is available for creation

3. **Source Coin Authentication:**
   - Validates all 10 source coins before making any changes
   - Verifies each source coin denomination (must be target denomination - 1)
   - Compares provided authentication numbers with stored values
   - Ensures all source coins are authentic before proceeding

4. **Source Coin Destruction:**
   - Frees all 10 source coins by setting MFS to 0
   - Marks all source coin pages as dirty for persistence
   - Makes source coin slots available for future use

5. **Target Coin Creation:**
   - Creates new larger denomination coin with provided authentication number
   - Sets MFS timestamp to current month
   - Marks target coin page as dirty for persistence

**Security Features:**
- All-or-nothing operation: either all source coins are authentic or none are processed
- Authentication verification before any state changes
- Session-based reservation prevents conflicts
- Atomic operation ensures consistency

**Input Format:**
- 4 bytes: Session ID
- 1 byte: Target coin denomination
- 4 bytes: Target coin serial number
- 16 bytes: Target coin authentication number
- 210 bytes: 10 source coins (21 bytes each: 1 byte denomination + 4 bytes serial + 16 bytes authentication)

**Output Format:**
- Status code indicating success or failure reason

## Data Structures and Formats

### Operation Types
- **BREAK Operation (0x1):** Convert one larger coin into 10 smaller coins
- **JOIN Operation (0x2):** Convert 10 smaller coins into one larger coin

### Coin Record Format
- **Change Query Record:** 1 byte denomination + 4 bytes serial number
- **Authentication Record:** 1 byte denomination + 4 bytes serial number + 16 bytes authentication number
- **Batch Record:** 10 authentication records for multi-coin operations

### Response Formats
- **Available SNs Response:** Target denomination + list of available serial numbers
- **Operation Response:** Status code indicating success or detailed failure reason

## Security Considerations

### Authentication Security
- **Proof of Ownership:** All operations require demonstration of current coin ownership
- **Cryptographic Destruction:** Destroyed coins receive cryptographically secure new authentication numbers
- **Session Management:** Page reservation prevents concurrent access conflicts

### Data Integrity
- **Page Locking:** Database page locking ensures consistent coin state during operations
- **MFS Timestamps:** All coin changes timestamped for audit trail
- **Dirty Page Tracking:** Modified data marked for reliable persistence

### Attack Prevention
- **Rate Limiting:** Prevents excessive breaking operations from single IP
- **Atomic Operations:** Change operations either succeed completely or fail entirely
- **Authentication Validation:** All coins validated before any state changes

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for exact expected length
- **Operation Validation:** Operation type must be valid BREAK or JOIN
- **Denomination Validation:** All denominations within acceptable range
- **Session Validation:** Pages must be reserved by requesting session

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_INVALID_PARAMETER`: Invalid operation type
- `ERROR_INVALID_SN_OR_DENOMINATION`: Invalid coin reference
- `ERROR_PAGE_IS_NOT_RESERVED`: Page not reserved by session
- `ERROR_REQUEST_RATE`: Rate limit exceeded
- `STATUS_ALL_FAIL`: Authentication failure for source coins
- `ERROR_MEMORY_ALLOC`: Memory allocation failure

### Recovery Mechanisms
- **State Consistency:** Failed operations leave coin state unchanged
- **Resource Cleanup:** Memory and page locks released on error conditions
- **Session Cleanup:** Page reservations released after operations

## Performance Characteristics

### On-Demand Cache Integration
- **Cache Efficiency:** Frequently accessed pages remain in memory
- **Memory Conservation:** Only accessed pages loaded into cache
- **I/O Optimization:** Batch operations minimize page cache pressure

### Page Reservation System
- **Session Management:** Prevents concurrent access conflicts
- **Automatic Cleanup:** Expired reservations automatically released
- **Conflict Prevention:** Sessions ensure exclusive access to coin slots

### Operation Efficiency
- **Batch Processing:** Multiple coin operations handled efficiently
- **Atomic Operations:** All-or-nothing processing prevents partial states
- **Resource Management:** Proper cleanup ensures no resource leaks

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Cryptographic Utilities:** Secure random number generation, hash functions
- **Rate Limiting:** IP-based request rate limiting
- **Configuration System:** Server identification and operational parameters
- **Utilities Module:** Secure authentication number generation

### Constants Required
- `RECORDS_PER_PAGE`: Database page organization constant
- `TOTAL_PAGES`: Total number of pages per denomination
- `MAX_CHANGE_COINS`: Maximum coins returned in availability query
- `MAX_DENOMINATION`:Maximum allowed denomination 
- `MIN_DENOMINATION`: Minimum usable denomination 
- `OP_BREAK`, `OP_JOIN`: Operation type identifiers
- `STATUS_SUCCESS`:Operation completed successfully.
- `STATUS_*`: Operation result status codes
- `ERROR_*`: Error condition definitions


## Threading and Concurrency
- **Page Locking:** Thread-safe access to coin data through database layer
- **Session Management:** Session-based reservations prevent conflicts
- **Atomic Operations:** Individual operations are atomic
- **Resource Safety:** Proper cleanup ensures no resource leaks

## Mathematical Relationships
- **Denomination Mapping:** Each denomination level represents 10x value increase
- **Coin Ratios:** 1 larger coin = 10 smaller coins of next denomination down
- **Value Conservation:** Total value preserved across all change operations
- **Batch Constraints:** Exactly 10 coins required for join operations

This change-making module provides essential denomination flexibility for the RAIDA network, enabling users to adjust coin denominations while maintaining cryptographic security and transactional integrity.