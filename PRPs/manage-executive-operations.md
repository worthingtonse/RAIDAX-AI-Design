# Executive Command Handlers (cmd_executive.c)

## Module Purpose
This module implements administrative executive commands for the RAIDA network, providing high-level coin management operations including coin creation, deletion, and availability queries. These commands require administrative authentication and are used for system administration, coin lifecycle management, and network maintenance operations.

## Core Functionality

### 1. Available Serial Numbers Query (`cmd_get_available_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: 54-byte payload with session ID, admin authentication, and denomination requests

**Returns:** None (modifies connection structure with available serial numbers and ranges)

**Purpose:** Provides administrative interface to query available coin serial numbers across all denominations, returning both individual serial numbers and contiguous ranges for efficient coin allocation.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (54 bytes)
   - Extracts session ID (4 bytes) and admin authentication key (16 bytes)
   - Validates admin authentication against configured admin key
   - Parses denomination request bitmap (16 bytes)

2. **Administrative Authentication:**
   - Compares provided admin key with configured system admin key
   - Rejects request if authentication fails
   - Provides secure access to administrative functions

3. **Denomination Processing:**
   - Iterates through all possible denominations based on request bitmap
   - For each requested denomination, scans all pages for available coins
   - Skips reserved pages to prevent conflicts with active operations
   - Reserves pages with session ID during processing

4. **Range Optimization:**
   - Identifies contiguous ranges of available serial numbers
   - Separates individual serial numbers from ranges
   - Optimizes response format for efficient client processing
   - Limits total coins returned to prevent oversized responses

5. **Response Generation:**
   - For each denomination with available coins:
     - Includes denomination identifier
     - Lists range count and individual serial number count
     - Provides range start/end pairs
     - Includes individual serial numbers

**Input Format:**
- 4 bytes: Session ID for page reservation
- 16 bytes: Admin authentication key
- 16 bytes: Denomination request bitmap
- 2 bytes: End-of-frame marker

**Output Format:**
Per denomination:
- 1 byte: Denomination identifier
- 1 byte: Range count
- 1 byte: Individual serial number count
- Variable: Range pairs (8 bytes each: 4 bytes start + 4 bytes end)
- Variable: Individual serial numbers (4 bytes each)

**Dependencies:**
- Database layer for on-demand page access and locking
- Configuration system for admin key validation
- Page reservation system for session management

### 2. Coin Creation Operation (`cmd_create_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) with session ID, admin auth, and coin list

**Returns:** None (modifies connection structure with old authentication numbers)

**Purpose:** Creates new coins by generating secure authentication numbers and updating coin records, returning the previous authentication numbers for tracking purposes.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (43 bytes)
   - Extracts session ID and admin authentication key
   - Validates admin authentication against configured admin key
   - Calculates coin count from payload size (5 bytes per coin)

2. **Coin List Processing:**
   - Processes each coin in the request list
   - Validates denomination and serial number for each coin
   - Retrieves coin page using on-demand cache system
   - Verifies page is reserved by requesting session or emergency session

3. **Authentication Number Generation:**
   - Creates secure hash input using server ID, serial number, and admin key
   - Generates new authentication number using cryptographic hash function
   - Ensures authentication numbers are cryptographically secure

4. **Coin Record Update:**
   - Stores previous authentication number for response
   - Updates coin record with new authentication number
   - Sets MFS timestamp to current month
   - Marks database page as dirty for persistence

5. **Response Generation:**
   - Returns list of previous authentication numbers (16 bytes each)
   - Provides audit trail for coin creation operations
   - Enables tracking of coin lifecycle changes

**Security Features:**
- Administrative authentication required for all operations
- Cryptographically secure authentication number generation
- Session-based page reservation prevents conflicts
- Audit trail through returned previous authentication numbers

**Input Format:**
- 4 bytes: Session ID
- 16 bytes: Admin authentication key
- Variable: Coin list (5 bytes each: 1 byte denomination + 4 bytes serial number)
- 2 bytes: End-of-frame marker

**Output Format:**
- Variable: Previous authentication numbers (16 bytes each)

### 3. Coin Deletion Operation (`cmd_delete_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) with admin auth and coin authentication data

**Returns:** None (modifies connection structure with deletion results bitmap)

**Purpose:** Deletes coins by verifying their current authentication numbers and then freeing them for reuse, providing secure coin destruction with authentication verification.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (55 bytes)
   - Extracts admin authentication key from payload start
   - Validates admin authentication against configured admin key
   - Calculates coin count from payload size (21 bytes per coin)

2. **Coin Authentication and Deletion:**
   - For each coin in the request:
     - Retrieves coin page using denomination and serial number
     - Compares provided authentication number with stored value
     - If authentic, marks coin as free (MFS = 0) and sets success bit
     - If not authentic, sets failure bit in response bitmap
     - Marks modified pages as dirty for persistence

3. **Result Tracking:**
   - Maintains counts of passed and failed authentications
   - Generates bitmap with one bit per coin indicating success/failure
   - Provides detailed results for batch operations

4. **Response Generation:**
   - **STATUS_ALL_PASS:** All coins authenticated and deleted
   - **STATUS_ALL_FAIL:** No coins authenticated
   - **STATUS_MIXED:** Partial success with detailed bitmap

**Security Features:**
- Administrative authentication required
- Authentication number verification before deletion
- Audit trail through success/failure tracking
- Secure coin destruction process

**Input Format:**
- 16 bytes: Admin authentication key
- Variable: Coin authentication records (21 bytes each: 1 byte denomination + 4 bytes serial + 16 bytes authentication)
- 2 bytes: End-of-frame marker

**Output Format:**
- Variable: Result bitmap (1 bit per coin, only for mixed results)

### 4. Coin Liberation Operation (`cmd_free_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) with admin auth and coin list

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Frees coins without authentication verification, making them available for reuse - used for administrative coin pool management.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (43 bytes)
   - Extracts admin authentication key
   - Validates admin authentication against configured admin key
   - Calculates coin count from payload size (5 bytes per coin)

2. **Coin Liberation:**
   - For each coin in the request:
     - Retrieves coin page using denomination and serial number
     - Sets MFS to 0 (free) without authentication verification
     - Marks database page as dirty for persistence

3. **Administrative Override:**
   - Bypasses normal authentication requirements
   - Provides emergency coin recovery capability
   - Enables administrative coin pool management

**Security Features:**
- Administrative authentication required
- Direct coin liberation without user authentication
- Audit trail through administrative logging
- Emergency recovery capability

**Input Format:**
- 16 bytes: Admin authentication key
- Variable: Coin identifiers (5 bytes each: 1 byte denomination + 4 bytes serial number)
- 2 bytes: End-of-frame marker

**Output Format:**
- Status code indicating success or failure reason

### 5. Complete Serial Number Bitmap (`cmd_get_all_sns`)
**Parameters:**
- Connection information structure
- Input: 35-byte payload with admin auth and denomination

**Returns:** None (modifies connection structure with complete coin usage bitmap)

**Purpose:** Returns complete bitmap of all coin serial numbers for a specific denomination, indicating which coins are currently in use versus available.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (35 bytes)
   - Extracts admin authentication key and requested denomination
   - Validates admin authentication against configured admin key
   - Validates denomination within acceptable range

2. **Direct File Access:**
   - Bypasses cache system for complete denomination scan
   - Reads page files directly from disk for consistency
   - Processes all pages for the requested denomination
   - Builds complete bitmap of coin usage status

3. **Bitmap Generation:**
   - Creates bitmap with one bit per possible serial number
   - Sets bit to 1 for coins currently in use (MFS != 0)
   - Leaves bit as 0 for available coins
   - Provides complete view of denomination usage

4. **Response Generation:**
   - Returns denomination identifier
   - Includes bitmap size information
   - Provides complete coin usage bitmap

**Administrative Features:**
- Complete denomination overview
- Direct file access for consistency
- Comprehensive usage reporting
- Administrative monitoring capability

**Input Format:**
- 16 bytes: Admin authentication key
- 1 byte: Requested denomination
- 18 bytes: Reserved/padding

**Output Format:**
- 1 byte: Denomination identifier
- 4 bytes: Bitmap size
- Variable: Complete usage bitmap (1 bit per serial number)

## Data Structures and Formats

### Administrative Authentication
- **Admin Key:** 16-byte cryptographic key for administrative access
- **Session ID:** 4-byte identifier for page reservation management
- **Emergency Session:** Special session ID (0xeeeeeeee) for emergency operations

### Coin Record Formats
- **Coin Identifier:** 1 byte denomination + 4 bytes serial number
- **Authentication Record:** Coin identifier + 16 bytes authentication number
- **Creation Record:** Coin identifier for new coin creation
- **Deletion Record:** Coin identifier + 16 bytes authentication number

### Response Formats
- **Range Response:** Start and end serial numbers for contiguous ranges
- **Individual Response:** List of individual serial numbers
- **Bitmap Response:** Bit field indicating operation results or coin usage
- **Authentication Response:** Previous authentication numbers for audit

## Security Considerations

### Administrative Security
- **Authentication Required:** All operations require valid admin key
- **Cryptographic Verification:** Admin key compared using secure comparison
- **Session Management:** Page reservations prevent conflicts
- **Audit Trail:** All operations logged for security monitoring

### Data Integrity
- **Page Locking:** Database page locking ensures consistent coin state
- **MFS Timestamps:** All coin changes timestamped for audit trail
- **Dirty Page Tracking:** Modified data marked for reliable persistence
- **Atomic Operations:** Operations either succeed completely or fail entirely

### Attack Prevention
- **Administrative Access Control:** Only valid admin keys accepted
- **Session Validation:** Page reservations prevent concurrent access
- **Input Validation:** All payloads validated for size and format
- **Rate Limiting:** Implicit through administrative access requirements

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for exact or minimum expected length
- **Authentication Validation:** Admin key verified against configuration
- **Denomination Validation:** All denominations within acceptable range
- **Session Validation:** Pages must be reserved by requesting session

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_ADMIN_AUTH`: Invalid admin authentication key
- `ERROR_COINS_NOT_DIV`: Coin data not properly aligned
- `ERROR_INVALID_SN_OR_DENOMINATION`: Invalid coin reference
- `ERROR_PAGE_IS_NOT_RESERVED`: Page not reserved by session
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

### Range Optimization
- **Contiguous Range Detection:** Efficiently identifies ranges of available coins
- **Response Optimization:** Minimizes response size through range compression
- **Processing Efficiency:** Single pass through page data for range detection

### Administrative Operations
- **Batch Processing:** Multiple coin operations handled efficiently
- **Direct File Access:** Bypasses cache for complete denomination scans
- **Resource Management:** Proper cleanup ensures no resource leaks

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Configuration System:** Admin key validation and server identification
- **Cryptographic Utilities:** Secure authentication number generation
- **Utilities Module:** Hash functions for authentication number creation

### Constants Required
- `RECORDS_PER_PAGE`: Database page organization constant
- `TOTAL_PAGES`: Total number of pages per denomination
- `MAX_AVAILABLE_COINS`: Maximum coins returned in availability query
- `TOTAL_DENOMINATIONS`: Total number of supported denominations
- `STATUS_*`: Operation result status codes
- `ERROR_*`: Error condition definitions

### Used By
- **Administrative Tools:** Primary interface for coin management
- **System Maintenance:** Coin lifecycle management operations
- **Network Operations:** Bulk coin allocation and deallocation
- **Monitoring Systems:** Coin usage reporting and analysis

## Threading and Concurrency
- **Page Locking:** Thread-safe access to coin data through database layer
- **Session Management:** Session-based reservations prevent conflicts
- **Atomic Operations:** Individual operations are atomic
- **Resource Safety:** Proper cleanup ensures no resource leaks

## Administrative Features
- **Emergency Operations:** Special session support for emergency coin management
- **Complete Denomination Views:** Full bitmap access for monitoring
- **Audit Trail:** Previous authentication numbers returned for tracking
- **Range Optimization:** Efficient representation of available coin ranges

This executive command module provides essential administrative capabilities for the RAIDA network, enabling secure coin lifecycle management and system administration while maintaining cryptographic security and operational integrity.