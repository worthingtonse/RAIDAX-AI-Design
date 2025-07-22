# Executive Command Handlers (cmd_executive)

## Module Purpose
This module implements executive-level administrative commands for coin creation, management, and deletion within the RAIDA network. These commands require administrative authentication and provide comprehensive coin lifecycle management including creation, deletion, liberation, and inventory operations. The module supports dual hashing for backward compatibility and implements secure administrative controls for network maintenance.

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

### 1. Get Available Serial Numbers (`cmd_get_available_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: 54-byte payload (session ID + admin key + denomination selection bitmap)

**Returns:** None (modifies connection structure with available serial numbers and ranges)

**Purpose:** Provides comprehensive inventory of available coin serial numbers across requested denominations, supporting both individual serial numbers and contiguous ranges for efficient bulk operations.

**Process:**
1. **Request Validation:**
   - Validates exact 54-byte payload size
   - Extracts session ID from request
   - Validates administrative authentication key against configuration

2. **Administrative Authentication:**
   - Compares provided admin key with configured admin key
   - Fails operation if authentication is invalid
   - Ensures only authorized administrators can query inventory

3. **Denomination Processing:**
   - Processes denomination selection bitmap (16 bytes)
   - For each requested denomination:
     - Iterates through all pages using on-demand cache
     - Skips pages reserved by other sessions
     - Reserves each accessed page for the requesting session
     - Identifies available coins (MFS = 0)

4. **Range Optimization:**
   - Groups contiguous available serial numbers into ranges
   - Separates individual serial numbers from ranges
   - Limits results to MAX_AVAILABLE_COINS per denomination
   - Optimizes response size by using ranges for contiguous blocks

5. **Response Generation:**
   - For each denomination with available coins:
     - Denomination identifier (1 byte)
     - Number of ranges (1 byte)
     - Number of individual serial numbers (1 byte)
     - Range data (start/end pairs, 8 bytes each)
     - Individual serial numbers (4 bytes each)

**Security Features:**
- Administrative key authentication required
- Session-based page reservation prevents conflicts
- Comprehensive availability checking

**Used By:** Administrative tools, bulk coin creation workflows

### 2. Create Coins (`cmd_create_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) with admin key, session ID, and coin specifications

**Returns:** None (modifies connection structure with previous authentication numbers)

**Purpose:** Creates new coins with cryptographically generated authentication numbers, supporting both administrative and session-based creation with dual hashing support.

**Process:**
1. **Request Validation:**
   - Validates minimum 43-byte payload size
   - Calculates coin count from payload size (5 bytes per coin)
   - Validates administrative authentication key

2. **Coin Creation Logic:**
   - For each coin in the request:
     - Extracts denomination and serial number
     - Retrieves target page using on-demand cache
     - Validates page reservation (session ID or special bypass)
     - Stores previous authentication number for response

3. **Dual Hash Authentication Number Generation:**
   - Constructs hash input from server RAIDA number, serial number, and admin key
   - **Legacy Clients (encryption_type < 4):** Uses MD5 hash algorithm
   - **Modern Clients (encryption_type >= 4):** Uses SHA-256 hash algorithm
   - Generates unique authentication number for each coin

4. **Coin Activation:**
   - Updates coin record with new authentication number
   - Sets MFS timestamp to mark coin as active
   - Marks database page as dirty for persistence
   - Returns previous authentication numbers to caller

**Security Features:**
- Administrative authentication required
- Cryptographically secure authentication number generation
- Session validation for non-administrative creation
- Dual hash support for client compatibility

**Used By:** Coin minting systems, administrative creation tools

### 3. Free Coins (`cmd_free_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) with admin key and coin specifications

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Liberates coins by setting their status to available, effectively removing them from circulation without deleting data.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size and admin authentication
   - Calculates coin count from payload (5 bytes per coin: denomination + serial number)

2. **Administrative Security:**
   - Validates administrative key against configuration
   - Ensures only authorized administrators can free coins

3. **Coin Liberation:**
   - For each specified coin:
     - Retrieves coin page using database layer
     - Sets MFS field to 0 (marking coin as available)
     - Marks page as dirty for persistence
     - Preserves authentication number data

**Security Features:**
- Administrative authentication required
- Non-destructive operation (preserves coin data)
- Immediate effect on coin availability

**Used By:** Administrative cleanup tools, coin recycling systems

### 4. Delete Coins (`cmd_delete_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) with admin key and coin deletion data

**Returns:** None (modifies connection structure with deletion results bitmap)

**Purpose:** Securely deletes coins by verifying ownership through authentication numbers before permanently removing them from circulation.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size
   - Calculates coin count from payload (21 bytes per coin: denomination + serial number + authentication number)
   - Validates administrative authentication key

2. **Ownership Verification:**
   - For each coin to be deleted:
     - Retrieves stored authentication number from database
     - Compares with provided authentication number
     - Only deletes coins with matching authentication numbers

3. **Secure Deletion:**
   - Sets MFS to 0 for authenticated coins (marks as available)
   - Marks affected pages as dirty
   - Tracks successful and failed deletions

4. **Response Generation:**
   - Creates bitmap indicating which coins were successfully deleted
   - **STATUS_ALL_PASS:** All coins deleted successfully
   - **STATUS_ALL_FAIL:** No coins could be deleted
   - **STATUS_MIXED:** Partial deletion with detailed bitmap

**Security Features:**
- Administrative authentication required
- Ownership verification before deletion
- Detailed audit trail through response bitmap
- Prevents unauthorized coin destruction

**Used By:** Administrative management tools, coin cleanup operations

### 5. Get All Serial Numbers (`cmd_get_all_sns`)
**Parameters:**
- Connection information structure
- Input: 35-byte payload (admin key + denomination + reserved bytes)

**Returns:** None (modifies connection structure with comprehensive coin status bitmap)

**Purpose:** Generates a complete bitmap of all coin statuses for a specific denomination, providing comprehensive inventory information for administrative analysis.

**Process:**
1. **Request Validation:**
   - Validates exact 35-byte payload size
   - Validates administrative authentication key
   - Validates requested denomination within valid range

2. **Direct File System Access:**
   - Bypasses cache system for comprehensive scan
   - Directly reads page files from disk storage
   - Ensures complete and current data

3. **Bitmap Generation:**
   - Creates bitmap covering all possible serial numbers for denomination
   - Sets bit to 1 for coins in circulation (MFS != 0)
   - Sets bit to 0 for available coins (MFS = 0)
   - Efficiently packs status into bitmap format

4. **Response Structure:**
   - Denomination identifier (1 byte)
   - Bitmap size (4 bytes)
   - Status bitmap (variable length, packed bits)

**Performance Characteristics:**
- Direct file access for guaranteed accuracy
- Efficient bitmap representation
- Comprehensive coverage of all possible serial numbers

**Used By:** Administrative analysis tools, inventory management systems

## Data Structures and Formats

### Request Formats
| Operation | Minimum Size | Structure |
|-----------|-------------|-----------|
| Get Available SNs | 54 bytes | Session ID (4) + Admin Key (16) + Denomination Bitmap (16) + Reserved (18) |
| Create Coins | 43 bytes | Session ID (4) + Admin Key (16) + Coin Specs (5 × N) + Reserved (18) |
| Free Coins | 43 bytes | Admin Key (16) + Coin Specs (5 × N) + Reserved (22) |
| Delete Coins | 55 bytes | Admin Key (16) + Coin Data (21 × N) + Reserved (18) |
| Get All SNs | 35 bytes | Admin Key (16) + Denomination (1) + Reserved (18) |

### Coin Data Formats
| Format | Size | Structure |
|--------|------|-----------|
| Coin Specification | 5 bytes | Denomination (1) + Serial Number (4) |
| Coin Deletion Record | 21 bytes | Denomination (1) + Serial Number (4) + Authentication Number (16) |

### Response Formats
| Operation | Response Format |
|-----------|----------------|
| Get Available SNs | Per denomination: Den (1) + Range Count (1) + Individual Count (1) + Range Data + Individual SNs |
| Create Coins | Previous authentication numbers (16 bytes per coin) |
| Free/Delete Coins | Status code or deletion bitmap |
| Get All SNs | Denomination (1) + Size (4) + Status Bitmap |

## Security Considerations

### Administrative Authentication
- **Key-Based Authentication:** All operations require valid administrative key
- **Secure Key Comparison:** Direct comparison with configured administrative key
- **Access Control:** Only authenticated administrators can perform executive operations

### Data Integrity
- **Ownership Verification:** Delete operations require proof of coin ownership
- **Atomic Operations:** Individual coin operations are atomic
- **Audit Trail:** All operations logged and tracked through MFS timestamps

### Cryptographic Security
- **Dual Hash Support:** Automatic algorithm selection based on client capabilities
- **Secure Random Generation:** Cryptographically secure authentication number generation
- **Algorithm Flexibility:** Support for both legacy MD5 and modern SHA-256

## Error Handling and Validation

### Input Validation
- **Size Validation:** Exact or minimum payload sizes enforced
- **Authentication Validation:** Administrative keys validated against configuration
- **Data Alignment:** Coin data properly aligned to record boundaries

### Runtime Validation
- **Denomination Validation:** All coin denominations within valid ranges
- **Page Validation:** Database pages accessible and valid
- **Session Validation:** Page reservations validated for create operations

### Error Recovery
- **Resource Cleanup:** Database locks and memory freed on errors
- **State Consistency:** Failed operations leave database state unchanged
- **Graceful Degradation:** Partial success operations report individual results

## Performance Characteristics

### Database Integration
- **On-Demand Cache:** Efficient use of database layer caching
- **Direct File Access:** Bypass cache for comprehensive operations when needed
- **Batch Processing:** Efficient handling of multiple coin operations

### Memory Management
- **Dynamic Allocation:** Response buffers sized based on operation requirements
- **Resource Cleanup:** Proper memory management prevents leaks
- **Efficient Packing:** Optimized data structures for large responses

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache and direct file access
- **Configuration Module:** Administrative key storage and server identification
- **Utilities Module:** Dual hash functions and cryptographic operations
- **File System:** Direct page file access for comprehensive operations

### External Constants Required
- `TOTAL_PAGES`: Total number of pages per denomination
- `RECORDS_PER_PAGE`: Number of coin records per page
- `MIN_DENOMINATION`, `MAX_DENOMINATION`: Valid denomination range
- Administrative key from configuration

### Used By
- **Administrative Tools:** Primary interface for coin management
- **Minting Systems:** Coin creation and activation workflows
- **Inventory Systems:** Comprehensive coin status and availability tracking
- **Maintenance Tools:** Coin cleanup and management operations

### Cross-File Dependencies
- **Configuration Module:** Administrative key validation and server settings
- **Database Layer:** Page access, locking, and persistence
- **Utilities Module:** Hash generation and cryptographic functions
- **File System Interface:** Direct file access for comprehensive operations

## Threading and Concurrency
- **Administrative Serialization:** Executive operations may require exclusive access
- **Page Locking:** Thread-safe database access through proper locking
- **Session Management:** Reservation system prevents concurrent conflicts
- **Resource Safety:** Proper cleanup ensures thread safety

## Backward Compatibility
- **Dual Hash Support:** Automatic selection of hash algorithm based on client version
- **Protocol Versioning:** Client encryption type determines hash algorithm used
- **Legacy Integration:** Full compatibility with existing administrative tools

This executive module provides essential administrative functionality for the RAIDA network, enabling secure coin lifecycle management, comprehensive inventory tracking, and efficient bulk operations while maintaining security through administrative authentication and cryptographic integrity.