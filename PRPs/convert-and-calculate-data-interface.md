# Utility Functions Header (utils.h)

## Module Purpose
This header defines the interface for essential utility functions used throughout the RAIDA network system. It provides function declarations for cryptographic operations, data format conversions, checksums, random number generation, and various helper functions that support core system operations.

## Core Function Declarations

### 1. Data Integrity Functions

#### CRC32 Checksum Calculation (`crc32b`)
**Parameters:**
- Message buffer (unsigned byte array pointer)
- Message length (integer)

**Returns:** 32-bit unsigned integer checksum

**Purpose:** Calculates CRC32 checksum for data integrity verification using IEEE 802.3 polynomial

**Usage:** Data integrity verification, network protocol validation, challenge checksums
**Algorithm:** Standard CRC32 with polynomial 0xEDB88320
**Performance:** Optimized for byte-wise processing

### 2. Data Format Conversion Functions

#### Serial Number Extraction (`get_sn`)
**Parameters:**
- Buffer containing 4-byte big-endian integer

**Returns:** 32-bit unsigned integer in host byte order

**Purpose:** Extracts serial numbers from network protocol buffers

**Usage:** Network protocol processing, coin identification, data parsing
**Byte Order:** Converts from network (big-endian) to host byte order
**Compatibility:** Handles endianness differences across platforms

#### Generic 32-bit Integer Extraction (`get_u32`)
**Parameters:**
- Buffer containing 4-byte big-endian integer

**Returns:** 32-bit unsigned integer in host byte order

**Purpose:** General-purpose 32-bit integer extraction from network data

**Usage:** Protocol parsing, data extraction, network communication
**Implementation:** Identical to `get_sn` for consistency
**Endianness:** Handles big-endian to host byte order conversion

#### Serial Number Storage (`put_sn`)
**Parameters:**
- 32-bit unsigned integer value
- Buffer for 4-byte big-endian output

**Returns:** None

**Purpose:** Stores serial numbers in network protocol format

**Usage:** Network protocol generation, response preparation, data serialization
**Byte Order:** Converts from host to network (big-endian) byte order
**Protocol:** Ensures consistent network representation

#### Generic 32-bit Integer Storage (`put_u32`)
**Parameters:**
- 32-bit unsigned integer value
- Buffer for 4-byte big-endian output

**Returns:** None

**Purpose:** General-purpose 32-bit integer storage in network format

**Usage:** Protocol generation, data serialization, network communication
**Implementation:** Identical to `put_sn` for consistency
**Endianness:** Handles host to big-endian byte order conversion

### 3. Temporal Functions

#### Months From Start Calculation (`get_mfs`)
**Parameters:**
- None

**Returns:** 8-bit unsigned integer representing months from epoch

**Purpose:** Calculates months elapsed since system epoch (January 2023)

**Usage:** Coin timestamps, expiration tracking, temporal validation
**Epoch:** January 2023 as system start date
**Format:** Compact 8-bit timestamp representation
**Timezone:** Uses GMT for consistent time representation

### 4. Value Calculation Functions

#### Denomination Value Calculation (`get_den_value`)
**Parameters:**
- Denomination identifier (1 byte signed integer)

**Returns:** 64-bit unsigned integer representing denomination value

**Purpose:** Calculates numeric value of coin denomination using power-of-10 scaling

**Usage:** Value calculations, coin arithmetic, denomination comparisons
**Scaling:** Each denomination represents 10x the previous level
**Precision:** 64-bit precision for large values
**Range:** Supports fractional to whole number denominations

#### Coin Value Calculation (`coin_value`)
**Parameters:**
- Denomination identifier (1 byte signed integer)
- Serial number (4 bytes unsigned integer)

**Returns:** 64-bit unsigned integer representing precise coin value

**Purpose:** Calculates exact value of coin based on denomination

**Usage:** Transaction calculations, value validation, economic operations
**Precision:** Fixed denomination values for exact calculations
**Range:** Supports denominations from 0.00000001 to 1,000,000
**Accuracy:** Provides precise fractional value representation

### 5. Byte Order Conversion Functions

#### 64-bit Byte Order Swap (`swap_uint64`)
**Parameters:**
- 64-bit unsigned integer value

**Returns:** 64-bit unsigned integer with swapped byte order

**Purpose:** Converts 64-bit integers between different byte orders

**Usage:** Network protocol compatibility, cross-platform data exchange
**Implementation:** Handles 64-bit endianness conversion through 32-bit operations
**Compatibility:** Ensures consistent data representation across platforms

### 6. Data Conversion Functions

#### Hexadecimal String Conversion (`hex2bin`)
**Parameters:**
- Input hexadecimal string
- Output binary buffer
- Conversion length (integer)

**Returns:** None

**Purpose:** Converts hexadecimal string representation to binary data

**Usage:** Configuration parsing, key conversion, data transformation
**Format:** Processes two hex characters per output byte
**Validation:** Handles malformed hex strings gracefully
**Efficiency:** Direct character-to-byte conversion

### 7. Cryptographic Functions

#### Secure Random Number Generation (`generate_random_bytes`)
**Parameters:**
- Output buffer (unsigned byte array)
- Number of bytes to generate (integer)

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Generates cryptographically secure random bytes using system entropy

**Usage:** Nonce generation, authentication number creation, cryptographic operations
**Security:** Uses system entropy source (/dev/urandom)
**Quality:** Cryptographically secure and unpredictable
**Error Handling:** Proper error reporting and validation

#### Secure Authentication Number Generation (`generate_an_hash`)
**Parameters:**
- Input data buffer (unsigned byte array)
- Input data length (integer)
- Output authentication number buffer (16 bytes)

**Returns:** None

**Purpose:** Generates 16-byte authentication numbers using SHA-256 hash function

**Usage:** Coin authentication numbers, secure token generation, cryptographic identifiers
**Algorithm:** SHA-256 cryptographic hash function
**Output:** 16-byte authentication numbers (first 16 bytes of SHA-256 hash)
**Security:** Cryptographically secure and collision-resistant
**Deterministic:** Same input produces same authentication number

## Data Type Requirements

### 1. Standard Integer Types
**Required Types:**
- `uint8_t`: 8-bit unsigned integer for bytes and flags
- `uint32_t`: 32-bit unsigned integer for serial numbers and values
- `uint64_t`: 64-bit unsigned integer for large values and calculations
- `int8_t`: 8-bit signed integer for denominations

**Usage:** All utility functions use standard integer types for consistency
**Portability:** Ensures consistent behavior across platforms
**Precision:** Provides exact bit-width specifications

### 2. Buffer Types
**Required Types:**
- `unsigned char*`: Raw byte buffers for data processing
- `char*`: String buffers for hex conversion
- Array types for fixed-size buffers

**Usage:** Data conversion, cryptographic operations, network processing
**Safety:** Proper buffer management and bounds checking
**Flexibility:** Supports various data formats and sizes

## Integration Requirements

### 1. System Dependencies
**Required Headers:**
- Standard integer types (`stdint.h`)
- Standard library functions for basic operations
- Cryptographic libraries (OpenSSL for SHA-256)
- System entropy source access

**Platform Support:**
- POSIX-compatible systems for entropy source
- Cross-platform endianness handling
- Standard C library compatibility

### 2. Module Dependencies
**Used By:**
- All system modules requiring data conversion
- Network protocol processing modules
- Cryptographic and security modules
- Database and storage modules
- Command processing modules

**Provides For:**
- Consistent data format conversion
- Secure cryptographic operations
- Reliable integrity verification
- Efficient value calculations

### 3. Error Handling Requirements
**Error Codes:**
- Functions returning status codes use 0 for success, -1 for failure
- Proper error reporting for all failure conditions
- Graceful handling of invalid inputs
- Resource cleanup on error conditions

**Validation:**
- All input parameters validated before processing
- Buffer bounds checking for safety
- Null pointer validation where appropriate
- Range validation for numeric inputs

## Usage Patterns

### 1. Network Protocol Processing
```
// Convert network data to host format
uint32_t sn = get_sn(buffer);
uint32_t value = get_u32(buffer + 4);

// Convert host data to network format
put_sn(sn, output_buffer);
put_u32(value, output_buffer + 4);
```

### 2. Cryptographic Operations
```
// Generate secure random nonce
unsigned char nonce[12];
if (generate_random_bytes(nonce, 12) != 0) {
    // Handle error
}

// Generate authentication number
unsigned char input_data[64];
unsigned char auth_number[16];
generate_an_hash(input_data, 64, auth_number);
```

### 3. Data Integrity Verification
```
// Calculate and verify checksum
uint32_t calculated_crc = crc32b(data, length);
if (calculated_crc != expected_crc) {
    // Handle integrity failure
}
```

### 4. Value Calculations
```
// Calculate coin value
uint64_t value = coin_value(denomination, serial_number);
uint64_t den_value = get_den_value(denomination);
```

## Security Considerations

### 1. Cryptographic Security
- **Random Generation:** Uses cryptographically secure entropy source
- **Hash Functions:** SHA-256 provides collision resistance
- **Authentication Numbers:** 128-bit security level
- **Deterministic Generation:** Consistent results from same inputs

### 2. Input Validation
- **Parameter Checking:** All inputs validated before processing
- **Buffer Management:** Proper bounds checking and validation
- **Error Handling:** Secure error reporting without information leakage
- **Resource Management:** Proper cleanup on all code paths

### 3. Data Integrity
- **CRC32 Checksums:** Reliable error detection
- **Byte Order Consistency:** Prevents data corruption
- **Format Validation:** Ensures proper data format processing
- **Overflow Protection:** Handles large values safely

## Performance Characteristics

### 1. Efficiency
- **Optimized Algorithms:** Efficient implementations for common operations
- **Minimal Memory Usage:** Low memory footprint for all functions
- **Fast Conversions:** Optimized data format conversions
- **Cached Operations:** Avoids redundant calculations where possible

### 2. Scalability
- **Thread Safety:** All functions are thread-safe
- **Concurrent Access:** Supports high concurrency
- **Resource Efficiency:** Efficient resource utilization
- **Performance Monitoring:** Enables performance tracking

### 3. Reliability
- **Error Handling:** Comprehensive error detection and reporting
- **Input Validation:** Robust input validation and sanitization
- **Resource Management:** Proper resource allocation and cleanup
- **Consistent Behavior:** Predictable behavior across all conditions

This utility header provides the essential interface for foundational operations throughout the RAIDA network system, ensuring consistent, secure, and efficient data processing across all components.