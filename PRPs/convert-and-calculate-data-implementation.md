# Implementation Prompt: CloudCoin Utils Implementation

## 1. Module Purpose
This prompt defines the requirements for implementing the CloudCoin utilities module based on the provided implementation file. It establishes the implementation patterns, algorithms, external dependencies, and function behaviors required for implementing cryptographic, conversion, and mathematical utility functions.

## 2. System Dependencies

### 2.1 Required External Libraries
```
Standard I/O operations
Standard integer type definitions
String manipulation functions
Character classification functions
Time handling functions
Mathematical functions
Network byte order functions
File operations
Operating system API
Error handling definitions
```

### 2.2 Required Local Modules
```
Database interface (for denomination constants)
Utils interface declarations
Logging interface
```

### 2.3 Core Data Types
The following types are used throughout the implementation:
- **unsigned_8bit_integer**: 8-bit unsigned integer
- **unsigned_32bit_integer**: 32-bit unsigned integer
- **unsigned_64bit_integer**: 64-bit unsigned integer
- **signed_8bit_integer**: 8-bit signed integer
- **signed_size_type**: Signed size type for system operations
- **time_type**: Time type for time operations
- **time_structure**: Time structure for date/time breakdown

## 3. Algorithm Implementations

### 3.1 CRC32b Checksum Algorithm
```
CRC32b Implementation:
    Initial CRC: 4294967295 (0xFFFFFFFF)
    Polynomial: 3988292384 (0xEDB88320)
    For each byte in buffer:
        XOR byte with CRC
        For each bit (8 times):
            If least significant bit is 1: shift right and XOR with polynomial
            Else: shift right only
    Return bitwise complement of final CRC
```

### 3.2 Big-Endian Conversion Algorithm
```
Get 32-bit Big-Endian:
    result = (byte[0] << 24) | (byte[1] << 16) | (byte[2] << 8) | byte[3]

Put 32-bit Big-Endian:
    byte[0] = (value >> 24) & 255
    byte[1] = (value >> 16) & 255
    byte[2] = (value >> 8) & 255
    byte[3] = value & 255
```

### 3.3 Months From Start Calculation
```
MFS Calculation:
    Get current UTC time
    Calculate years since 2023: (current_year - base_year_offset) - 123
    Calculate months since February: current_month - 1
    Result: years * 12 + months
    Epoch: February 2023
```

### 3.4 Coin Value Calculation
```
Denomination Value Mapping:
    -8: 1
    -7: 10
    -6: 100
    -5: 1000
    -4: 10000
    -3: 100000
    -2: 1000000
    -1: 10000000
    0: 100000000
    1: 1000000000
    2: 10000000000
    3: 100000000000
    4: 1000000000000
    5: 10000000000000
    6: 100000000000000
```

## 4. Function Implementations - IMPLEMENTED IN THIS MODULE

### 4.1 Checksum Functions

#### 4.1.1 crc32b(message, len)
```
Function: crc32b
Parameters: byte_buffer message, integer len
Returns: unsigned_integer (CRC32b checksum)
Implementation: Polynomial-based CRC32b algorithm with standard polynomial
Purpose: Calculate CRC32b checksum for data integrity verification
```

### 4.2 Big-Endian Conversion Functions

#### 4.2.1 get_sn(buf) and get_u32(buf)
```
Function: get_sn / get_u32
Parameters: byte_buffer buf
Returns: unsigned_32bit_integer
Implementation: Extract 4 bytes in big-endian format using bit operations
Purpose: Convert network byte order to host byte order
```

#### 4.2.2 put_sn(val, buf) and put_u32(val, buf)
```
Function: put_sn / put_u32
Parameters: unsigned_32bit_integer val, byte_buffer buf
Returns: void
Implementation: Store 4 bytes in big-endian format using bit operations and masking
Purpose: Convert host byte order to network byte order
```

### 4.3 Time Functions

#### 4.3.1 get_mfs()
```
Function: get_mfs
Parameters: none
Returns: unsigned_8bit_integer
Implementation: UTC time calculation with February 2023 epoch
Purpose: Calculate months elapsed since CloudCoin epoch
Algorithm: (years_since_2023 * 12) + (months_since_february)
```

### 4.4 Mathematical Functions

#### 4.4.1 get_den_value(den)
```
Function: get_den_value
Parameters: signed_8bit_integer den
Returns: unsigned_64bit_integer
Implementation: Power of 10 calculation using mathematical power function
Formula: 10^(den + 8)
Purpose: Calculate numerical value for denomination
```

#### 4.4.2 coin_value(den, sn)
```
Function: coin_value
Parameters: signed_8bit_integer den, unsigned_32bit_integer sn
Returns: unsigned_64bit_integer
Implementation: Conditional logic with fixed denomination values
Purpose: Get coin value using predefined denomination mapping
Note: Serial number parameter present but not used in calculation
```

### 4.5 Conversion Functions

#### 4.5.1 hex2bin(input, res, len)
```
Function: hex2bin
Parameters: string input, byte_buffer res, integer len
Returns: void
Implementation: Format scanning with 2-character hexadecimal parsing
Purpose: Convert hexadecimal string to binary buffer
Algorithm: Process 2 characters at a time, advance position by 2
```

#### 4.5.2 swap_uint64(val)
```
Function: swap_uint64
Parameters: unsigned_64bit_integer val
Returns: unsigned_64bit_integer
Implementation: Network byte order conversion with bit manipulation for 64-bit endian swap
Purpose: Convert 64-bit value between little-endian and big-endian
```

### 4.6 Cryptographic Functions

#### 4.6.1 generate_random_bytes(buf, len)
```
Function: generate_random_bytes
Parameters: byte_buffer buf, integer len
Returns: integer (0 = success, -1 = failure)
Implementation: Read from system random device with proper error handling
Algorithm: 
    1. Open system random device for reading
    2. Read bytes in loop until requested bytes obtained
    3. Handle partial reads and errors
    4. Close system resource
    5. Return success/failure status
```

## 5. External Dependencies - CALLED FROM EXTERNAL SOURCES

### 5.1 Standard Library Functions
The following functions are called from external standard libraries:

#### 5.1.1 Time Functions
- **get_current_time()**: Get current system time
- **convert_to_utc()**: Convert time to UTC broken-down time

#### 5.1.2 Mathematical Functions
- **power_function(base, exponent)**: Power calculation

#### 5.1.3 String/Memory Functions
- **format_scan(string, format, ...)**: Formatted string scanning

#### 5.1.4 Network Functions
- **host_to_network_long(value)**: Host to network byte order conversion

#### 5.1.5 System Functions
- **open_file(path, mode)**: Open system resource
- **read_from_resource(handle, buffer, count)**: Read from system resource
- **close_resource(handle)**: Close system resource
- **get_error_description(error_code)**: Get error string description

#### 5.1.6 Logging Functions
- **log_error(format, ...)**: Error logging function
- **print_output(format, ...)**: Standard output formatting

### 5.2 External Constants
The following constants are used from external modules:
- **Denomination constants**: Denomination enumeration values
- **File access modes**: System file access constants
- **Error codes**: Global error number definitions

### 5.3 Platform Dependencies
- **System random device**: Access to cryptographically secure random source
- **Network byte order functions**: For endianness conversion
- **UTC time functions**: For portable time calculations

## 6. Error Handling Patterns

### 6.1 Random Generation Error Handling
```
Error Handling Pattern:
    1. Check system resource opening
    2. Handle partial reads in loop
    3. Verify total bytes read
    4. Log errors with descriptive messages
    5. Return -1 on any failure, 0 on success
```

### 6.2 Resource Management
```
Resource Management:
    1. Always close system resources
    2. Handle system operation failures gracefully
    3. Use proper error codes and logging
```

## 7. Performance Characteristics

### 7.1 Algorithm Complexity
- **CRC32b**: O(n) where n is buffer length
- **Big-endian conversion**: O(1) constant time
- **Hex conversion**: O(n) where n is output length
- **Time calculation**: O(1) constant time
- **Random generation**: O(n) where n is bytes requested

### 7.2 Memory Usage
- **Stack-based operations**: No dynamic memory allocation
- **Fixed buffer operations**: Caller-provided buffers
- **Minimal local variables**: Efficient memory usage

## 8. Security Considerations

### 8.1 Cryptographic Security
- **Secure random source**: Uses system cryptographic random device
- **Proper error handling**: Fails securely on random generation errors
- **Complete byte reading**: Ensures full entropy collection

### 8.2 Data Integrity
- **CRC32b verification**: Industry-standard checksum algorithm
- **Endianness handling**: Consistent network byte order usage

## 9. Platform Compatibility

### 9.1 System Compatibility
- **Unix-like systems**: Uses standard system APIs
- **Random device dependency**: Requires system-provided secure random source
- **Network functions**: Standard network API usage

### 9.2 Time Handling
- **UTC time usage**: Avoids timezone dependencies
- **Epoch definition**: February 2023 as CloudCoin epoch
- **Year calculation**: Handles time structure offset correctly

This implementation prompt provides complete guidance for implementing CloudCoin utility functions based on the provided source code. Developers should follow the exact algorithms and error handling patterns while adapting to their target platform's specific requirements.