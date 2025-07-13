# Executive Commands Implementation (cmd_executive)

## Module Purpose
This module implements administrative and executive commands for coin lifecycle management in the RAIDA network. It provides secure, authenticated operations for coin creation, deletion, liberation, and comprehensive system auditing. All operations require administrative authentication and use enhanced security measures with mandatory admin key validation from configuration.


## constants
| Constant              | Description                                                             |
| --------------------- | ----------------------------------------------------------------------- |
| `DENOMINATION_RANGE`  | Allowed denomination values: integers from `-8` to `+6` inclusive       |
| `MAX_AVAILABLE_COINS` | Max number of available coins returned per denomination (e.g., `1029`)  |
| `RECORDS_PER_PAGE`    | Number of coin records per data page                                    |
| `TOTAL_PAGES`         | Number of pages allocated per denomination                              |
| `MFS`                 | "Months From Start"; non-zero means coin is in circulation              |
| `SESSION_ID_OVERRIDE` | Special session ID (`0xeeeeeeee`) that bypasses reservation enforcement |


## page file structure 
| Component           | Description                                                             |
| ------------------- | ----------------------------------------------------------------------- |
| Base Path           | Root path to data storage                                               |
| Denomination Folder | Folder per denomination (e.g., `/Data/-4/`)                             |
| High Byte Subfolder | Splits page ID using high byte for directory nesting                    |
| Page File           | File path format: `/<base_path>/<denomination>/<hi_byte>/<page_id>.bin` |
| Addressing Format   | `serial_number = page_number × RECORDS_PER_PAGE + offset`               |


## Core Functionality

### 1. Available Serial Numbers Discovery (`cmd_get_available_sns`)
**Parameters:**
- Connection information structure
- Input: 54-byte payload containing session ID, admin key, and denomination selection flags

**Returns:** None (modifies connection structure with available serial number ranges and individual coins)

**Purpose:** Discovers and reserves available coin serial numbers across requested denominations for subsequent coin creation operations.

**Process:**
1. **Authentication and Validation:**
   - Validates exact payload size (54 bytes)
   - **SECURITY ENHANCEMENT:** Direct comparison against config.admin_key (no hardcoded fallbacks)
   - Extracts session ID for page reservation tracking
   - Validates administrative authorization before proceeding

2. **Denomination Selection:**
   - Processes denomination flags array (16 bytes) indicating requested denominations
   - Iterates through all supported denominations (-8 to +6)
   - Skips denominations not requested via flags

3. **Serial Number Discovery:**
   - For each requested denomination:
     - Scans all possible pages using on-demand cache system
     - Skips pages already reserved by other sessions
     - Reserves discovered pages with provided session ID
     - Identifies ranges of consecutive available serial numbers
     - Identifies individual available serial numbers

4. **Range Optimization:**
   - Groups consecutive available coins into ranges for efficiency
   - Single coins stored as individual entries
   - Limits total discovery to MAX_AVAILABLE_COINS per denomination
   - Optimizes response size through range compression

5. **Response Construction:**
   - Per denomination: 1-byte denomination + 1-byte range count + 1-byte individual count
   - Range data: 8 bytes per range (4-byte start + 4-byte end)
   - Individual data: 4 bytes per serial number
   - Variable response size based on availability

**Session Management:**
- Pages reserved during discovery to prevent concurrent allocation
- Session ID enables subsequent coin creation operations
- Prevents race conditions in multi-administrator environments

### 2. Coin Creation (`cmd_create_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) containing session ID, admin key, and coin specifications

**Returns:** None (modifies connection structure with previous authentication numbers)

**Purpose:** Creates new coins in the system by generating unique authentication numbers and establishing initial ownership.

**Process:**
1. **Authentication and Validation:**
   - Validates minimum payload size (43 bytes)
   - **SECURITY ENHANCEMENT:** Direct admin key validation against configuration
   - Extracts session ID for page reservation verification
   - Validates coin data alignment (5 bytes per coin)

2. **Coin Creation Logic:**
   - For each coin specification:
     - Validates denomination and serial number
     - Verifies page is reserved by provided session ID (or special override)
     - Generates unique authentication number using cryptographic method

3. **Authentication Number Generation:**
   - Combines RAIDA server number, serial number, and admin key
   - Uses MD5 hash for cryptographically derived authentication number
   - Ensures uniqueness across all coins and servers
   - Provides deterministic but unpredictable authentication numbers

4. **Coin Record Creation:**
   - Stores previous authentication number in response (for audit)
   - Updates coin record with new authentication number
   - Sets MFS (Months From Start) timestamp
   - Marks database page as dirty for persistence

**Security Features:**
- Session-based page reservation prevents unauthorized creation
- Cryptographic authentication number generation
- Admin key incorporation ensures server-specific uniqueness
- Audit trail through previous authentication number return

### 3. Coin Liberation (`cmd_free_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) containing admin key and coin specifications

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Frees coins from circulation, making them available for future allocation without changing authentication numbers.

**Process:**
1. **Authentication and Validation:**
   - Validates minimum payload size and admin key
   - **SECURITY ENHANCEMENT:** Direct admin key validation
   - Validates coin data alignment (5 bytes per coin)

2. **Coin Liberation Logic:**
   - For each specified coin:
     - Validates denomination and serial number
     - Retrieves coin data using on-demand cache
     - Sets MFS to 0 to mark coin as not in circulation
     - Marks database page as dirty for persistence

3. **Liberation Effects:**
   - Coins become available for future allocation
   - Authentication numbers remain unchanged
   - Coins removed from active circulation
   - Enables coin recycling without re-authentication

**Use Cases:**
- Administrative coin management
- Cleaning up unused or test coins
- Preparing coins for reallocation
- System maintenance operations

### 4. Authenticated Coin Deletion (`cmd_delete_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) containing admin key and coin authentication data

**Returns:** None (modifies connection structure with deletion results bitmap)

**Purpose:** Securely deletes coins from circulation after verifying ownership through authentication number validation.

**Process:**
1. **Authentication and Validation:**
   - Validates minimum payload size (55 bytes)
   - **SECURITY ENHANCEMENT:** Direct admin key validation
   - Validates coin data alignment (21 bytes per coin)

2. **Ownership Verification:**
   - For each coin deletion request:
     - Extracts denomination, serial number, and authentication number
     - Retrieves current coin data using on-demand cache
     - Compares provided authentication number with stored value
     - Only deletes coins with matching authentication numbers

3. **Secure Deletion Process:**
   - Sets bit in response bitmap for successfully deleted coins
   - Sets MFS to 0 to remove coin from circulation
   - Marks database page as dirty for persistence
   - Counts successful and failed deletions

4. **Response Generation:**
   - **STATUS_ALL_PASS:** All coins successfully deleted
   - **STATUS_ALL_FAIL:** No coins deleted (authentication failures)
   - **STATUS_MIXED:** Partial deletion with detailed bitmap

**Security Features:**
- Requires proof of ownership before deletion
- Prevents unauthorized coin destruction
- Detailed audit trail through bitmap responses
- Atomic operation per coin

### 5. Comprehensive System Audit (`cmd_get_all_sns`)
**Parameters:**
- Connection information structure
- Input: 35-byte payload containing admin key and target denomination

**Returns:** None (modifies connection structure with complete coin bitmap)

**Purpose:** Provides comprehensive audit of all coins in circulation for a specific denomination by directly reading from disk.

**Process:**
1. **Authentication and Validation:**
   - Validates exact payload size (35 bytes)
   - **SECURITY ENHANCEMENT:** Direct admin key validation
   - Validates requested denomination within acceptable range

2. **Performance Optimization Strategy:**
   - **CRITICAL:** Bypasses memory cache to avoid cache pollution
   - Reads page files directly from disk for administrative audit
   - Prevents interference with normal operational cache

3. **Complete Audit Process:**
   - Iterates through all possible pages for denomination
   - Constructs file path for each page using hierarchical structure
   - Reads page data directly from disk storage
   - Scans all coin records for circulation status (MFS ≠ 0)

4. **Bitmap Construction:**
   - Creates bitmap with one bit per possible serial number
   - Sets bit to 1 for coins in circulation
   - Calculates bitmap size: (TOTAL_PAGES × RECORDS_PER_PAGE) / 8
   - Returns denomination + size + complete bitmap

**Administrative Features:**
- Complete system state visibility
- Cache-independent operation for reliability
- Efficient bitmap representation
- Suitable for compliance and auditing

## Enhanced Security Architecture

### Mandatory Administrative Authentication
- **SECURITY IMPROVEMENT:** Removed all hardcoded administrative keys
- **Configuration-Based:** All admin keys must be specified in config.toml
- **Direct Validation:** Admin key compared directly against config.admin_key
- **No Fallbacks:** Server refuses to start without proper admin key configuration

### Administrative Key Management
- **16-Byte Keys:** Cryptographically strong administrative authentication
- **Configuration Security:** Keys stored securely in configuration system
- **Access Control:** All executive operations require valid admin key
- **Audit Trail:** All administrative operations logged with authentication

### Cryptographic Security
- **Authentication Number Generation:** MD5-based deterministic but unpredictable generation
- **Server-Specific:** Incorporates RAIDA server number for uniqueness
- **Key Incorporation:** Uses admin key as cryptographic input
- **Collision Resistance:** Combination ensures unique authentication numbers

## Session and Reservation Management

### Session-Based Operations
- **Page Reservation:** Available serial number discovery reserves pages
- **Session Tracking:** Coin creation validates session-based reservations
- **Conflict Prevention:** Prevents concurrent administrative operations
- **Resource Management:** Automatic cleanup of expired reservations

### Administrative Override
- **Special Session:** Session ID 0xeeeeeeee bypasses reservation checks
- **Emergency Operations:** Enables administrative override when necessary
- **Audit Implications:** Override usage logged for security monitoring

## Data Structures and Formats

### Input Data Formats
- **Available SNs Query:** 54 bytes (4-byte session + 16-byte admin key + 16-byte denomination flags + padding)
- **Coin Creation:** Variable size (20-byte header + 5 bytes per coin: denomination + serial)
- **Coin Liberation:** Variable size (20-byte header + 5 bytes per coin: denomination + serial)
- **Coin Deletion:** Variable size (16-byte admin key + 21 bytes per coin: denomination + serial + authentication)
- **System Audit:** 35 bytes (16-byte admin key + 1-byte denomination + padding)

### Output Data Formats
- **Available SNs:** Variable size per denomination (denomination + range count + individual count + range/individual data)
- **Coin Creation:** 16 bytes per coin (previous authentication numbers)
- **Coin Operations:** Bitmap responses for batch operations
- **System Audit:** 5-byte header + bitmap (denomination + 4-byte size + bitmap data)

### Administrative Data
- **Admin Key:** 16-byte cryptographic authentication key
- **Session ID:** 32-bit session identifier for reservation tracking
- **Denomination Flags:** 16-byte array indicating requested denominations
- **MFS Timestamps:** Months From Start values for coin lifecycle tracking

## Error Handling and Validation

### Authentication Errors
- `ERROR_ADMIN_AUTH`: Administrative authentication failure
- **Security Policy:** No detailed error information to prevent key enumeration
- **Audit Logging:** All authentication failures logged for security monitoring

### Operational Errors
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size for operation
- `ERROR_COINS_NOT_DIV`: Coin data not aligned to record boundaries
- `ERROR_INVALID_SN_OR_DENOMINATION`: Invalid coin reference
- `ERROR_PAGE_IS_NOT_RESERVED`: Attempt to create coin on unreserved page
- `ERROR_MEMORY_ALLOC`: Memory allocation failure for responses

### Data Validation
- **Size Validation:** All operations validate exact or minimum payload sizes
- **Alignment Validation:** Coin data must align to proper record boundaries
- **Range Validation:** Denominations and serial numbers within valid ranges
- **Session Validation:** Page reservations verified before coin creation

### Recovery Mechanisms
- **Atomic Operations:** Failed operations leave system state unchanged
- **Resource Cleanup:** Memory and page locks properly released on errors
- **Partial Success:** Mixed operations report detailed success/failure information

## Performance Characteristics

### On-Demand Cache Integration
- **Discovery Operations:** May trigger extensive cache loading for availability scanning
- **Creation Operations:** Benefit from cached pages during coin creation
- **Audit Operations:** Bypass cache entirely for independent verification

### Administrative Efficiency
- **Range Compression:** Consecutive available coins grouped into ranges
- **Batch Operations:** Multiple coins processed in single administrative request
- **Session Optimization:** Page reservations reduce repeated validation overhead

### Resource Management
- **Memory Usage:** Dynamic allocation based on actual availability and request size
- **Disk I/O:** Audit operations use direct disk access for independence
- **Cache Impact:** Discovery operations may impact operational cache performance

## Security Considerations

### Administrative Access Control
- **Strong Authentication:** 16-byte cryptographic keys required for all operations
- **Configuration Security:** Admin keys must be properly configured before server startup
- **Session Security:** Session-based operations prevent unauthorized access
- **Audit Trail:** All administrative operations logged with full context

### Cryptographic Security
- **Authentication Number Generation:** Uses cryptographically secure methods
- **Key Incorporation:** Admin keys incorporated into coin authentication generation
- **Deterministic Generation:** Reproducible but unpredictable authentication numbers
- **Collision Resistance:** Multiple factors ensure unique authentication numbers

### Operational Security
- **Audit Independence:** System audit operations independent of operational cache
- **Resource Protection:** Page reservations prevent concurrent modification conflicts
- **Error Handling:** Security-conscious error responses prevent information leakage
- **Access Logging:** Administrative operations provide comprehensive audit trails

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache with reservation system
- **Configuration System:** Administrative key management and validation
- **Cryptographic Utilities:** MD5 hashing for authentication number generation
- **Logging System:** Administrative operation audit trails
- **File System:** Direct disk access for independent audit operations

### External Constants Required
- `MAX_AVAILABLE_COINS`: Limit for serial number discovery operations
- `TOTAL_PAGES`: Maximum pages per denomination for complete scanning
- `RECORDS_PER_PAGE`: Database organization constant
- `MIN_DENOMINATION` / `MAX_DENOMINATION`: Valid denomination range
- `ERROR_*` / `STATUS_*`: Protocol error and status definitions

### Used By
- **Administrative Tools:** System management and maintenance operations
- **Compliance Systems:** Audit and reporting functionality
- **Emergency Operations:** System recovery and maintenance procedures
- **Development Tools:** Testing and development support operations

## Administrative Workflow Patterns

### Coin Creation Workflow
1. **Discovery:** Query available serial numbers for target denominations
2. **Reservation:** Pages automatically reserved during discovery
3. **Creation:** Create coins using reserved pages and session ID
4. **Verification:** Optionally audit created coins for confirmation

### System Maintenance Workflow
1. **Audit:** Complete system audit to identify issues
2. **Liberation:** Free unused or problematic coins
3. **Recreation:** Create replacement coins as needed
4. **Verification:** Confirm system integrity after maintenance

### Emergency Recovery Workflow
1. **Assessment:** Comprehensive audit to understand system state
2. **Cleanup:** Delete or free problematic coins
3. **Restoration:** Create replacement coins for lost value
4. **Validation:** Verify system integrity and value conservation

## Monitoring and Observability

### Administrative Metrics
- **Operation Counts:** Track frequency of administrative operations
- **Success Rates:** Monitor success/failure rates for each operation type
- **Response Times:** Track performance of administrative operations
- **Resource Usage:** Monitor memory and disk usage during operations

### Security Monitoring
- **Authentication Attempts:** Log all administrative authentication attempts
- **Access Patterns:** Monitor timing and frequency of administrative access
- **Error Rates:** Track authentication and operation failures
- **Audit Trails:** Comprehensive logging of all administrative activities

### Operational Impact
- **Cache Performance:** Monitor impact of discovery operations on cache
- **System Load:** Track resource usage during administrative operations
- **Availability:** Monitor system responsiveness during administrative tasks

## Future Enhancements

### Performance Improvements
- **Parallel Discovery:** Multi-threaded availability scanning for large systems
- **Incremental Audit:** Delta-based auditing for improved performance
- **Smart Caching:** Predictive caching for administrative operations
- **Batch Optimization:** Enhanced batch processing for large operations

### Security Enhancements
- **Multi-Factor Authentication:** Enhanced administrative authentication
- **Role-Based Access:** Granular permissions for different administrative operations
- **Hardware Security:** HSM integration for administrative key protection
- **Advanced Auditing:** Enhanced audit trails with cryptographic verification

### Operational Features
- **Scheduled Operations:** Automated administrative operations
- **Distributed Administration:** Multi-administrator coordination
- **Recovery Automation:** Automated system recovery procedures
- **Compliance Reporting:** Enhanced reporting for regulatory compliance

This executive commands module provides the essential administrative foundation for secure, efficient management of the RAIDA coin ecosystem while maintaining strict security controls and comprehensive audit capabilities.