# Implementation Prompt: CloudCoin Utils Interface (utisl.h)

## 1. Module Purpose
This prompt defines the requirements for implementing the CloudCoin utilities interface layer. It establishes the utility function signatures and external dependencies required for implementing cryptographic, conversion, and mathematical utility functions based on the provided header interface.

## 2. Module Dependencies and Integration

### 2.1 This Module's Position in Architecture
- **Layer**: Foundation Layer
- **Build Order**: Build this 4th (after database implementation)
- **Depends On**: database_interface for denomination constants
- **Used By**: protocol_implementation, commands_module, network_module, crypto_module

### 2.2 Required Imports (Build Dependencies)
```
MUST import before building this module:
- standard_integers (for type definitions)
- database_interface (for denomination constants used in coin_value function)

WILL BE imported by these modules:
- protocol_implementation (uses CRC, endian conversion, time functions, secure random)
- commands_module (uses all utility functions for processing)
- network_module (uses CRC, endian conversion for network data)
- crypto_module (uses secure random generation)
```

### 2.3 Interface Contracts
```
Functions this module PROVIDES to others:
- crc32b() - used by network and database modules for integrity
- get_u32()/put_u32() - used by protocol parsers and network handlers
- get_mfs() - used by coin creation and temporal validation
- coin_value() - used by user interfaces and financial calculations
- generate_random_bytes() - used by crypto and security modules

Functions this module REQUIRES from others:
- None (this is a foundation module with no internal dependencies)
```

### 2.4 External Modules to Import
```
standard_integers - Provides standard integer type definitions (uint8_t, uint32_t, etc.)
```

### 2.5 Core Data Types
The following types are used throughout the interface:
- **uint8_t**: 8-bit unsigned integer
- **uint32_t**: 32-bit unsigned integer
- **uint64_t**: 64-bit unsigned integer
- **int8_t**: 8-bit signed integer
- **unsigned char**: Unsigned character type for byte operations
- **char**: Character type for string operations
- **int**: Standard integer type

## 3. Available Function Interface

### 3.1 Function Categories Overview
This module provides utility functions organized into functional categories for CloudCoin operations. These functions handle data integrity, format conversion, time calculations, and cryptographic operations essential for the CloudCoin system.

## 4. External Function Calls - TO BE CALLED FROM OTHER MODULES

### 4.1 Checksum and Integrity Functions

#### 4.1.1 crc32b(buffer, length)
```
Function: crc32b
Purpose: Calculate CRC32b checksum for data integrity verification
When to call: Before transmitting data or storing sensitive information to detect corruption
Context: Used by network protocol handlers and data persistence modules
Parameters: pointer to unsigned_char buffer, integer length
Returns: unsigned_integer (CRC32b checksum)
Defined in: utils module implementation
```

### 4.2 Data Format Conversion Functions

#### 4.2.1 get_sn(buffer) / get_u32(buffer)
```
Function: get_sn / get_u32
Purpose: Extract 32-bit values from network byte order buffers
When to call: When parsing received network data or reading stored binary data
Context: Used by protocol parsers and database readers to extract serial numbers and values
Parameters: pointer to unsigned_char buffer
Returns: unsigned_32bit_integer
Defined in: utils module implementation
```

#### 4.2.2 put_sn(value, buffer) / put_u32(value, buffer)
```
Function: put_sn / put_u32
Purpose: Store 32-bit values in network byte order format
When to call: When preparing data for network transmission or binary storage
Context: Used by protocol writers and database writers to format serial numbers and values
Parameters: unsigned_32bit_integer value, pointer to unsigned_char buffer
Returns: void
Defined in: utils module implementation
```

#### 4.2.3 hex2bin(hex_string, binary_buffer, length)
```
Function: hex2bin
Purpose: Convert hexadecimal string representation to binary data
When to call: When processing user input or configuration data in hex format
Context: Used by configuration parsers and user input handlers
Parameters: pointer to char hex_string, pointer to char binary_buffer, integer length
Returns: void
Defined in: utils module implementation
```

#### 4.2.4 swap_uint64(value)
```
Function: swap_uint64
Purpose: Convert 64-bit values between little-endian and big-endian byte order
When to call: When handling cross-platform data exchange or specific protocol requirements
Context: Used by network handlers dealing with different endianness requirements
Parameters: unsigned_64bit_integer value
Returns: unsigned_64bit_integer
Defined in: utils module implementation
```

### 4.3 Time and Date Functions

#### 4.3.1 get_mfs()
```
Function: get_mfs
Purpose: Calculate "Months From Start" value based on CloudCoin epoch (February 2023)
When to call: When creating new coins or validating time-based operations
Context: Used by coin creation modules and temporal validation systems
Parameters: none
Returns: unsigned_8bit_integer
Defined in: utils module implementation
```

### 4.4 Coin Value Calculation Functions

#### 4.4.1 get_den_value(denomination)
```
Function: get_den_value
Purpose: Calculate numerical value for a given denomination using power-of-10 formula
When to call: When performing mathematical operations on coin values
Context: Used by financial calculation modules and denomination converters
Parameters: signed_8bit_integer denomination
Returns: unsigned_64bit_integer
Defined in: utils module implementation
```

#### 4.4.2 coin_value(denomination, serial_number)
```
Function: coin_value
Purpose: Get coin value using predefined denomination mapping for display or calculations
When to call: When displaying coin values to users or performing value aggregations
Context: Used by user interface modules and reporting systems
Parameters: signed_8bit_integer denomination, unsigned_32bit_integer serial_number
Returns: unsigned_64bit_integer
Defined in: utils module implementation
```

### 4.5 Cryptographic Security Functions

#### 4.5.1 generate_random_bytes(buffer, length)
```
Function: generate_random_bytes
Purpose: Generate cryptographically secure random bytes for security operations
When to call: When creating cryptographic keys, nonces, or any security-critical random data
Context: Used by key generation modules, authentication systems, and security protocols
Parameters: pointer to unsigned_char buffer, integer length
Returns: integer (0 = success, negative = error)
Defined in: utils module implementation
```

## 5. Integration Guidelines

### 5.1 Module Import Requirements
When implementing modules that use these utilities:
```
Import Required:
- standard_integers (for type definitions)
- utils_interface (this module)

Optional Imports Based on Usage:
- logging_module (if error handling is needed)
- time_module (if time operations are used)
- crypto_module (if security functions are used)
```

### 5.2 Error Handling Integration
```
Functions with Error Returns:
- generate_random_bytes: Check return value, 0 = success, negative = failure
- Always validate buffer parameters before calling functions
- Handle potential null pointer scenarios gracefully
```

### 5.3 Performance Considerations
```
High-Frequency Operations:
- get_sn/put_sn: O(1) operations, safe for frequent use
- crc32b: O(n) operation, consider caching for large repeated buffers
- get_mfs: O(1) but involves system time call, cache if called frequently

Security-Critical Operations:
- generate_random_bytes: May block on entropy, avoid in tight loops
- Always check return values for cryptographic functions
```

## 6. Data Type Requirements

### 6.1 Standard Integer Types
- **int8_t**: 8-bit signed integer (-128 to 127)
- **uint8_t**: 8-bit unsigned integer (0 to 255)
- **uint32_t**: 32-bit unsigned integer (0 to 4294967295)
- **uint64_t**: 64-bit unsigned integer (0 to 18446744073709551615)
- **int**: Standard signed integer type
- **unsigned int**: Standard unsigned integer type

### 6.2 Buffer and Pointer Types
- **char**: Character type for string operations
- **unsigned char**: Unsigned character type for byte buffer operations
- **pointer types**: Used for buffer and string parameter passing

## 7. Usage Patterns and Best Practices

### 7.1 Network Data Handling
```
Typical Usage Pattern:
1. Receive network data into buffer
2. Call get_u32(buffer) to extract values
3. Process extracted values
4. Call put_u32(value, output_buffer) to prepare response
5. Send response buffer
```

### 7.2 Coin Operations
```
Typical Usage Pattern:
1. Call get_mfs() to get current time reference
2. Call coin_value(denomination, sn) to get coin worth
3. Use crc32b() to verify coin data integrity
4. Call generate_random_bytes() for any security operations
```

### 7.3 Data Conversion
```
Typical Usage Pattern:
1. Receive hex string input
2. Call hex2bin() to convert to binary
3. Process binary data
4. Use endian conversion if cross-platform compatibility needed
```

This implementation prompt provides complete interface definition for integrating CloudCoin utility functions. Developers should understand the context and purpose of each function call to implement effective CloudCoin applications.