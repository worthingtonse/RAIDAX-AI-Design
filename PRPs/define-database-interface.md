# Specification:  Database Header Interface

## 1. Module Purpose
This prompt defines the requirements for implementing the CloudCoin database layer. It establishes the data structures, constants, function signatures, and type definitions required for implementing a thread-safe, high-performance coin database system based on the provided header interface.

## 2. System Constants and Configuration

### 2.1 Core System Parameters
```
TOTAL_DENOMINATIONS = 15
TOTAL_PAGES = 1000
RECORDS_PER_PAGE = 1024
RECORD_SIZE = 17 bytes
DENOMINATION_OFFSET = 8
RESERVED_PAGE_RELEASE_SECONDS = 16
PAGES_IN_RAM = TOTAL_PAGES (all pages kept in memory)
```

### 2.2 Denomination Enumeration
The system must support exactly 15 denominations with the following mapping:

| Symbolic Name    | Integer Value | Decimal Representation |
|------------------|---------------|------------------------|
| DEN_0_00000001   | -8           | 0.00000001            |
| DEN_0_0000001    | -7           | 0.0000001             |
| DEN_0_000001     | -6           | 0.000001              |
| DEN_0_00001      | -5           | 0.00001               |
| DEN_0_0001       | -4           | 0.0001                |
| DEN_0_001        | -3           | 0.001                 |
| DEN_0_01         | -2           | 0.01                  |
| DEN_0_1          | -1           | 0.1                   |
| DEN_1            | 0            | 1                     |
| DEN_10           | 1            | 10                    |
| DEN_100          | 2            | 100                   |
| DEN_1000         | 3            | 1000                  |
| DEN_10000        | 4            | 10000                 |
| DEN_100000       | 5            | 100000                |
| DEN_1000000      | 6            | 1000000               |

### 2.3 Boundary Constants
```
MIN_DENOMINATION = -8 (DEN_0_00000001)
MAX_DENOMINATION = 6  (DEN_1000000)
```

## 3. Core Data Structures

### 3.1 Page Structure Definition
The `page_structure` represents a single page of coin data and must contain the following fields in this exact logical order:

```
page_structure:
    data: byte_array[RECORDS_PER_PAGE × 17]     // 17,408 bytes total
    reserved_at: timestamp_type                  // Platform-specific time type
    reserved_by: unsigned_32bit_integer         // Session ID (0 = not reserved)
    mutex: thread_mutex_type                    // Platform-specific mutex
    in_dirty_queue: integer                     // Flag: 0 = not queued, 1 = queued
    denomination: signed_8bit_integer           // Range: -8 to 6 (debugging field)
    page_number: unsigned_16bit_integer         // Range: 0 to 999 (debugging field)
```

### 3.2 Field Specifications

#### 3.2.1 Data Field
- **Size**: RECORDS_PER_PAGE × 17 bytes = 1024 × 17 = 17,408 bytes
- **Layout**: Each record is exactly 17 bytes (16-byte AN + 1-byte MFS)
- **Access**: Direct byte array indexing for performance

#### 3.2.2 Reservation Fields
- **reserved_at**: Platform-specific timestamp
  - Set to current time when page is reserved
  - Used for timeout calculations
- **reserved_by**: 32-bit session identifier
  - 0 indicates page is not reserved
  - Non-zero indicates active reservation

#### 3.2.3 Synchronization Fields
- **mutex**: Platform-specific thread synchronization primitive
  - Must support lock/unlock operations
  - Required for thread-safe page access
- **in_dirty_queue**: Integer flag for persistence queue management
  - 0 = page not in dirty queue
  - 1 = page is queued for persistence
  - Prevents duplicate queue entries

#### 3.2.4 Debug Fields
- **denomination**: Signed 8-bit integer storing the page's denomination
  - Range: -8 to 6
  - Used for debugging and validation
  - Can be removed if memory optimization is critical
- **page_number**: Unsigned 16-bit integer storing the page index
  - Range: 0 to 999
  - Used for debugging and validation
  - Can be removed if memory optimization is critical

## 4. Available Functions - DECLARED FOR EXTERNAL USE

### 4.1 Data Structure Access
The page_structure can be accessed through the provided function interface. No direct access to internal fields is permitted.

## 5. External Dependencies - CALLED FROM EXTERNAL SOURCES

### 5.1 Database Functions (Implemented Elsewhere)
The following functions are declared in the header and implemented in external database module:

#### 5.1.1 Initialization Functions
- **init_db()** → integer
  - Parameters: none
  - Purpose: Initialize the entire database system

- **init_page(seed, denomination, page_number)** → integer
  - Parameters: integer seed, signed_8bit_integer denomination (range: -8 to 6), integer page_number (range: 0 to 999)
  - Purpose: Initialize single page file if it doesn't exist

- **load_pages()** → integer
  - Parameters: none
  - Purpose: Load all page files from disk into memory

#### 5.1.2 Page Access Functions
- **get_page(denomination, page_number)** → page_pointer
  - Parameters: signed_8bit_integer denomination (range: -8 to 6), integer page_number (range: 0 to 999)
  - Purpose: Get page reference without locking

- **get_page_lock(denomination, page_number)** → page_pointer
  - Parameters: signed_8bit_integer denomination (range: -8 to 6), integer page_number (range: 0 to 999)
  - Purpose: Get page reference and acquire its mutex

- **unlock_page(page_pointer)** → void
  - Parameters: pointer to page_structure
  - Purpose: Release the page mutex

- **get_page_by_sn_lock(denomination, serial_number)** → page_pointer
  - Parameters: signed_8bit_integer denomination (range: -8 to 6), unsigned_32bit_integer serial_number
  - Purpose: Get and lock page containing specified serial number

#### 5.1.3 Page Reservation Functions
- **page_is_reserved(page_pointer)** → integer
  - Parameters: pointer to page_structure
  - Purpose: Check if page is currently reserved, auto-release if expired

- **reserve_page(page_pointer, session_id)** → void
  - Parameters: pointer to page_structure, unsigned_32bit_integer session_id
  - Purpose: Reserve page for exclusive access by session

- **release_reserved_page(page_pointer)** → void
  - Parameters: pointer to page_structure
  - Purpose: Manually release page reservation

#### 5.1.4 Persistence Functions
- **add_page_to_dirty_queue(page_pointer)** → void
  - Parameters: pointer to page_structure
  - Purpose: Queue modified page for background persistence

- **sync_pages_thread(thread_argument)** → thread_return_type
  - Parameters: platform-specific thread parameter
  - Purpose: Background thread function for persistence operations

- **sync_page(page_pointer)** → void
  - Parameters: pointer to page_structure
  - Purpose: Write single page to disk immediately

#### 5.1.5 Utility Functions
- **get_den_idx(denomination)** → integer
  - Parameters: signed_8bit_integer denomination (range: -8 to 6)
  - Purpose: Convert denomination value to array index

- **get_den_by_idx(index)** → signed_8bit_integer
  - Parameters: integer index (range: 0 to 14)
  - Purpose: Convert array index to denomination value

### 5.2 Platform Thread Primitives
The following are called from external thread libraries:
- pthread_mutex_t operations (lock/unlock/init/destroy)
- Thread creation and management functions
- Platform-specific threading primitives

### 5.3 Platform Standard Types
The following are included from external headers:
- int8_t, uint16_t, uint32_t (from stdint.h or equivalent)
- time_t (from time.h or equivalent)
- pthread_mutex_t (from pthread.h or equivalent)

### 5.4 Platform File I/O
The following are called from external file system libraries:
- File creation and writing operations
- Directory creation operations  
- File reading operations

### 5.5 Platform Memory Operations
The following are called from external memory management:
- Memory allocation for queue nodes
- Memory alignment operations

## 6. Data Type Requirements

### 6.1 Platform-Specific Types
Implementations must map these logical types to appropriate platform types:

- **signed_8bit_integer**: 8-bit signed integer (-128 to 127)
- **unsigned_16bit_integer**: 16-bit unsigned integer (0 to 65535)
- **unsigned_32bit_integer**: 32-bit unsigned integer (0 to 4294967295)
- **timestamp_type**: Platform timestamp type
- **thread_mutex_type**: Platform mutex type
- **byte_array**: Byte storage array type

### 6.2 Memory Layout Requirements
- **Alignment**: Page data should be aligned for optimal memory access
- **Packing**: No padding between data records within a page
- **Endianness**: Implementation-defined (must be consistent across system)

## 7. Thread Safety Requirements

### 7.1 Locking Protocol
- **Page-Level Locking**: Each page has its own mutex for fine-grained concurrency
- **Lock Ordering**: Always acquire page mutex before checking/setting in_dirty_queue
- **No Global Locks**: Avoid system-wide locking for scalability

### 7.2 Thread-Safe Functions
All public functions must be thread-safe with the following exceptions:
- Functions returning page pointers without locking require caller synchronization
- Page modification requires holding the page mutex

### 7.3 Synchronization Primitives Required
- **Mutex**: For page-level locking
- **Condition Variable**: For dirty queue signaling
- **Thread Creation**: For background persistence thread

## 8. Error Handling Interface

### 8.1 Return Code Convention
- **Success**: Return 0
- **General Error**: Return negative integer
- **Invalid Parameters**: Return -1
- **System Error**: Return platform-specific error code
- **Memory Error**: Return memory allocation error equivalent

### 8.2 Null Pointer Handling
- Functions returning pointers return null on error
- Caller must check for null before dereferencing
- Null parameters to functions should be handled gracefully

## 9. Memory Management Contract

### 9.1 Static Allocation
- All page objects are statically allocated at startup
- No dynamic allocation/deallocation during normal operation
- Global page array: `page_structure[TOTAL_DENOMINATIONS][TOTAL_PAGES]`

### 9.2 Resource Lifecycle
- **Initialization**: All resources allocated during init_db()
- **Runtime**: No allocation/deallocation except for dirty queue nodes
- **Cleanup**: Platform-specific cleanup (not defined in this interface)

### 9.3 Memory Footprint
- **Total Page Data**: 15 denominations × 1000 pages × 17,408 bytes ≈ 256 MB
- **Metadata Overhead**: Minimal (mutexes, flags, debug fields)
- **Queue Overhead**: Dynamic based on modification rate

## 10. Performance Characteristics

### 10.1 Access Patterns
- **Read Operations**: O(1) direct array access
- **Write Operations**: O(1) modification + O(1) queue insertion
- **Lock Contention**: Per-page granularity minimizes blocking

### 10.2 Scalability Limits
- **Maximum Pages**: 1000 per denomination (fixed)
- **Maximum Denominations**: 15 (fixed)
- **Concurrent Access**: Limited by page-level lock contention

## 11. Integration Requirements

### 11.1 Required Dependencies
Implementations must provide or include:
- Thread synchronization primitives (mutex, condition variables)
- Standard integer types
- Time handling functions

### 11.2 Platform Considerations
- **Thread Safety**: Platform must support thread-safe code generation
- **Atomic Operations**: Platform should support atomic integer operations
- **Memory Barriers**: Implementation should consider memory ordering

## 12. Validation and Testing Interface

### 12.1 Parameter Validation
All functions must validate:
- Denomination range (-8 to 6)
- Page number range (0 to 999)
- Non-null pointer parameters where required

### 12.2 Debug Support
- Debug fields in page structure support runtime validation
- Error logging should include context information
- Performance metrics should be collectible

This implementation prompt provides complete interface definition for implementing a CloudCoin database system. Developers should implement all specified functions while adapting data types and synchronization primitives to their target platform.