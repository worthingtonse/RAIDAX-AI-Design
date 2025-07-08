
# Specification:  Database Header Interface

## 1. Module Purpose
This specification defines the public interface for the CloudCoin database layer. It establishes the data structures, constants, function signatures, and type definitions required for implementing a thread-safe, high-performance coin database system.

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
The `page_object` structure represents a single page of coin data and must contain the following fields in this exact logical order:

```
page_object:
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
- **reserved_at**: Platform-specific timestamp (time_t equivalent)
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

## 4. Public Function Interface

### 4.1 Initialization Functions

#### 4.1.1 init_db()
```
Function: init_db
Parameters: none
Returns: integer (0 = success, negative = error)
Purpose: Initialize the entire database system
```
- Creates directory structure
- Initializes all pages
- Loads pages into memory
- Starts background threads

#### 4.1.2 init_page(seed, denomination, page_number)
```
Function: init_page
Parameters: 
    seed: integer (random seed for AN generation)
    denomination: signed_8bit_integer (range: -8 to 6)
    page_number: integer (range: 0 to 999)
Returns: integer (0 = success, negative = error)
Purpose: Initialize a single page file if it doesn't exist
```

#### 4.1.3 load_pages()
```
Function: load_pages
Parameters: none
Returns: integer (0 = success, negative = error)
Purpose: Load all page files from disk into memory
```

### 4.2 Page Access Functions

#### 4.2.1 get_page(denomination, page_number)
```
Function: get_page
Parameters:
    denomination: signed_8bit_integer (range: -8 to 6)
    page_number: integer (range: 0 to 999)
Returns: pointer to page_object (NULL if invalid)
Purpose: Get page reference without locking
```

#### 4.2.2 get_page_lock(denomination, page_number)
```
Function: get_page_lock
Parameters:
    denomination: signed_8bit_integer (range: -8 to 6)
    page_number: integer (range: 0 to 999)
Returns: pointer to page_object (NULL if invalid)
Purpose: Get page reference and acquire its mutex
```

#### 4.2.3 unlock_page(page_pointer)
```
Function: unlock_page
Parameters:
    page_pointer: pointer to page_object
Returns: void
Purpose: Release the page mutex
```

#### 4.2.4 get_page_by_sn_lock(denomination, serial_number)
```
Function: get_page_by_sn_lock
Parameters:
    denomination: signed_8bit_integer (range: -8 to 6)
    serial_number: unsigned_32bit_integer
Returns: pointer to page_object (NULL if invalid)
Purpose: Get and lock page containing the specified serial number
Implementation: page_number = serial_number / RECORDS_PER_PAGE
```

### 4.3 Page Reservation Functions

#### 4.3.1 page_is_reserved(page_pointer)
```
Function: page_is_reserved
Parameters:
    page_pointer: pointer to page_object
Returns: integer (0 = not reserved, 1 = reserved)
Purpose: Check if page is currently reserved, auto-release if expired
```

#### 4.3.2 reserve_page(page_pointer, session_id)
```
Function: reserve_page
Parameters:
    page_pointer: pointer to page_object
    session_id: unsigned_32bit_integer
Returns: void
Purpose: Reserve page for exclusive access by session
```

#### 4.3.3 release_reserved_page(page_pointer)
```
Function: release_reserved_page
Parameters:
    page_pointer: pointer to page_object
Returns: void
Purpose: Manually release page reservation
```

### 4.4 Persistence Functions

#### 4.4.1 add_page_to_dirty_queue(page_pointer)
```
Function: add_page_to_dirty_queue
Parameters:
    page_pointer: pointer to page_object
Returns: void
Purpose: Queue modified page for background persistence
Prerequisites: Page mutex must be held by caller
```

#### 4.4.2 sync_pages_thread(thread_argument)
```
Function: sync_pages_thread
Parameters:
    thread_argument: platform-specific thread parameter
Returns: platform-specific thread return type
Purpose: Background thread function for persistence operations
```

#### 4.4.3 sync_page(page_pointer)
```
Function: sync_page
Parameters:
    page_pointer: pointer to page_object
Returns: void
Purpose: Write single page to disk immediately
```

### 4.5 Utility Functions

#### 4.5.1 get_den_idx(denomination)
```
Function: get_den_idx
Parameters:
    denomination: signed_8bit_integer (range: -8 to 6)
Returns: integer (array index: 0 to 14)
Purpose: Convert denomination value to array index
Implementation: return denomination + DENOMINATION_OFFSET
```

#### 4.5.2 get_den_by_idx(index)
```
Function: get_den_by_idx
Parameters:
    index: integer (range: 0 to 14)
Returns: signed_8bit_integer (denomination: -8 to 6)
Purpose: Convert array index to denomination value
Implementation: return index - DENOMINATION_OFFSET
```

## 5. Data Type Requirements

### 5.1 Platform-Specific Types
Implementations must map these logical types to appropriate platform types:

- **signed_8bit_integer**: int8_t or equivalent (-128 to 127)
- **unsigned_16bit_integer**: uint16_t or equivalent (0 to 65535)
- **unsigned_32bit_integer**: uint32_t or equivalent (0 to 4294967295)
- **timestamp_type**: time_t or equivalent platform timestamp
- **thread_mutex_type**: pthread_mutex_t or equivalent platform mutex
- **byte_array**: unsigned char array or equivalent byte storage

### 5.2 Memory Layout Requirements
- **Alignment**: Page data should be aligned for optimal memory access
- **Packing**: No padding between data records within a page
- **Endianness**: Implementation-defined (must be consistent across system)

## 6. Thread Safety Requirements

### 6.1 Locking Protocol
- **Page-Level Locking**: Each page has its own mutex for fine-grained concurrency
- **Lock Ordering**: Always acquire page mutex before checking/setting in_dirty_queue
- **No Global Locks**: Avoid system-wide locking for scalability

### 6.2 Thread-Safe Functions
All public functions must be thread-safe with the following exceptions:
- Functions returning page pointers without locking require caller synchronization
- Page modification requires holding the page mutex

### 6.3 Synchronization Primitives Required
- **Mutex**: For page-level locking
- **Condition Variable**: For dirty queue signaling
- **Thread Creation**: For background persistence thread

## 7. Error Handling Interface

### 7.1 Return Code Convention
- **Success**: Return 0
- **General Error**: Return negative integer
- **Invalid Parameters**: Return -1
- **System Error**: Return platform-specific error code
- **Memory Error**: Return -ENOMEM equivalent

### 7.2 Null Pointer Handling
- Functions returning pointers return NULL on error
- Caller must check for NULL before dereferencing
- NULL parameters to functions should be handled gracefully

## 8. Memory Management Contract

### 8.1 Static Allocation
- All page objects are statically allocated at startup
- No dynamic allocation/deallocation during normal operation
- Global page array: `page_object[TOTAL_DENOMINATIONS][TOTAL_PAGES]`

### 8.2 Resource Lifecycle
- **Initialization**: All resources allocated during init_db()
- **Runtime**: No allocation/deallocation except for dirty queue nodes
- **Cleanup**: Platform-specific cleanup (not defined in this interface)

### 8.3 Memory Footprint
- **Total Page Data**: 15 denominations × 1000 pages × 17,408 bytes ≈ 256 MB
- **Metadata Overhead**: Minimal (mutexes, flags, debug fields)
- **Queue Overhead**: Dynamic based on modification rate

## 9. Performance Characteristics

### 9.1 Access Patterns
- **Read Operations**: O(1) direct array access
- **Write Operations**: O(1) modification + O(1) queue insertion
- **Lock Contention**: Per-page granularity minimizes blocking

### 9.2 Scalability Limits
- **Maximum Pages**: 1000 per denomination (fixed)
- **Maximum Denominations**: 15 (fixed)
- **Concurrent Access**: Limited by page-level lock contention

## 10. Integration Requirements

### 10.1 Include Dependencies
Implementations must provide or include:
- Thread synchronization primitives (mutex, condition variables)
- Standard integer types (int8_t, uint16_t, uint32_t)
- Time handling functions (time_t or equivalent)

### 10.2 Compiler/Platform Considerations
- **Thread Safety**: Compiler must support thread-safe code generation
- **Atomic Operations**: Platform should support atomic integer operations
- **Memory Barriers**: Implementation should consider memory ordering

## 11. Validation and Testing Interface

### 11.1 Parameter Validation
All functions must validate:
- Denomination range (-8 to 6)
- Page number range (0 to 999)
- Non-null pointer parameters where required

### 11.2 Debug Support
- Debug fields in page structure support runtime validation
- Error logging should include context information
- Performance metrics should be collectible


This header specification provides complete interface definition for implementing a CloudCoin database system. Developers should implement all specified functions while adapting data types and synchronization primitives to their target platform.