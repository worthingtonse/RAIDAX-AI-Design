
# On-Demand Page Cache Database Implementation (db)

## Module Purpose
This module implements a revolutionary on-demand page cache database system for efficient coin data management in the RAIDA network. It replaces the traditional approach of loading all pages into memory with an intelligent caching system that loads pages only when needed, maintains an LRU (Least Recently Used) eviction policy, and provides background persistence with dirty page tracking. This approach dramatically reduces memory usage while maintaining high performance.

## Core Architecture

### On-Demand Loading Philosophy
**Approach:** Load pages on-demand when accessed, cache in memory, evict when memory limit reached

**Benefits:**
- Dramatically reduced memory footprint
- Faster server startup times
- Scalable to larger datasets
- Efficient memory utilization

### Cache Management System
- **Hash Table:** Fast O(1) lookup for cached pages
- **LRU List:** Doubly-linked list for efficient cache eviction
- **Dirty Tracking:** Modified pages marked for background synchronization
- **Thread Safety:** Fine-grained locking for concurrent access


### Constants defined 

| Constant Name                   | Value / Source              | Description                                                                                                  |
| ------------------------------- | --------------------------- | ------------------------------------------------------------------------------------------------------------ |
| `MAX_CACHED_PAGES`              | `1000`                      | Maximum number of pages that can be held in the in-memory cache at once                                      |
| `HASH_TABLE_SIZE`               | `2048`                      | Number of buckets in the hash table for page lookup; should be a prime number larger than `MAX_CACHED_PAGES` |
| `RECORDS_PER_PAGE`              | External (Required)         | Number of coin records per page (used for page size calculation, typically 1024)                             |
| `TOTAL_PAGES`                   | External (Required)         | Number of pages per denomination (usually 1000)                                                              |
| `MIN_DENOMINATION`              | External (Required)         | Minimum supported denomination (usually -8)                                                                  |
| `MAX_DENOMINATION`              | External (Required)         | Maximum supported denomination (usually +6)                                                                  |
| `DENOMINATION_OFFSET`           | External (Required)         | Used to map negative/positive denomination to array indices (typically 8)                                    |
| `RESERVED_PAGE_RELEASE_SECONDS` | External (Required)         | Time after which a reserved page is automatically released if not used                                       |
| `PATH_MAX`                      | (Usually 4096)              | Maximum path length for file operations                                                                      |
| `config.flush_freq`             | From `config` struct        | Time interval (in seconds) between background sync flushes                                                   |
| `config.cwd`                    | From `config` struct        | Base path for all page file storage                                                                          |
| `config.raida_no`               | From `config` struct        | Server ID used for page initialization and MD5 hashing                                                       |


###  Denomination Enumeration

The system supports 15 distinct denominations representing different coin values:

DEN_0_00000001 = -8 (0.00000001)
DEN_0_0000001 = -7 (0.0000001)
DEN_0_000001 = -6 (0.000001)
DEN_0_00001 = -5 (0.00001)
DEN_0_0001 = -4 (0.0001)
DEN_0_001 = -3 (0.001)
DEN_0_01 = -2 (0.01)
DEN_0_1 = -1 (0.1)
DEN_1 = 0 (1)
DEN_10 = 1 (10)
DEN_100 = 2 (100)
DEN_1000 = 3 (1000)
DEN_10000 = 4 (10000)
DEN_100000 = 5 (100000)
DEN_1000000 = 6 (1000000)


- Internally, this enumeration is mapped using an offset (DENOMINATION_OFFSET = 8) to convert to array indices from 0 to 14.



### Page Object Structure

Each `page_s` object represents a cached database page and includes:

- `denomination`: signed 8-bit integer (`int8_t`)  
  Represents the coin's denomination, from -8 to +6.

- `no`: unsigned 16-bit integer (`uint16_t`)  
  Page number (0 to 65535), derived from serial number.

- `data`: byte array of size `RECORDS_PER_PAGE × 17`  
  Contains coin record data (each record is 17 bytes: 16-byte AN + 1-byte MFS).

- `is_dirty`: integer boolean flag  
  Indicates whether this page has been modified and needs to be written back to disk.

- `mtx`: pthread mutex  
  Lock for synchronizing concurrent access to the page.

- `reserved_by`: unsigned 32-bit integer  
  Session ID holding the reservation; `0` means not reserved.

- `reserved_at`: time_t  
  Timestamp indicating when the reservation was made.

- `prev`, `next`: pointers to other `page_s` objects  
  Used for maintaining a doubly-linked LRU list for eviction ordering.


## Core Functionality

### 1. Database Initialization (`init_db`)
**Parameters:** None

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Initializes the on-demand page cache database system and starts background persistence thread.

**Process:**
1. **Cache Initialization:**
   - Initializes hash table for page lookups (all entries set to NULL)
   - Sets up LRU list pointers (head and tail initially NULL)
   - Initializes cache statistics (cached page count to 0)
   - Sets up cache mutex for thread synchronization

2. **Directory Structure Creation:**
   - Ensures all required page file directories exist
   - Creates denomination-specific directory hierarchy
   - Initializes page files that don't exist with default data

3. **Background Thread Launch:**
   - Starts persistence and eviction thread for dirty page management
   - Configures thread for continuous operation until shutdown

4. **Validation:**
   - Verifies directory structure creation
   - Confirms thread launch success
   - Validates cache initialization

**Dependencies:**
- File system operations for directory creation
- Threading system for background processing
- Random number generation for page initialization
- Configuration system for base directory paths

### 2. Page Access with Caching (`get_page_by_sn_lock`)
**Parameters:**
- Denomination identifier (8-bit signed integer)
- Serial number (32-bit unsigned integer)

**Returns:** Pointer to page structure (NULL on failure, locked for caller)

**Purpose:** Main interface for page access with intelligent caching and on-demand loading.

**Process:**
1. **Address Calculation:**
   - Calculates page number from serial number (sn / RECORDS_PER_PAGE)
   - Generates cache key from denomination and page number

2. **Cache Lookup:**
   - Acquires cache mutex for thread safety
   - Searches hash table using generated key
   - If found (cache hit):
     - Moves page to front of LRU list
     - Releases cache mutex
     - Locks page mutex for caller
     - Returns page pointer

3. **Cache Miss Handling:**
   - Checks if cache is at capacity (MAX_CACHED_PAGES)
   - If full, evicts least recently used page
   - Loads page from disk using file I/O
   - Adds new page to cache hash table and LRU list
   - Increments cached page count

4. **Page Locking:**
   - Locks individual page mutex before returning
   - Provides thread-safe access to page data
   - Caller responsible for unlocking when done

**Cache Performance:**
- **Hit Ratio:** High hit ratios for frequently accessed pages
- **Miss Penalty:** Single disk read plus cache insertion overhead
- **Eviction Cost:** Dirty page write plus memory deallocation

**Dependencies:**
- File system for page loading
- Hash table management for cache lookups
- LRU list management for eviction policy
- Thread synchronization for concurrent access

### 3. Page Unlocking (`unlock_page`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Releases exclusive lock on a page after use.

**Process:**
1. Validates page pointer is not NULL
2. Releases page-specific mutex
3. Allows other threads to access the page

**Thread Safety:** Essential for preventing deadlocks and enabling concurrent access

### 4. Background Persistence Thread (`persistence_and_eviction_thread`)
**Parameters:** Thread argument pointer (unused)

**Returns:** Thread result pointer

**Purpose:** Background thread that periodically synchronizes dirty pages to disk and manages cache eviction.

**Process:**
1. **Continuous Operation:** Runs until server shutdown signal received

2. **Sleep Cycle:** Sleeps for configured flush frequency between operations

3. **Dirty Page Synchronization:**
   - Acquires cache mutex for safe iteration
   - Traverses LRU list to find dirty pages
   - For each dirty page:
     - Calls sync_page to write data to disk
     - Clears dirty flag
     - Counts synchronized pages
   - Releases cache mutex
   - Logs synchronization statistics

4. **Memory Management:**
   - Monitors cache usage levels
   - Triggers eviction if memory pressure detected
   - Maintains optimal cache performance

**Performance Optimization:**
- Batches disk writes for efficiency
- Minimizes lock contention through brief critical sections
- Balances synchronization frequency with performance

**Dependencies:**
- Configuration system for flush frequency
- File system for disk synchronization
- Logging system for operational status
- Global shutdown signaling

## Cache Management Internals

### 5. LRU List Management (`lru_move_to_front`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Moves accessed page to front of LRU list to indicate recent use.

**Process:**
1. **Early Return:** If page is already at front, no operation needed
2. **List Removal:**
   - Updates previous page's next pointer
   - Updates next page's previous pointer
   - Handles tail pointer if removing last page
3. **Front Insertion:**
   - Sets page as new head of list
   - Updates head pointer
   - Handles empty list case for tail pointer

**Complexity:** O(1) operation using doubly-linked list pointers

### 6. Cache Addition (`cache_add`)
**Parameters:**
- Cache key (32-bit unsigned integer)
- Page structure pointer

**Returns:** None

**Purpose:** Adds newly loaded page to both hash table and LRU list.

**Process:**
1. **Hash Table Insertion:**
   - Calculates hash index from cache key
   - Creates new cache entry structure
   - Links entry into hash table chain
2. **LRU List Insertion:**
   - Adds page to front of LRU list
   - Updates list pointers appropriately
   - Increments cached page count

### 7. Cache Lookup (`cache_lookup`)
**Parameters:**
- Cache key (32-bit unsigned integer)

**Returns:** Pointer to page structure (NULL if not found)

**Purpose:** Searches hash table for cached page by key.

**Process:**
1. Calculates hash index from cache key
2. Traverses hash chain for matching key
3. Returns page pointer if found, NULL otherwise

**Complexity:** O(1) average case, O(n) worst case with hash collisions

### 8. Cache Eviction (`cache_evict`)
**Parameters:** None

**Returns:** None

**Purpose:** Removes least recently used page from cache to free memory.

**Process:**
1. **Victim Selection:** Identifies LRU page (tail of list)
2. **Dirty Page Handling:** If page is dirty, synchronizes to disk first
3. **List Removal:** Removes page from LRU list, updates pointers
4. **Hash Table Removal:** Finds and removes corresponding hash entry
5. **Memory Cleanup:** Destroys page mutex and frees page memory
6. **Statistics Update:** Decrements cached page count

### 9. Disk Page Loading (`load_page_from_disk`)
**Parameters:**
- Denomination identifier (8-bit signed integer)
- Page number (16-bit unsigned integer)

**Returns:** Pointer to page structure (NULL on failure)

**Process:**
1. **File Path Construction:**
   - Builds hierarchical path using denomination and page number
   - Uses MSB of page number for subdirectory organization
2. **File Access:**
   - Opens page file in read-only mode
   - Handles file not found gracefully
3. **Data Loading:**
   - Allocates new page structure
   - Reads complete page data from file
   - Validates read operation success
4. **Page Initialization:**
   - Sets denomination and page number
   - Initializes dirty flag to false
   - Creates page mutex for thread safety
   - Sets up LRU list pointers

## Page File Management

### 10. Page Initialization (`init_page`)
**Parameters:**
- Seed value (integer for random generation)
- Denomination identifier (8-bit signed integer)
- Page number (integer)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Creates page file on disk if it doesn't exist, with cryptographically generated initial data.

**Process:**
1. **Path Construction:** Builds hierarchical directory and file paths
2. **Existence Check:** Returns success if page file already exists
3. **Directory Creation:** Creates necessary subdirectories if missing
4. **Data Generation:**
   - Allocates buffer for page data
   - Generates unique authentication numbers for each coin record
   - Uses MD5 hash of seed, denomination, page, record, and RAIDA ID
5. **File Creation:** Writes generated data to new page file

**Dependencies:**
- File system operations for directory and file creation
- Cryptographic utilities for MD5 hash generation
- Configuration system for RAIDA identification

### 11. Page Synchronization (`sync_page`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Writes modified page data to disk storage.

**Process:**
1. **Path Construction:** Builds file path from page metadata
2. **File Access:** Opens page file in write-only mode
3. **Data Writing:** Writes complete page data to file
4. **Validation:** Verifies write operation completed successfully
5. **Logging:** Records synchronization status

**Thread Safety:** Called with appropriate locking from persistence thread

## Page Reservation System

### 12. Page Reservation (`reserve_page`)
**Parameters:**
- Page structure pointer
- Session ID (32-bit unsigned integer)

**Returns:** None

**Purpose:** Reserves page for exclusive use by specific session.

**Process:**
1. Records session ID in page structure
2. Sets reservation timestamp
3. Logs reservation operation

**Used By:** Shard operations requiring exclusive page access

### 13. Reservation Status Check (`page_is_reserved`)
**Parameters:**
- Page structure pointer

**Returns:** Integer boolean (1 if reserved, 0 if free)

**Purpose:** Checks if page is currently reserved and handles automatic timeout.

**Process:**
1. **Reservation Check:** Validates reservation fields are set
2. **Timeout Handling:** Calculates time since reservation
3. **Automatic Release:** Releases expired reservations
4. **Status Return:** Returns current reservation status

### 14. Reservation Release (`release_reserved_page`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Manually releases page reservation.

**Process:**
1. Clears session ID field
2. Clears reservation timestamp
3. Logs release operation

## Utility Functions

### 15. Denomination Index Conversion (`get_den_idx`, `get_den_by_idx`)
**Parameters:**
- Denomination value (8-bit signed integer) OR index value (integer)

**Returns:** Converted index or denomination value

**Purpose:** Converts between denomination values (-8 to +6) and array indices (0 to 14).

**Implementation:**
- `get_den_idx`: Adds DENOMINATION_OFFSET (8) to denomination
- `get_den_by_idx`: Subtracts DENOMINATION_OFFSET from index

## Performance Characteristics

### Memory Usage
- **Cache Size:** Limited to MAX_CACHED_PAGES (1000 pages)
- **Page Size:** RECORDS_PER_PAGE * 17 bytes per page (17,408 bytes)
- **Maximum Memory:** ~17MB for full cache vs. ~17GB for full database
- **Memory Efficiency:** 99.9% reduction in memory usage

### Access Performance
- **Cache Hit:** O(1) hash lookup + O(1) LRU update
- **Cache Miss:** O(1) hash + disk I/O + cache insertion
- **Eviction:** O(1) LRU removal + potential disk write

### Disk I/O Patterns
- **Read Pattern:** On-demand loading of individual pages
- **Write Pattern:** Periodic batch writes of dirty pages
- **Directory Structure:** Hierarchical organization for efficient access

## Threading and Concurrency

### Synchronization Strategy
- **Cache Mutex:** Protects hash table and LRU list modifications
- **Page Mutexes:** Individual locks for page data access
- **Lock Ordering:** Cache mutex first, then page mutex to prevent deadlocks

### Thread Safety Guarantees
- **Safe Concurrent Access:** Multiple threads can access different pages simultaneously
- **Atomic Cache Operations:** Cache modifications are atomic and consistent
- **No Race Conditions:** Proper synchronization prevents data corruption

### Performance Considerations
- **Fine-Grained Locking:** Minimizes lock contention
- **Lock-Free Reads:** Page data access doesn't require cache lock
- **Background Writes:** Dirty page synchronization doesn't block reads

## Dependencies and Integration

### Required Modules
- **Configuration System:** Base directory paths, server identification, flush frequency
- **File System Interface:** Directory creation, file I/O operations
- **Threading System:** Background processing, mutex synchronization
- **Cryptographic Utilities:** MD5 hash generation for page initialization
- **Memory Management:** Dynamic allocation for pages and cache entries
- **Logging System:** Debug output, error reporting, operational status

### External Constants Required
- `RECORDS_PER_PAGE`: Number of coin records per page (1024)
- `TOTAL_PAGES`: Maximum pages per denomination (1000)
- `MIN_DENOMINATION` / `MAX_DENOMINATION`: Denomination range (-8 to +6)
- `DENOMINATION_OFFSET`: Index conversion constant (8)
- Cache configuration constants for hash table size and maximum pages

### Used By
- **Command Handlers:** All operations requiring coin data access
- **Locker Systems:** Coin storage and retrieval operations
- **Authentication Systems:** Coin ownership verification
- **Administrative Tools:** System monitoring and maintenance

## Error Handling and Recovery

### Page Loading Errors
- **File Not Found:** Treated as normal condition for unused page ranges
- **Read Errors:** Logged and reported to caller as NULL return
- **Permission Errors:** Filesystem access problems handled gracefully
- **Corruption Detection:** Invalid page data detected and reported

### Cache Management Errors
- **Memory Allocation:** Failed allocations handled with graceful degradation
- **Hash Collisions:** Chain traversal handles multiple entries per bucket
- **LRU Corruption:** List integrity maintained through careful pointer management
- **Thread Synchronization:** Deadlock prevention through consistent lock ordering

### Disk Synchronization Errors
- **Write Failures:** Logged but don't prevent continued operation
- **Disk Full:** Handled gracefully with error reporting
- **Permission Denied:** File access problems logged and reported
- **Partial Writes:** Detected and retried when possible

## Configuration and Tuning

### Cache Configuration
- **MAX_CACHED_PAGES:** 1000 pages (tunable based on available memory)
- **HASH_TABLE_SIZE:** 2048 buckets (should be prime number larger than max pages)
- **Cache Hit Ratio:** Target >95% for optimal performance
- **Eviction Frequency:** Balanced with dirty page write batching

### Performance Tuning
- **Flush Frequency:** Balance between data durability and performance
- **Hash Table Size:** Optimize for cache size and collision minimization
- **LRU Management:** Efficient doubly-linked list operations
- **Memory Allocation:** Minimize fragmentation through consistent page sizes

### Directory Organization
- **Hierarchical Structure:** `{base}/Data/{den_hex}/{page_msb}/{page_num}.bin`
- **Denomination Directories:** Two-digit hex (00-0E for denominations -8 to +6)
- **Page Subdirectories:** Upper 8 bits of page number for organization
- **File Naming:** Four-digit hex page number with .bin extension

## Migration from Legacy System

### Compatibility
- **File Format:** Maintains compatibility with existing page files
- **Directory Structure:** Uses same hierarchical organization
- **Data Format:** 17 bytes per coin record (16-byte AN + 1-byte MFS)
- **Initialization:** Existing files detected and used without modification

### Performance Improvements
- **Startup Time:** Reduced from minutes to seconds for large databases
- **Memory Usage:** Reduced from gigabytes to megabytes
- **Scalability:** Handles larger datasets with constant memory usage
- **Response Time:** Faster access to frequently used pages

### Operational Changes
- **Background Processing:** New persistence thread for dirty page management
- **Cache Monitoring:** New metrics for cache hit ratios and eviction rates
- **Memory Management:** Dynamic memory allocation replaces static arrays
- **Concurrency:** Improved multi-threaded access patterns

## Monitoring and Observability

### Cache Statistics
- **Hit Ratio:** Percentage of requests served from cache
- **Miss Rate:** Frequency of disk reads for page loading
- **Eviction Rate:** Frequency of cache page removal
- **Dirty Page Count:** Number of modified pages awaiting synchronization

### Performance Metrics
- **Access Latency:** Time for page access operations
- **Disk I/O Rate:** Frequency of read and write operations
- **Memory Usage:** Current cache memory consumption
- **Thread Contention:** Lock wait times and contention levels

### Operational Monitoring
- **Background Thread Status:** Persistence thread health and activity
- **Disk Space Usage:** Storage consumption for page files
- **Error Rates:** Frequency of I/O errors and failures
- **Cache Efficiency:** Optimal cache size and configuration validation

## Security Considerations

### Data Protection
- **File Permissions:** Page files created with restricted access (0640)
- **Directory Security:** Database directories protected from unauthorized access
- **Memory Protection:** Page data protected through proper locking
- **Integrity Verification:** Page data validated during loading

### Concurrency Security
- **Race Condition Prevention:** Proper synchronization prevents data corruption
- **Deadlock Avoidance:** Consistent lock ordering prevents system hangs
- **Atomic Operations:** Cache modifications are atomic and consistent
- **Thread Safety:** All public interfaces are thread-safe

### Access Control
- **Page Locking:** Prevents concurrent modification of page data
- **Reservation System:** Session-based exclusive access for critical operations
- **Validation:** Input parameters validated before processing
- **Error Handling:** Security-conscious error reporting

## Future Enhancements

### Scalability Improvements
- **Adaptive Cache Size:** Dynamic cache sizing based on system memory
- **Intelligent Prefetching:** Predictive page loading for better hit ratios
- **Compression:** Page compression for reduced memory usage
- **Distributed Caching:** Multi-node cache coordination

### Performance Optimizations
- **Lock-Free Algorithms:** Reduced synchronization overhead
- **NUMA Awareness:** Memory allocation optimization for multi-socket systems
- **SSD Optimization:** I/O patterns optimized for solid-state storage
- **Asynchronous I/O:** Non-blocking disk operations for better concurrency

### Advanced Features
- **Snapshotting:** Point-in-time database snapshots
- **Replication:** Real-time page replication for high availability
- **Checksums:** Data integrity verification for page contents
- **Metrics API:** Detailed performance and usage statistics

This on-demand page cache system represents a fundamental architectural improvement that dramatically reduces memory usage while maintaining high performance, enabling the RAIDA system to scale to much larger datasets efficiently.