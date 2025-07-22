# Change-Making Command Handlers (cmd_change)

## Module Purpose
This module implements change-making operations for the RAIDA network, enabling clients to break larger denomination coins into smaller ones and join smaller coins into larger denominations. It provides secure change-making operations with reservation systems, dual hashing support for backward compatibility, and comprehensive validation to ensure monetary conservation across all operations.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_CHANGE_COINS` | 64 | Maximum number of coins that can be returned in a single change operation |
| `OP_BREAK` | 0x1 | Operation code for breaking larger coins into smaller denominations |
| `OP_JOIN` | 0x2 | Operation code for joining smaller coins into larger denominations |

## Error Codes
| Constant | Description |
|----------|-------------|
| `ERROR_INVALID_PACKET_LENGTH` | Request payload size is incorrect for the operation |
| `ERROR_INVALID_PARAMETER` | Invalid operation type specified in request |
| `ERROR_INVALID_SN_OR_DENOMINATION` | Invalid coin serial number or denomination provided |
| `ERROR_MEMORY_ALLOC` | Failed to allocate memory for response buffer |
| `ERROR_PAGE_IS_NOT_RESERVED` | Required page is not reserved by the requesting session |
| `STATUS_ALL_FAIL` | All coins in the operation failed validation |
| `STATUS_SUCCESS` | Operation completed successfully |

## Core Functionality

### 1. Get Available Change Serial Numbers (`cmd_get_available_change_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: 24-byte payload (session ID + operation type + denomination + reserved bytes)

**Returns:** None (modifies connection structure with available serial numbers)

**Purpose:** Retrieves available coin serial numbers for change-making operations, supporting both break and join operations by finding appropriate target denominations.

**Process:**
1. **Request Validation:**
   - Validates exact 24-byte payload size
   - Extracts session ID, operation type, and denomination
   - Validates operation type (OP_BREAK or OP_JOIN)
   - Validates denomination within valid range

2. **Target Denomination Calculation:**
   - **Break Operation:** Searches for denomination one level smaller (den-1)
   - **Join Operation:** Searches for denomination one level larger (den+1)

3. **Available Coin Discovery:**
   - Iterates through all pages for target denomination
   - Uses on-demand page cache with reservation checking
   - Skips reserved pages (reserved by other sessions)
   - Reserves each accessed page for the requesting session
   - Identifies coins with zero MFS (available for allocation)
   - Collects up to MAX_CHANGE_COINS available serial numbers

4. **Response Generation:**
   - First byte: Target denomination
   - Following bytes: Available serial numbers (4 bytes each)
   - Limited to MAX_CHANGE_COINS results

**Used By:** Change-making preparation workflows

**Dependencies:** Database layer for page access, reservation system

### 2. Break Operation (`cmd_break`)
**Parameters:**
- Connection information structure
- Input: 253-byte payload with session ID, coin to break, and 10 smaller coins data

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Breaks a single larger denomination coin into 10 smaller denomination coins with secure validation and dual hashing support.

**Process:**
1. **Request Validation:**
   - Validates exact 253-byte payload size
   - Extracts session ID and larger coin data (denomination + serial number + authentication number)
   - Validates larger coin denomination within valid range
   - Applies rate limiting to prevent abuse

2. **Large Coin Authentication:**
   - Retrieves page for the coin to be broken
   - Verifies provided authentication number matches stored value
   - Ensures coin is authentic before proceeding with break operation

3. **Smaller Coins Validation:**
   - Processes each of the 10 smaller coins in the request
   - Validates each coin has denomination exactly one level smaller (bden-1)
   - Verifies each target page is reserved by the requesting session
   - Ensures all smaller coin slots are available

4. **Ownership Transfer:**
   - Takes ownership of all 10 smaller coins
   - Copies provided authentication numbers to coin storage
   - Sets MFS timestamp for all newly owned coins
   - Marks all affected pages as dirty for persistence

5. **Large Coin Destruction (Dual Hash Support):**
   - Generates new random authentication number for the broken coin
   - **Legacy Clients (encryption_type < 4):** Uses MD5 hash for AN generation
   - **Modern Clients (encryption_type >= 4):** Uses SHA-256 hash for AN generation
   - Sets MFS to 0 (marks coin as available)
   - Ensures monetary conservation (1 large = 10 small)

**Security Features:**
- Authentication required before breaking
- Session-based page reservation prevents conflicts
- Atomic operation (succeeds completely or fails entirely)
- Rate limiting prevents abuse
- Dual hashing maintains backward compatibility

**Used By:** Client change-making interfaces, wallet break operations

### 3. Join Operation (`cmd_join`)
**Parameters:**
- Connection information structure  
- Input: 253-byte payload with session ID, target large coin, and 10 smaller coins data

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Joins 10 smaller denomination coins into a single larger denomination coin with complete validation and monetary conservation.

**Process:**
1. **Request Validation:**
   - Validates exact 253-byte payload size
   - Extracts session ID and target large coin data
   - Validates target denomination within valid range

2. **Target Page Verification:**
   - Retrieves page for the target large coin
   - Verifies page is reserved by the requesting session
   - Ensures target coin slot is available for creation

3. **Smaller Coins Authentication:**
   - Validates each of the 10 smaller coins
   - Verifies each coin has denomination exactly one level smaller
   - Authenticates each coin by comparing stored authentication numbers
   - Ensures all provided authentication numbers match stored values
   - Fails entire operation if any coin is not authentic

4. **Smaller Coins Destruction:**
   - Sets MFS to 0 for all authenticated smaller coins
   - Marks all affected pages as dirty
   - Effectively removes the 10 smaller coins from circulation

5. **Large Coin Creation:**
   - Creates the new larger denomination coin
   - Uses client-provided authentication number
   - Sets MFS timestamp for the new coin
   - Ensures monetary conservation (10 small = 1 large)

**Security Features:**
- All 10 smaller coins must be authentic before operation proceeds
- Session-based reservation prevents race conditions
- Atomic operation ensures consistent state
- Monetary conservation enforced

**Used By:** Client change-making interfaces, wallet join operations

## Data Structures and Formats

### Request Formats
| Operation | Size | Structure |
|-----------|------|-----------|
| Get Available SNs | 24 bytes | Session ID (4) + Operation Type (1) + Denomination (1) + Reserved (18) |
| Break Operation | 253 bytes | Session ID (4) + Large Coin Data (21) + 10 Small Coins Data (210) + Reserved (18) |
| Join Operation | 253 bytes | Session ID (4) + Target Coin Data (21) + 10 Small Coins Data (210) + Reserved (18) |

### Coin Data Format
| Field | Size | Description |
|-------|------|-------------|
| Denomination | 1 byte | Coin denomination identifier |
| Serial Number | 4 bytes | Unique coin serial number |
| Authentication Number | 16 bytes | Coin ownership proof |

### Response Formats
| Operation | Response Format |
|-----------|----------------|
| Get Available SNs | Target denomination (1 byte) + Serial numbers (4 bytes each, up to MAX_CHANGE_COINS) |
| Break/Join | Status code only |

## Security Considerations

### Authentication and Authorization
- **Ownership Verification:** All coins must be proven authentic before processing
- **Session Management:** Page reservation system prevents concurrent conflicts
- **Rate Limiting:** Break operations subject to IP-based rate limiting

### Monetary Conservation
- **1:10 Ratio:** Break operations always maintain 1 large coin = 10 smaller coins
- **Atomic Operations:** All changes succeed together or fail together
- **Audit Trail:** MFS timestamps track all coin state changes

### Dual Hash Support
- **Backward Compatibility:** Legacy MD5 support for older clients
- **Forward Security:** SHA-256 for modern clients
- **Client Detection:** Automatic algorithm selection based on encryption type

## Error Handling and Validation

### Input Validation
- **Size Validation:** Exact payload sizes required for each operation
- **Denomination Validation:** All coin denominations must be within valid ranges
- **Operation Validation:** Operation types must be valid (OP_BREAK or OP_JOIN)

### Runtime Validation
- **Authentication Validation:** All provided authentication numbers verified
- **Reservation Validation:** Required pages must be reserved by requesting session
- **Availability Validation:** Target coin slots must be available

### Error Recovery
- **Resource Cleanup:** Page locks released on error conditions
- **State Consistency:** Failed operations leave coin state unchanged
- **Memory Management:** Response buffers freed on allocation failures

## Performance Characteristics

### On-Demand Cache Integration
- **Efficient Page Access:** Uses database layer's on-demand caching
- **Memory Conservation:** Only accessed pages loaded into memory
- **Lock Management:** Proper page locking ensures thread safety

### Reservation System Benefits
- **Conflict Prevention:** Session-based reservations prevent race conditions
- **Resource Management:** Automatic reservation timeout and cleanup
- **Scalability:** Multiple concurrent change operations supported

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Utilities Module:** Dual hash functions (MD5 and SHA-256)
- **Configuration System:** Server identification and denomination limits
- **Rate Limiting:** IP-based request throttling

### External Constants Required
- `TOTAL_PAGES`: Total number of pages per denomination
- `RECORDS_PER_PAGE`: Number of coin records per page
- `MIN_DENOMINATION`, `MAX_DENOMINATION`: Valid denomination range
- `RESERVED_PAGE_RELEASE_SECONDS`: Page reservation timeout

### Used By
- **Client Wallets:** Direct change-making operations
- **Trading Systems:** Change preparation for transactions
- **Administrative Tools:** Bulk denomination management

### Cross-File Dependencies
- **Database Layer:** Page locking, reservation, and persistence
- **Utilities Module:** Hash generation and random number functions
- **Configuration Module:** Server settings and denomination configuration
- **Rate Limiting Module:** IP-based throttling for break operations

## Threading and Concurrency
- **Page Locking:** Thread-safe access through database layer locking
- **Session Isolation:** Each session operates on reserved pages only
- **Atomic Operations:** Individual change operations are atomic
- **Resource Safety:** Proper cleanup ensures no resource leaks

## Backward Compatibility
- **Dual Hash Support:** Automatic selection of MD5 or SHA-256 based on client version
- **Protocol Versioning:** Client encryption type determines hash algorithm
- **Legacy Integration:** Seamless operation with older client implementations

This change-making module provides essential denomination conversion functionality for the RAIDA network, ensuring monetary conservation, security, and backward compatibility while supporting efficient concurrent operations through the reservation system.