#  Specification: Filesystem Commands Implementation

## 1. Module Purpose
This module implements secure filesystem operations for the CloudCoin RAIDA system. It provides authenticated file management capabilities with robust path traversal protection, enabling secure storage and retrieval of objects within a sandboxed directory structure.

## 2. System Architecture Overview

### 2.1 Core Components
- **Authenticated File Operations**: Admin key-based access control for all operations
- **Path Traversal Protection**: Real path resolution and validation for security
- **Sandboxed Environment**: All operations restricted to designated Folders directory
- **Binary File Support**: Complete file content handling for arbitrary data types
- **Cryptographic Key Management**: Specialized key file retrieval functionality

### 2.2 Security Model
- **Admin Authentication**: 16-byte admin key required for all operations
- **Directory Sandboxing**: All file paths restricted to base Folders directory
- **Path Validation**: Real path resolution prevents traversal attacks
- **File Type Validation**: Regular file verification for all operations

## 3. System Constants and Configuration

### 3.1 Security Constants
```
PATH_MAX = maximum path length (platform-specific, typically 4096)
ADMIN_KEY_SIZE = 16 bytes
BASE_DIRECTORY = "{config.cwd}/Folders/"
FILE_PERMISSIONS = 0644 (rw-r--r--)
```

### 3.2 Request Format Constants
```
GET_OBJECT_MIN_SIZE = 35 bytes      // 16 admin + 16 challenge + 1 path + 2 EOF
PUT_OBJECT_MIN_SIZE = 40 bytes      // 16 admin + 16 challenge + 4 size + 1 filename + 1 content + 2 EOF
RM_OBJECT_MIN_SIZE = 35 bytes       // 16 admin + 16 challenge + 1 path + 2 EOF
```

### 3.3 Status Response Codes
```
STATUS_SUCCESS = successful operation
ERROR_INVALID_PACKET_LENGTH = malformed request size
ERROR_ADMIN_AUTH = authentication failure or security violation
ERROR_INVALID_PARAMETER = invalid path or parameter
ERROR_FILE_NOT_EXIST = requested file not found
ERROR_FILE_EXISTS = file already exists (for creation operations)
ERROR_FILESYSTEM = file I/O operation failure
ERROR_MEMORY_ALLOC = memory allocation failure
```

## 4. Core Command Implementations

### 4.1. cmd_get_object
**Purpose**: Retrieves file content from the sandboxed filesystem.

**Request Format**:
```
[16 bytes Challenge Header]
[16 bytes Admin Key]
[Variable length File Path]
[2 bytes EOF trailer]
Minimum size: 35 bytes
```

**Security Validation**:
1. **Request Size Check**: Verify minimum 35 bytes
2. **Admin Authentication**: Compare admin key with configured value
3. **Path Length Validation**: Ensure path length < PATH_MAX
4. **Path Extraction**: Calculate path length as (body_size - 16 - 18)

**Path Security Processing**:
1. **Base Path Construction**: Combine config.cwd + "/Folders/" + user_path
2. **Real Path Resolution**: Use realpath() to resolve absolute path
3. **Sandbox Validation**: Verify resolved path starts with base directory
4. **Traversal Detection**: Reject paths outside sandbox with ERROR_ADMIN_AUTH

**File Operations**:
1. **File Existence**: Use stat() to verify file exists and is regular file
2. **Size Determination**: Extract file size from stat structure
3. **Memory Allocation**: Allocate output buffer of exact file size
4. **File Reading**: Open file read-only and read entire content
5. **Response Preparation**: Set output buffer and STATUS_SUCCESS

**Error Handling**:
- Path resolution failure: ERROR_FILE_NOT_EXIST
- Non-regular file: ERROR_INVALID_PARAMETER
- File I/O errors: ERROR_FILESYSTEM
- Memory allocation failure: ERROR_MEMORY_ALLOC

### 4.2. cmd_put_object
**Purpose**: Stores file content in the sandboxed filesystem.

**Request Format**:
```
[16 bytes Challenge Header]
[16 bytes Admin Key]
[4 bytes File Size (big-endian)]
[Variable length Filename (null-terminated)]
[Variable length File Content]
[2 bytes EOF trailer]
Minimum size: 40 bytes
```

**Security Validation**:
1. **Request Size Check**: Verify minimum 40 bytes
2. **Admin Authentication**: Compare admin key with configured value
3. **Size Extraction**: Get file size from bytes 16-19 using get_u32()
4. **Filename Validation**: Extract null-terminated filename, validate length

**Body Size Verification**:
- **Expected Size**: 16 + 4 + filename_length + 1 + file_size + 2
- **Actual Size**: Compare with ci->body_size
- **Mismatch Handling**: Return ERROR_INVALID_PACKET_LENGTH

**Path Security Processing**:
1. **Directory Resolution**: Resolve directory portion of target path
2. **Directory Validation**: Ensure target directory exists and is within sandbox
3. **Path Construction**: Build full target path within validated directory
4. **Traversal Prevention**: Reject attempts to write outside sandbox

**File Operations**:
1. **File Creation**: Open with O_WRONLY | O_CREAT | O_TRUNC flags
2. **Permission Setting**: Set file permissions to 0644
3. **Content Writing**: Write exact file_size bytes from payload
4. **Write Verification**: Ensure all bytes written successfully
5. **File Closure**: Close file descriptor and verify success

**Error Handling**:
- Directory doesn't exist: ERROR_FILE_NOT_EXIST
- Path traversal attempt: ERROR_ADMIN_AUTH
- File creation failure: ERROR_FILESYSTEM
- Write operation failure: ERROR_FILESYSTEM

### 4.3. cmd_rm_object
**Purpose**: Removes file from the sandboxed filesystem.

**Request Format**:
```
[16 bytes Challenge Header]
[16 bytes Admin Key]
[Variable length File Path]
[2 bytes EOF trailer]
Minimum size: 35 bytes
```

**Security Validation**:
1. **Request Size Check**: Verify minimum 35 bytes
2. **Admin Authentication**: Compare admin key with configured value
3. **Path Length Validation**: Ensure path length < PATH_MAX
4. **Path Extraction**: Calculate path length as (body_size - 16 - 18)

**Path Security Processing**:
1. **Base Path Construction**: Combine config.cwd + "/Folders/" + user_path
2. **Real Path Resolution**: Use realpath() to resolve absolute path
3. **File Existence**: Verify file exists before attempting removal
4. **Sandbox Validation**: Ensure resolved path within base directory

**File Operations**:
1. **Removal Operation**: Use remove() system call to delete file
2. **Operation Verification**: Check return value for success
3. **Status Setting**: Set STATUS_SUCCESS on successful removal

**Error Handling**:
- File doesn't exist: ERROR_FILE_NOT_EXIST
- Path traversal attempt: ERROR_ADMIN_AUTH
- Removal failure: ERROR_FILESYSTEM

## 5. Utility Functions

### 5.1. get_crypto_key(ticker, size_output)
**Purpose**: Retrieves cryptographic key content from filesystem for internal use.

**Parameters**:
- ticker: String identifier for the key file
- size_output: Pointer to integer for returning file size

**Processing Logic**:
1. **Path Construction**: Build path as "{config.cwd}/Folders/{ticker}"
2. **File Validation**: Check file existence and regular file status
3. **Size Determination**: Use stat() to get file size
4. **Memory Allocation**: Allocate buffer for entire file content
5. **File Reading**: Read complete file content into buffer
6. **Size Return**: Set size_output to actual bytes read

**Return Behavior**:
- **Success**: Return pointer to allocated buffer with key content
- **Failure**: Return NULL for any error condition
- **Caller Responsibility**: Free returned buffer when finished

**Error Conditions**:
- File access failure: Return NULL
- File stat failure: Return NULL
- Non-regular file: Return NULL
- Memory allocation failure: Return NULL
- File read failure: Return NULL and free buffer

## 6. Security Implementation Requirements

### 6.1 Path Traversal Prevention
- **Real Path Resolution**: Use realpath() to resolve symbolic links and relative paths
- **Absolute Path Validation**: Compare resolved paths with base directory
- **String Prefix Checking**: Use strncmp() to verify paths start with base directory
- **Rejection Response**: Use ERROR_ADMIN_AUTH to avoid revealing path structure

### 6.2 Admin Authentication
- **Key Comparison**: Use memcmp() for constant-time comparison
- **Key Storage**: Admin key stored in configuration structure
- **Authentication Failure**: Return ERROR_ADMIN_AUTH immediately
- **Key Validation**: Required for every filesystem operation

### 6.3 Sandbox Enforcement
- **Base Directory**: All operations limited to "{config.cwd}/Folders/"
- **Directory Validation**: Ensure all resolved paths within sandbox
- **Access Control**: No operations permitted outside base directory
- **Error Responses**: Generic errors to prevent information disclosure

## 7. File Operation Safety

### 7.1 Memory Management
- **Buffer Allocation**: Allocate exact file size for content
- **Error Cleanup**: Free allocated buffers on all error paths
- **Size Verification**: Verify read/write operations match expected sizes
- **Resource Management**: Close file descriptors on all exit paths

### 7.2 File System Interaction
- **File Type Validation**: Use S_ISREG() to verify regular files only
- **Permission Setting**: Set appropriate file permissions (0644)
- **Atomic Operations**: Use appropriate flags for file creation/truncation
- **Error Propagation**: Convert system errors to appropriate status codes

### 7.3 Input Validation
- **Size Limits**: Enforce PATH_MAX limits on all path inputs
- **Null Termination**: Ensure proper string termination for paths
- **Length Calculation**: Verify calculated lengths match actual data
- **Buffer Bounds**: Prevent buffer overflows in all operations

## 8. Error Handling and Logging

### 8.1 Error Classification
- **Authentication Errors**: Admin key validation failures
- **Security Errors**: Path traversal attempts and sandbox violations
- **Filesystem Errors**: File I/O operation failures
- **Parameter Errors**: Invalid request format or parameters

### 8.2 Logging Strategy
- **Security Events**: Log all authentication failures and traversal attempts
- **Operation Success**: Debug log successful file operations with paths
- **Error Context**: Include system error messages with file operations
- **Path Information**: Log resolved paths for debugging (after validation)

### 8.3 Error Response Strategy
- **Generic Errors**: Use ERROR_ADMIN_AUTH for security violations
- **Specific Errors**: Use appropriate codes for legitimate failures
- **Information Hiding**: Avoid revealing internal path structure
- **Consistent Responses**: Same error code for similar security violations

## 9. Performance Considerations

### 9.1 File I/O Optimization
- **Single Read/Write**: Complete file operations in single system calls
- **Buffer Sizing**: Allocate exact file size to minimize memory usage
- **File Descriptor Management**: Minimize open file descriptor lifetime
- **Path Caching**: Reuse resolved base paths where possible

### 9.2 Memory Efficiency
- **Dynamic Allocation**: Allocate only required memory for file content
- **Early Cleanup**: Free resources immediately after use
- **Error Path Cleanup**: Ensure no memory leaks on error conditions
- **Size Validation**: Prevent excessive memory allocation attempts

### 9.3 Security Performance
- **Path Resolution**: Use efficient realpath() for security validation
- **String Operations**: Use optimized string comparison functions
- **Authentication**: Constant-time comparison for admin keys
- **Validation Order**: Perform cheap validations before expensive operations

## 10. Integration Requirements

### 10.1 Configuration Dependencies
- **Admin Key**: Access to config.admin_key for authentication
- **Base Directory**: Access to config.cwd for path construction
- **Path Limits**: Platform-specific PATH_MAX constant

### 10.2 System Dependencies
- **File System**: POSIX file system operations (open, read, write, remove)
- **Path Resolution**: realpath() function for security validation
- **Memory Management**: malloc/free for dynamic buffer allocation
- **String Operations**: Standard string manipulation functions

### 10.3 Protocol Integration
- **Connection Info**: Access to body_size, output, output_size, command_status
- **Payload Access**: get_body_payload() function for request data
- **Status Codes**: Protocol-defined error and success constants
- **Response Format**: Standard response preparation mechanism

This specification provides complete implementation guidance for secure filesystem operations while remaining language-agnostic and accurately reflecting the security-focused implementation requirements.