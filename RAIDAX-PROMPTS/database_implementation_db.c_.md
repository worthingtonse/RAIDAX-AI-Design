Language-Agnostic Specification: Database Implementation
1. Module Purpose
This module provides the concrete implementation for the RAIDAX database layer. It manages the in-memory cache of coin data pages and implements an efficient, event-driven mechanism for persisting changes to disk.

2. Data Model Summary
Each page_object represents a block of coin data and must include:

denomination: integer

page_number: integer (page ID)

data: byte array or buffer (Size: RECORDS_PER_PAGE × 17 bytes)

mutex: A per-page lock primitive for thread-safe access.

in_dirty_queue: A boolean flag to prevent redundant queueing.

3. Architectural Requirements
In-Memory First: All data must be loaded from the file system into memory at startup for fast read access.

Asynchronous Persistence: All write operations to disk must be handled by a single, dedicated background thread to avoid blocking worker threads.

Event-Driven Syncing (Core Optimization): The persistence mechanism must be event-driven. A thread-safe, concurrent queue (the "dirty page queue") must be used. When a worker thread modifies a page, it adds the page to this queue and signals the persistence thread. The persistence thread must sleep until signaled.

4. Core Logic and Implementation Details
4.1. Initialization
initialize_database():

On startup, iterate through all possible denominations and pages.

For each page, check if a corresponding file exists on disk. If not, create and populate it with initial, randomly generated data.

Load the contents of every page file into the corresponding in-memory page object.

Initialize the dirty page queue and its synchronization primitives (e.g., a mutex and a condition variable) and launch the persistence_thread.

4.2. The Dirty Page Queue
add_page_to_dirty_queue(page_object):

This function must be thread-safe.

It must first acquire the lock for the page object itself to check and set the in_dirty_queue flag.

It then acquires the lock for the global dirty queue, adds the page, and signals the condition variable.

4.3. The Persistence Thread
persistence_thread():

This function runs in an infinite loop, waiting on a condition variable.

When woken, it locks the dirty queue, moves the entire current queue to a local list, and then unlocks the global queue to minimize contention.

It then iterates through its local list of dirty pages, acquiring each page's lock, calling sync_page(), resetting the in_dirty_queue flag, and releasing the lock.

4.4. sync_page(page_object)
Role: A utility function that performs the actual file I/O.

Logic:

Construct a file path based on the page's denomination and number using platform-appropriate path handling.

Open or create the file in write mode (overwriting existing contents), write the page buffer, and ensure the file is properly closed or flushed to disk.

5. Error Handling & Security
All disk and I/O operations must fail gracefully if permissions or space are insufficient.

Errors must be logged via the platform's logging facility. A failed sync_page() call must not crash the persistence thread.

Security Consideration: Implementers should consider file system permissions or encryption-at-rest for sensitive data.





////




# Specification: CloudCoin Database Implementation

## 1. Module Purpose
This module provides the concrete implementation for the RAIDAX database layer. It manages the in-memory cache of coin data pages and implements an efficient, event-driven mechanism for persisting changes to disk. The system is designed to handle high-throughput coin operations while maintaining data integrity and performance.

## 2. System Architecture Overview

### 2.1 Core Components
- **In-Memory Database**: A 3-dimensional array structure holding all coin pages
- **Event-Driven Persistence**: Background thread with queue-based synchronization
- **Thread-Safe Page Access**: Per-page locking with reservation system
- **Hierarchical File System**: Organized by denomination and page ranges

### 2.2 Performance Characteristics
- **Memory-First**: All data loaded at startup for O(1) access
- **Lock Granularity**: Per-page locks to minimize contention
- **Batch Persistence**: Queue-based batching reduces I/O overhead
- **Event-Driven**: No polling, threads sleep until work is available

## 3. Data Model and Constants

### 3.1 System Constants
```
TOTAL_DENOMINATIONS = 15
TOTAL_PAGES = 1000
RECORDS_PER_PAGE = 1024
RECORD_SIZE = 17 bytes (16-byte AN + 1-byte MFS)
MIN_DENOMINATION = -8 (DEN_0_00000001)
MAX_DENOMINATION = 6 (DEN_1000000)
DENOMINATION_OFFSET = 8
RESERVED_PAGE_RELEASE_SECONDS = 16
PAGES_IN_RAM = TOTAL_PAGES (all pages kept in memory)
PATH_MAX = 4096 (or platform equivalent)
```

### 3.2 Denomination Enumeration
The system supports 15 distinct denominations representing different coin values:
```
DEN_0_00000001 = -8  (0.00000001)
DEN_0_0000001  = -7  (0.0000001)
DEN_0_000001   = -6  (0.000001)
DEN_0_00001    = -5  (0.00001)
DEN_0_0001     = -4  (0.0001)
DEN_0_001      = -3  (0.001)
DEN_0_01       = -2  (0.01)
DEN_0_1        = -1  (0.1)
DEN_1          = 0   (1)
DEN_10         = 1   (10)
DEN_100        = 2   (100)
DEN_1000       = 3   (1000)
DEN_10000      = 4   (10000)
DEN_100000     = 5   (100000)
DEN_1000000    = 6   (1000000)
```

### 3.2 Page Object Structure
Each `page_object` must contain:
- `denomination`: signed 8-bit integer (-8 to 8)
- `page_number`: unsigned 32-bit integer (0 to 65535)
- `data`: byte array of size RECORDS_PER_PAGE × 17 bytes
- `mutex`: thread synchronization primitive for page-level locking
- `in_dirty_queue`: boolean flag to prevent duplicate queueing
- `reserved_by`: unsigned 32-bit integer (session ID, 0 = not reserved)
- `reserved_at`: timestamp for reservation expiry

### 3.3 Global Data Structure
```
coins_database[TOTAL_DENOMINATIONS][TOTAL_PAGES] of page_object
```

### 3.4 Dirty Queue Node Structure
```
dirty_page_node:
  - page_reference: pointer to page_object
  - next: pointer to next dirty_page_node
```

## 4. File System Organization

### 4.1 Directory Hierarchy
```
{base_directory}/
├── Data/
│   ├── f8/          # denomination -8 (0xf8)
│   │   ├── 00/      # pages 0x0000-0x00ff
│   │   │   ├── 0000.bin
│   │   │   ├── 0001.bin
│   │   │   └── ...
│   │   ├── 01/      # pages 0x0100-0x01ff
│   │   └── ...
│   ├── f9/          # denomination -7 (0xf9)
│   └── ...
├── Keys/
├── Trades/
├── Transactions/
├── TransactionsOld/
├── Folders/
└── Zones/
```

### 4.2 File Naming Convention
- Page files: `{page_number:04x}.bin` (e.g., "0000.bin", "03e7.bin" for page 999)
- Directory structure: `Data/{denomination:02x}/{page_msb:02x}/`
- Where `page_msb = (page_number >> 8) & 0xff` (since max page is 999, only first few MSB directories will be used)

## 5. Initialization Process

### 5.1 initialize_database()
1. **Directory Creation**: Create all required directories with proper permissions (0755)
2. **Denomination Iteration**: For each denomination from MIN_DENOMINATION to MAX_DENOMINATION:
   - Create denomination directory if it doesn't exist
   - For each page from 0 to TOTAL_PAGES-1 (0 to 999):
     - Call `initialize_page()` to ensure file exists
3. **Memory Loading**: Call `load_pages_into_memory()`
4. **Queue Initialization**: Initialize dirty page queue and synchronization primitives
5. **Thread Launch**: Start the persistence thread

### 5.2 initialize_page(seed, denomination, page_number)
1. **Path Construction**: Build file path using hierarchical structure
2. **Existence Check**: Skip if file already exists
3. **Directory Creation**: Create page MSB directory if needed
4. **Data Generation**: 
   - Allocate buffer of size RECORDS_PER_PAGE × 17
   - For each record i (0 to RECORDS_PER_PAGE-1):
     - Create input string: `{seed:02x}{denomination:02x}{page_number:04x}{record_index:02x}{raida_number:02x}`
     - Generate MD5 hash and store in buffer at offset i × 17
5. **File Creation**: Write buffer to file with proper error handling

### 5.3 load_pages_into_memory()
1. **Memory Initialization**: Zero-initialize the global coins_database array
2. **Page Loading**: For each valid denomination and page:
   - Initialize page object fields
   - Initialize page mutex
   - Read file contents into page.data
   - Handle file I/O errors gracefully

## 6. Thread-Safe Page Access

### 6.1 get_page(denomination, page_number)
- **Validation**: Check denomination and page_number bounds
- **Index Calculation**: Use `denomination + DENOMINATION_OFFSET` for array indexing
- **Return**: Reference to page_object or NULL if invalid

### 6.2 get_page_with_lock(denomination, page_number)
- Call `get_page()` to get page reference
- Acquire page mutex before returning
- Return locked page or NULL

### 6.3 unlock_page(page_object)
- Release the page mutex

### 6.4 get_page_by_serial_number(denomination, serial_number)
- **Page Calculation**: `page_number = serial_number / RECORDS_PER_PAGE`
- Call `get_page_with_lock(denomination, page_number)`

## 7. Page Reservation System

### 7.1 reserve_page(page_object, session_id)
- Set `page_object.reserved_by = session_id`
- Set `page_object.reserved_at = current_timestamp`

### 7.2 is_page_reserved(page_object)
- Return false if `reserved_by` is 0 or `reserved_at` is 0
- Calculate time difference from `reserved_at` to current time
- If difference > RESERVED_PAGE_RELEASE_SECONDS (16 seconds):
  - Call `release_reserved_page()`
  - Return false
- Return true

### 7.3 release_reserved_page(page_object)
- Set `page_object.reserved_by = 0`
- Set `page_object.reserved_at = 0`

## 8. Event-Driven Persistence System

### 8.1 Global Queue Variables
```
dirty_queue_head: pointer to first dirty_page_node
dirty_queue_mutex: mutex for queue synchronization
dirty_queue_condition: condition variable for thread signaling
```

### 8.2 add_page_to_dirty_queue(page_object)
**Prerequisites**: Page mutex must be held by caller

1. **Duplicate Check**: If `page_object.in_dirty_queue` is non-zero, return immediately
2. **Node Creation**: Allocate new dirty_page_node
3. **Queue Operations**:
   - Lock dirty_queue_mutex
   - Set `page_object.in_dirty_queue = 1`
   - Add node to end of linked list
   - Signal dirty_queue_condition
   - Unlock dirty_queue_mutex

### 8.3 persistence_thread()
**Main Loop**:
1. **Wait for Work**: 
   - Lock dirty_queue_mutex
   - While queue is empty and not shutting down:
     - Wait on dirty_queue_condition
2. **Batch Processing**:
   - Move entire queue to local variable
   - Set dirty_queue_head = NULL
   - Unlock dirty_queue_mutex
3. **Page Synchronization**:
   - For each node in local list:
     - Lock page mutex
     - Call `sync_page(page_object)`
     - Set `page_object.in_dirty_queue = 0`
     - Unlock page mutex
     - Free node
4. **Repeat**: Continue until shutdown signal

### 8.4 sync_page(page_object)
1. **Path Construction**: Build file path from denomination and page_number
2. **File Operations**:
   - Open file in write mode (overwrite existing)
   - Write page.data buffer (RECORDS_PER_PAGE × 17 bytes)
   - Close file handle
3. **Error Handling**: Log all I/O errors with system error codes

## 9. Error Handling and Logging

### 9.1 Error Categories
- **File System Errors**: Permission denied, disk full, path not found
- **Memory Allocation Errors**: Out of memory conditions
- **Threading Errors**: Mutex/condition variable failures
- **Data Validation Errors**: Invalid denomination or page numbers

### 9.2 Error Response Strategy
- **Non-Fatal Errors**: Log error and continue operation
- **Fatal Errors**: Log error and signal shutdown
- **I/O Errors**: Include system error codes in log messages
- **Resource Cleanup**: Ensure all allocated resources are freed

### 9.3 Logging Requirements
- **Debug Level**: Detailed operation tracing
- **Error Level**: All error conditions with context
- **Performance Metrics**: Page sync counts and timing

## 10. Utility Functions

### 10.1 get_denomination_index(denomination)
- Convert signed denomination (-8 to 6) to array index (0 to 14)
- Formula: `denomination + DENOMINATION_OFFSET`

### 10.2 get_denomination_by_index(index)
- Convert array index back to signed denomination
- Formula: `index - DENOMINATION_OFFSET`

## 11. Memory Management

### 11.1 Startup Allocation
- Pre-allocate all page objects in global array
- No dynamic allocation for page data during runtime
- Initialize all mutexes during startup

### 11.2 Runtime Allocation
- Dirty queue nodes: Allocate/deallocate as needed
- Temporary buffers: For file I/O operations only
- Error handling: Graceful degradation on allocation failure

## 12. Concurrency and Thread Safety

### 12.1 Locking Hierarchy
1. **Page Level**: Individual page mutexes (finest granularity)
2. **Queue Level**: Dirty queue mutex (coarse granularity)
3. **No Global Locks**: Avoid system-wide contention

### 12.2 Deadlock Prevention
- **Consistent Ordering**: Always acquire page lock before checking/setting in_dirty_queue
- **Minimal Hold Time**: Release locks as quickly as possible
- **No Nested Locking**: Avoid holding multiple page locks simultaneously

### 12.3 Thread Coordination
- **Worker Threads**: Modify pages and queue for persistence
- **Persistence Thread**: Single background thread for all disk I/O
- **Shutdown Protocol**: Use global shutdown flag with condition variable signaling

## 13. Performance Considerations

### 13.1 Optimization Strategies
- **Batch Processing**: Process multiple dirty pages in single persistence cycle
- **Memory Locality**: Organize data structures for cache efficiency
- **I/O Minimization**: Only write pages that have actually changed
- **Lock Contention**: Use fine-grained locking to maximize concurrency

### 13.2 Scalability Factors
- **Page Size**: 1024 records per page balances memory usage and I/O efficiency
- **Queue Depth**: Unbounded queue prevents blocking worker threads
- **Thread Count**: Single persistence thread prevents I/O contention
- **Memory Footprint**: ~256MB total for all pages in RAM (15 denominations × 1000 pages × 17,408 bytes per page)

## 14. Security and Data Integrity

### 14.1 File System Security
- **Directory Permissions**: 0755 (read/execute for all, write for owner)
- **File Permissions**: 0640 (read/write for owner, read for group)
- **Path Validation**: Prevent directory traversal attacks

### 14.2 Data Integrity
- **Atomic Writes**: Complete page writes or failure
- **Error Detection**: Verify all I/O operations complete successfully
- **Graceful Degradation**: Continue operation despite individual page failures

## 15. Implementation Notes

### 15.1 Platform Considerations
- **Path Handling**: Use platform-appropriate path separators and limits
- **Threading**: Adapt to platform-specific thread and synchronization APIs
- **File I/O**: Handle platform-specific error codes and behaviors

### 15.2 Testing Requirements
- **Unit Tests**: Individual function validation
- **Integration Tests**: Multi-threaded operation verification
- **Stress Tests**: High-concurrency scenarios
- **Error Injection**: Simulate I/O failures and resource constraints

This specification provides complete implementation guidance while remaining language-agnostic. Implementers should adapt the threading primitives, file I/O operations, and error handling to their target platform while maintaining the core architectural principles.