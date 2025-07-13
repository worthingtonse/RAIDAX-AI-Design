# Filesystem Commands Implementation (cmd_fs)

## Module Purpose
This module implements secure filesystem operations for the RAIDA server, providing authenticated object storage, retrieval, and management capabilities. It includes comprehensive security measures with path traversal prevention, administrative authentication, and secure file operations within a controlled directory structure.

## Core Functionality

### 1. Object Retrieval (`cmd_get_object`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 35 bytes) containing admin key and file path

**Returns:** None (modifies connection structure with file contents)

**Purpose:** Securely retrieves objects from the filesystem with comprehensive path validation and access control.

**Process:**
1. **Authentication and Validation:**
   - Validates minimum payload size (35 bytes minimum)
   - **SECURITY ENHANCEMENT:** Direct admin key validation against config.admin_key
   - Extracts user-provided file path from payload
   - Validates path length to prevent buffer overflow attacks

2. **Path Security Processing:**
   - Constructs base directory path: `{config.cwd}/Folders/`
   - Combines base path with user-provided relative path
   - **CRITICAL SECURITY:** Uses realpath() to resolve absolute path and prevent traversal
   - Verifies resolved path remains within designated Folders directory

3. **Path Traversal Prevention:**
   - Checks that resolved path starts with base directory path
   - Rejects any attempt to access files outside authorized area
   - Prevents `../` and other directory traversal attacks
   - Returns generic authentication error to avoid information disclosure

4. **File Access and Validation:**
   - Uses stat() to verify file exists and is a regular file
   - Rejects directories, symbolic links, and special files
   - Extracts file size for response buffer allocation
   - Opens file in read-only mode for security

5. **Content Retrieval:**
   - Allocates response buffer based on actual file size
   - Reads complete file contents into response buffer
   - Validates that entire file was read successfully
   - Returns complete file contents to client

**Security Features:**
- **Path Traversal Protection:** realpath() resolution prevents directory escape
- **Access Control:** Administrative authentication required
- **File Type Validation:** Only regular files allowed
- **Error Information Control:** Generic errors prevent information leakage

### 2. Object Storage (`cmd_put_object`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 40 bytes) containing admin key, file size, filename, and content

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Securely stores objects in the filesystem with comprehensive validation and security controls.

**Process:**
1. **Authentication and Validation:**
   - Validates minimum payload size (40 bytes)
   - **SECURITY ENHANCEMENT:** Direct admin key validation
   - Extracts file size from payload (4-byte big-endian value)
   - Validates filename is null-terminated and within length limits

2. **Payload Structure Validation:**
   - Validates total body size matches expected structure:
     - 16 bytes: Admin key
     - 4 bytes: File size
     - Variable: Null-terminated filename
     - Variable: File content (matching declared size)
     - 2 bytes: Protocol trailer
   - Rejects requests with size mismatches to prevent attacks

3. **Path Security Processing:**
   - Constructs base directory path: `{config.cwd}/Folders/`
   - Combines with user-provided filename
   - Resolves directory portion to verify it exists within base
   - **SECURITY:** Only allows writing to existing subdirectories

4. **Directory Validation:**
   - Temporarily removes filename to validate directory path
   - Uses realpath() to resolve directory and prevent traversal
   - Ensures write location is within authorized area
   - Restores filename after validation

5. **File Creation:**
   - Opens file with O_WRONLY | O_CREAT | O_TRUNC flags
   - Sets file permissions to 0644 (readable by owner/group, writable by owner)
   - Writes complete file content in single operation
   - Validates that entire content was written successfully

**Security Features:**
- **Directory Confinement:** Only allows writing within Folders subdirectory
- **Path Validation:** Prevents directory traversal for write operations
- **Size Validation:** File size must match declared size exactly
- **Atomic Writing:** Complete file written in single operation

### 3. Object Removal (`cmd_rm_object`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 35 bytes) containing admin key and file path

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Securely removes objects from the filesystem with path validation and access control.

**Process:**
1. **Authentication and Validation:**
   - Validates minimum payload size (35 bytes)
   - **SECURITY ENHANCEMENT:** Direct admin key validation
   - Extracts file path with length validation
   - Prevents excessively long paths that could cause buffer issues

2. **Path Security Processing:**
   - Constructs complete file path within base directory
   - Uses realpath() to resolve absolute path and prevent traversal
   - Verifies resolved path remains within authorized directory
   - Rejects attempts to access files outside controlled area

3. **Deletion Validation:**
   - Confirms file exists before attempting removal
   - Uses remove() system call for secure deletion
   - Handles deletion errors appropriately
   - Provides success confirmation upon completion

**Security Features:**
- **Path Traversal Protection:** realpath() prevents directory escape attempts
- **Existence Validation:** Verifies file exists before deletion attempt
- **Error Handling:** Appropriate error responses for various failure conditions
- **Access Control:** Requires administrative authentication

### 4. Cryptographic Key Retrieval (`get_crypto_key`)
**Parameters:**
- Ticker string (currency/asset identifier)
- Size output pointer (receives key size)

**Returns:** Character buffer pointer containing key data (NULL on failure)

**Purpose:** Internal helper function for retrieving cryptocurrency private keys from secure filesystem storage.

**Process:**
1. **Key File Location:**
   - Constructs key file path: `{config.cwd}/Folders/{ticker}`
   - Uses ticker symbol as filename within Folders directory
   - Validates file exists and is accessible

2. **File Validation:**
   - Uses stat() to verify file is regular file
   - Extracts file size for buffer allocation
   - Rejects directories and special files

3. **Secure Key Loading:**
   - Allocates buffer based on actual file size
   - Opens file in read-only mode
   - Reads complete key data into buffer
   - Validates entire file was read successfully

4. **Resource Management:**
   - Returns dynamically allocated buffer to caller
   - Caller responsible for freeing returned buffer
   - Proper cleanup on all error conditions

**Used By:**
- **Crossover Operations:** Cryptocurrency withdrawal operations
- **Trading Systems:** Blockchain transaction signing
- **Financial Operations:** Cross-blockchain value transfers

**Security Considerations:**
- **Key Storage:** Keys stored as files in protected directory
- **Access Control:** Function used only by authenticated operations
- **Memory Management:** Secure handling of sensitive key material

## Security Architecture

### Path Traversal Prevention
- **realpath() Resolution:** Converts all paths to absolute canonical form
- **Directory Confinement:** All operations restricted to `{base}/Folders/` directory
- **Traversal Detection:** Any resolved path outside base directory rejected
- **Attack Mitigation:** Prevents `../`, symbolic link, and other traversal attacks

### Authentication and Authorization
- **Administrative Access:** All operations require valid admin key
- **Key Validation:** Direct comparison against configured admin key
- **No Hardcoded Keys:** Admin key must be properly configured
- **Error Handling:** Generic errors prevent authentication oracle attacks

### File Security
- **Type Validation:** Only regular files allowed for read/write operations
- **Permission Control:** Created files have appropriate permissions (0644)
- **Atomic Operations:** File operations completed atomically when possible
- **Error Isolation:** File operation errors don't leak sensitive information


### Path Resolution Rules
- **Base Path:** `{config.cwd}/Folders/` - All operations confined to this area
- **Relative Paths:** User-provided paths treated as relative to base
- **Absolute Resolution:** realpath() used to resolve final absolute paths
- **Validation:** Resolved paths must remain within base directory

### File Organization
- **Cryptocurrency Keys:** Stored directly in Folders/ with ticker as filename
- **User Objects:** Stored in subdirectories or root level as specified
- **Permission Model:** Files created with 0644 permissions (owner read/write, group/other read)

## Data Structures and Formats

### Input Data Formats
- **Get Object:** 16-byte admin key + variable-length null-terminated path + protocol overhead
- **Put Object:** 16-byte admin key + 4-byte size + null-terminated filename + file content + protocol overhead
- **Remove Object:** 16-byte admin key + variable-length null-terminated path + protocol overhead

### Output Data Formats
- **Get Object:** Complete file contents (binary data preserved)
- **Put Object:** Status code only (no response body)
- **Remove Object:** Status code only (no response body)
- **Get Crypto Key:** Dynamically allocated buffer with key data

### Error Responses
- **Authentication Errors:** `ERROR_ADMIN_AUTH` for invalid admin keys
- **File Errors:** `ERROR_FILE_NOT_EXIST`, `ERROR_FILESYSTEM` for file operation failures
- **Parameter Errors:** `ERROR_INVALID_PARAMETER` for invalid paths or parameters
- **Resource Errors:** `ERROR_MEMORY_ALLOC` for memory allocation failures

## Error Handling and Validation

### Input Validation
- **Size Validation:** All operations validate minimum and expected payload sizes
- **Path Validation:** File paths validated for length and format
- **Content Validation:** File contents and sizes validated for consistency
- **Authentication Validation:** Admin keys validated before any file operations

### Security Validation
- **Path Traversal Prevention:** All paths resolved and validated against base directory
- **File Type Validation:** Only regular files allowed for operations
- **Permission Validation:** File permissions verified before access attempts
- **Existence Validation:** File existence verified where appropriate

### Error Conditions
- `ERROR_ADMIN_AUTH`: Administrative authentication failure
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_INVALID_PARAMETER`: Invalid path, filename, or parameters
- `ERROR_FILE_NOT_EXIST`: Requested file does not exist
- `ERROR_FILESYSTEM`: File operation failure (permissions, I/O errors)
- `ERROR_MEMORY_ALLOC`: Memory allocation failure

### Recovery Mechanisms
- **Partial Write Protection:** File operations are atomic when possible
- **Resource Cleanup:** Memory and file descriptors properly released on errors
- **State Consistency:** Failed operations leave filesystem state unchanged
- **Error Reporting:** Detailed error codes aid in troubleshooting

## Performance Characteristics

### File I/O Optimization
- **Single-Pass Operations:** Files read/written in single operations when possible
- **Buffer Management:** Appropriate buffer sizes based on actual file sizes
- **Resource Efficiency:** Minimal resource usage for file operations
- **Error Path Optimization:** Quick failure for invalid operations

### Memory Management
- **Dynamic Allocation:** Buffers allocated based on actual file sizes
- **Automatic Cleanup:** Proper cleanup on all code paths
- **Resource Limits:** Implicit limits through available system memory
- **Leak Prevention:** Careful resource management prevents memory leaks

### Security Performance
- **Path Resolution:** realpath() overhead acceptable for security benefit
- **Authentication:** Direct key comparison for minimal overhead
- **Validation:** Input validation adds minimal processing overhead
- **Error Handling:** Security-conscious error handling with minimal performance impact

## Dependencies and Integration

### Required Modules
- **Configuration System:** Admin key validation and base directory configuration
- **File System APIs:** Standard POSIX file operations (open, read, write, stat, remove)
- **Path Resolution:** realpath() for secure path canonicalization
- **Memory Management:** Dynamic allocation for file contents and buffers

### External Constants Required
- `ERROR_*`: Protocol error code definitions
- `STATUS_SUCCESS`: Success status indicator
- `PATH_MAX`: Maximum path length for system
- File permission constants (O_RDONLY, O_WRONLY, etc.)

### Used By
- **Crossover Operations:** Cryptocurrency key retrieval for blockchain transactions
- **Administrative Tools:** System configuration and key management
- **Backup Systems:** Configuration and key backup/restore operations
- **Development Tools:** Testing and debugging support

## Security Considerations

### File System Security
- **Directory Confinement:** All operations restricted to designated area
- **Path Traversal Prevention:** Comprehensive protection against directory escape
- **Permission Management:** Appropriate file permissions for created objects
- **Error Information Control:** Prevents information leakage through error messages

### Administrative Security
- **Strong Authentication:** Requires valid administrative credentials
- **Access Logging:** File operations logged for security audit
- **Error Handling:** Security-conscious error responses
- **Key Protection:** Secure handling of cryptocurrency private keys

### Operational Security
- **Atomic Operations:** File operations completed atomically when possible
- **Resource Protection:** Prevents resource exhaustion through proper limits
- **Error Recovery:** Graceful handling of file system errors
- **Audit Trail:** Administrative file operations provide audit information

## Threading and Concurrency
- **Thread Safety:** File operations are thread-safe at the system level
- **Resource Safety:** Proper cleanup prevents resource leaks in multi-threaded environment
- **Concurrent Access:** Standard file system locking mechanisms apply
- **Error Isolation:** File operation errors don't affect other threads

This filesystem module provides secure, authenticated object storage capabilities for the RAIDA server while maintaining strict security controls and preventing unauthorized access to the underlying file system.