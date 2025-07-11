# Status and Monitoring Commands Implementation (cmd_status)

## Module Purpose
This module implements essential server status and monitoring commands for the RAIDA network. It provides health check capabilities, version information, comprehensive audit functionality for coin circulation analysis, and administrative statistics reporting. The module has been optimized for the on-demand page cache system to minimize memory usage while maintaining accurate reporting.

## Core Functionality

### Constants Used
- `STATUS_SUCCESS = 0x00`
- `ERROR_MEMORY_ALLOC = 0x01`
- `ERROR_ADMIN_AUTH = 0x02`
- `VERSION`: `uint8_t[8]` — build version from build system


### 1. Health Check Echo (`cmd_echo`)
**Parameters:**
- Connection information structure

**Returns:** None (modifies connection structure)

**Purpose:** Provides a simple health check endpoint to verify server responsiveness and basic functionality.

**Process:**
1. Logs operation start
2. Sets output size to zero (no response body)
3. Sets success status
4. Increments echo statistics counter
5. Logs operation completion

**Dependencies:**
- Statistics system for operation counting
- Logging system for debug output

**Used By:** 
- Network monitoring systems
- Load balancers for health checking
- Client applications for connectivity testing

### 2. Version Information (`cmd_version`)
**Parameters:**
- Connection information structure

**Returns:** None (modifies connection structure with version data)

**Purpose:** Returns the server's build version information for compatibility checking and debugging.

**Process:**
1. Allocates 8-byte response buffer
2.  Constructs a response message containing an 8-character version string in fixed-length format. If the string is not available, returns ERROR_MEMORY_ALLOC.
3. Copies version constant to response buffer
4. Sets output size to 8 bytes
5. Sets success status

**Dependencies:**
- Memory management for response allocation
- Version constants from build system

**Used By:**
- Client applications for compatibility verification
- Administrative tools for version tracking
- Debugging and support operations

### 3. Coin Circulation Audit (`cmd_audit`)
**Parameters:**
- Connection information structure

**Returns:** None (modifies connection structure with circulation data)

**Purpose:** Provides comprehensive audit information showing the number of issued coins per denomination across the entire system. Optimized to read directly from disk to avoid polluting the memory cache.

**Process:**
1. Allocates response buffer for all denominations (4 bytes per denomination)
2. For each denomination from minimum to maximum:
   - Iterates through all possible page files on disk
   - Constructs page file path using denomination and page number
   - Attempts to open each page file directly
   - Reads complete page data into temporary buffer
   - Counts coins in circulation (MFS ≠ 0) from buffer data
   - Accumulates count for the denomination
3. Formats response with denomination and count for each denomination
4. Sets total response size and success status

**Performance Optimization:**
- Bypasses memory cache to avoid pollution during large scans
- Reads page files directly from filesystem
- Uses efficient counting without loading pages into cache
- Handles missing page files gracefully (expected for unused ranges)

**Dependencies:**
- Filesystem operations for direct page file access
- Configuration system for base directory paths
- Memory management for response buffer allocation
- Logging system for error reporting

**Used By:**
- Auditing systems for compliance reporting
- Financial analysis tools
- Regulatory reporting systems
- System health monitoring

### 4. Administrative Statistics (`cmd_show_stats`)
**Parameters:**
- Connection information structure
- Input: 16-byte payload containing administrative authentication key

**Returns:** None (modifies connection structure with statistics data)

**Purpose:** Returns comprehensive server statistics for administrative monitoring and performance analysis. Requires administrative authentication for access.

**Process:**
1. Validates payload contains exactly 16 bytes
2. Extracts authentication key from payload
3. Compares provided key with configured administrative key
4. If authentication succeeds:
   - Allocates response buffer for complete statistics structure
   - Copies current statistics data to response buffer
   - Sets output size to statistics structure size
   - Sets success status
5. If authentication fails:
   - Sets authentication error status
   - Logs authentication failure

**Security Features:**
- Administrative key validation prevents unauthorized access to sensitive statistics
- Constant-time comparison to prevent timing attacks
- Error logging for security audit trails

**Dependencies:**
- Configuration system for administrative key storage
- Statistics system for data collection and access
- Memory management for response allocation
- Logging system for security event recording

**Used By:**
- Administrative monitoring dashboards
- Performance analysis tools
- System health monitoring systems
- Operational reporting systems

## Data Structures and Output Formats

### conn_info_t
Fields:
  - bodySize: Integer
  - commandStatus: Enum(StatusCode)
  - output: BinaryBuffer or String (based on command)
  - outputSize: Integer

### stats_s
- Structure should include key metrics (e.g., `ECHO_FIELD_IDX`, request counts)
- Mention if it's fixed-size or dynamically extended


### Version Response Format
- **Size:** 8 bytes
- **Content:** Build version identifier
- **Encoding:** Binary version data

### Audit Response Format
- **Per Denomination:** 4 bytes total
  - 1 byte: Denomination identifier (-8 to +6)
  - 3 bytes: Coin count (big-endian format)
- **Total Size:** 4 bytes × 15 denominations = 60 bytes
- **Order:** Sequential from lowest to highest denomination

### Statistics Response Format
- **Size:** Complete statistics structure size (implementation-dependent)
- **Content:** Binary dump of server statistics
- **Includes:** Operation counts, timing data, resource usage metrics

### Error Conditions
- `ERROR_MEMORY_ALLOC`: Memory allocation failure for response buffers
- `ERROR_ADMIN_AUTH`: Administrative authentication failure for statistics
- Filesystem errors during audit operations (logged but not fatal)


### Memory Allocation Errors
If malloc fails:
- Set `ci->command_status = ERROR_MEMORY_ALLOC`
- Set `ci->output_size = 0`
- Do not write to `ci->output`


## Performance Characteristics

### Echo Command
- **Latency:** Minimal - simple counter increment and status setting
- **Memory:** No dynamic allocation
- **I/O:** None
- **Scalability:** Handles high request volumes efficiently

### Version Command
- **Latency:** Low - single memory allocation and copy
- **Memory:** 8-byte allocation per request
- **I/O:** None
- **Scalability:** Lightweight operation suitable for frequent queries

### Audit Command
- **Latency:** High - scans entire database on disk
- **Memory:** Single page buffer reused across all operations
- **I/O:** Intensive - reads all existing page files
- **Scalability:** Resource-intensive operation, should be used sparingly
- **Optimization:** Direct disk access avoids memory cache pollution

### Statistics Command
- **Latency:** Low - single structure copy after authentication
- **Memory:** Statistics structure size allocation
- **I/O:** None after authentication
- **Scalability:** Fast operation but requires administrative privileges

## Security Considerations

### Authentication and Authorization
- Administrative statistics require valid authentication key
- Echo and version commands are publicly accessible
- Audit command provides read-only system information

### Information Disclosure
- Version information may reveal system details to attackers
- Audit data provides coin circulation patterns
- Statistics contain operational intelligence requiring protection

### Access Control
- Administrative key stored securely in configuration
- Authentication failures logged for security monitoring
- No sensitive data exposed through public endpoints

## File System Dependencies

### Audit Operation File Access
- **Directory Structure:** `{base_path}/Data/{denomination_hex}/{page_msb_hex}/{page_number_hex}.bin`
- **File Format:** Binary page files containing coin records
- **Access Pattern:** Sequential read-only access to all existing files
- **Error Handling:** Missing files treated as empty (normal condition)

### Page File Organization
- **Denominations:** Hexadecimal directory names (00-0E for denominations -8 to +6)
- **Page MSB:** Subdirectory based on upper 8 bits of page number
- **Page Files:** 4-digit hexadecimal page number with .bin extension
- **Record Format:** 17 bytes per coin record within each page

## Dependencies and Integration

### Required Modules
- **Configuration System:** Administrative keys, base directory paths, server identification
- **Statistics System:** Counter management, data collection, thread-safe access
- **Filesystem Interface:** Direct file access for audit operations
- **Memory Management:** Dynamic allocation for response buffers
- **Logging System:** Debug output, error reporting, security event logging

### External Constants Required
- `TOTAL_DENOMINATIONS`: Number of denomination types (15)
- `MIN_DENOMINATION` / `MAX_DENOMINATION`: Denomination range bounds (-8 to +6)
- `TOTAL_PAGES`: Maximum pages per denomination
- `RECORDS_PER_PAGE`: Records per page file (1024)
- `ERROR_*`: Protocol error code definitions
- `STATUS_SUCCESS`: Success status indicator
- `VERSION`: Build version constant

### Used By
- **Network Protocol Layer:** Command routing and response handling
- **Monitoring Systems:** Health checking and status reporting
- **Administrative Tools:** System management and analysis
- **Client Applications:** Version compatibility and health verification

## Threading and Concurrency

### Thread Safety
- Echo statistics updates use atomic operations
- Statistics access protected by appropriate synchronization
- Audit operations read-only with no shared state modifications
- Version command is stateless and thread-safe

### Performance Impact
- Echo command has minimal thread contention
- Audit command performs extensive I/O but doesn't affect memory cache
- Statistics command requires brief lock for data copying
- Version command is completely lock-free

## Monitoring and Observability

### Operation Metrics
- Echo command increments request counters for monitoring
- All commands contribute to execution time statistics
- Error conditions logged with appropriate detail levels
- Administrative access attempts logged for security audit

### Health Monitoring
- Echo command serves as primary health check endpoint
- Version command enables compatibility monitoring
- Audit command provides data integrity verification
- Statistics command offers comprehensive operational insight

This module provides essential observability and health checking capabilities for the RAIDA network, enabling effective monitoring, debugging, and operational management while maintaining security and performance requirements.