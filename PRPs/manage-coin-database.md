# On-Demand Page Cache Database System (db.c)

## Module Purpose
This module implements a high-performance, on-demand page cache database system for the RAIDA network. It provides memory-efficient access to coin data through intelligent caching, lazy loading, and background persistence. The system minimizes memory usage while maximizing performance through LRU-based cache management and asynchronous disk operations.



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


## Core Functionality

### 1. Database Initialization (`init_db`)
**Parameters:**
- None

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Initializes the on-demand page cache system, creates necessary directory structures, and starts background persistence threads.

**Process:**
1. **Cache Initialization:**
   - Initializes hash table for page lookups
   - Sets up LRU linked list structures
   - Initializes cache statistics and counters
   - Creates cache management mutex

2. **File System Setup:**
   - Creates directory structure for all denominations
   - Initializes page files that don't exist
   - Validates file system permissions
   - Sets up page file organization

3. **Background Thread Launch:**
   - Starts persistence and eviction thread
   - Configures thread for background operation
   - Sets up periodic synchronization schedule
   - Initializes thread communication mechanisms

**Dependencies:**
- File system for directory creation
- Threading system for background operations
- Configuration system for paths and parameters

### 2. Page Retrieval with Caching (`get_page_by_sn_lock`)
**Parameters:**
- Denomination identifier (1 byte signed integer)
- Serial number (4 bytes unsigned integer)

**Returns:** Page structure pointer (locked) or null on failure

**Purpose:** Retrieves a page containing the specified coin, using intelligent caching to minimize disk I/O while ensuring thread-safe access.

**Process:**
1. **Cache Key Generation:**
   - Calculates page number from serial number
   - Generates cache key from denomination and page number
   - Ensures consistent key generation across operations

2. **Cache Lookup:**
   - Searches cache hash table for existing page
   - If found, moves page to LRU front
   - If not found, triggers cache miss handling

3. **Cache Miss Handling:**
   - Evicts pages if cache is full
   - Loads page from disk storage
   - Adds new page to cache structures
   - Updates LRU ordering

4. **Page Locking:**
   - Acquires page-specific mutex before returning
   - Ensures thread-safe access to page data
   - Prevents concurrent modification conflicts

**Cache Features:**
- Hash table for O(1) average lookup time
- LRU ordering for intelligent eviction
- Thread-safe access with fine-grained locking
- Automatic cache size management

**Dependencies:**
- Hash table implementation for fast lookups
- LRU management for cache eviction
- Mutex system for thread safety
- Disk I/O for cache misses

### 3. Page Unlocking (`unlock_page`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Releases the page-specific mutex, allowing other threads to access the page data.

**Process:**
1. **Lock Release:**
   - Releases page-specific mutex
   - Validates page pointer before operation
   - Ensures proper lock pairing

**Thread Safety:**
- Must be called after `get_page_by_sn_lock`
- Prevents deadlock through proper lock ordering
- Enables concurrent access to different pages

### 4. Background Persistence Thread (`persistence_and_eviction_thread`)
**Parameters:**
- Thread argument pointer (unused)

**Returns:** Thread result (null)

**Purpose:** Runs continuously in background to persist dirty pages to disk and manage cache eviction.

**Process:**
1. **Periodic Synchronization:**
   - Sleeps for configured flush frequency
   - Wakes up to sync dirty pages
   - Continues until system shutdown

2. **Dirty Page Synchronization:**
   - Scans LRU list for dirty pages
   - Writes modified pages to disk
   - Clears dirty flags after successful write
   - Tracks synchronization statistics

3. **Cache Management:**
   - Monitors cache size and usage
   - Triggers eviction when necessary
   - Maintains optimal cache performance
   - Handles memory pressure situations

**Background Features:**
- Asynchronous disk I/O for performance
- Configurable flush frequency
- Automatic dirty page detection
- Memory pressure handling

### 5. Cache Management Functions

#### LRU List Management (`lru_move_to_front`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Moves a page to the front of the LRU list, marking it as recently used.

**Process:**
1. **List Manipulation:**
   - Removes page from current position
   - Inserts page at list head
   - Updates prev/next pointers
   - Maintains list integrity

#### Cache Addition (`cache_add`)
**Parameters:**
- Cache key (4 bytes)
- Page structure pointer

**Returns:** None

**Purpose:** Adds a new page to the cache hash table and LRU list.

**Process:**
1. **Hash Table Insertion:**
   - Calculates hash index from key
   - Creates new hash table entry
   - Links entry to existing chain
   - Updates hash table statistics

2. **LRU List Insertion:**
   - Adds page to front of LRU list
   - Updates list head pointer
   - Maintains list consistency
   - Increments cache count

#### Cache Lookup (`cache_lookup`)
**Parameters:**
- Cache key (4 bytes)

**Returns:** Page structure pointer or null if not found

**Purpose:** Searches the cache hash table for a specific page.

**Process:**
1. **Hash Table Search:**
   - Calculates hash index from key
   - Traverses hash chain for key match
   - Returns matching page or null
   - Maintains search statistics

#### Cache Eviction (`cache_evict`)
**Parameters:**
- None

**Returns:** None

**Purpose:** Removes the least recently used page from cache to make room for new pages.

**Process:**
1. **LRU Victim Selection:**
   - Selects page from LRU tail
   - Ensures page is suitable for eviction
   - Validates eviction safety

2. **Dirty Page Handling:**
   - Syncs dirty pages before eviction
   - Ensures data persistence
   - Prevents data loss

3. **Cache Cleanup:**
   - Removes page from hash table
   - Removes page from LRU list
   - Frees page memory
   - Updates cache statistics

### 6. Disk I/O Operations

#### Page Loading (`load_page_from_disk`)
**Parameters:**
- Denomination identifier (1 byte)
- Page number (2 bytes)

**Returns:** Page structure pointer or null on failure

**Purpose:** Loads a page from disk storage into memory cache.

**Process:**
1. **File Path Construction:**
   - Builds file path from denomination and page number
   - Uses hierarchical directory structure
   - Handles MSB-based subdirectories

2. **File Reading:**
   - Opens page file for reading
   - Reads complete page data
   - Validates read operation success
   - Closes file handle

3. **Page Structure Initialization:**
   - Allocates page memory
   - Initializes page metadata
   - Sets up page mutex
   - Initializes LRU pointers

#### Page Synchronization (`sync_page`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Writes a page's data to disk storage, ensuring persistence.

**Process:**
1. **File Path Construction:**
   - Builds file path from page metadata
   - Uses same path logic as loading
   - Handles directory hierarchy

2. **File Writing:**
   - Opens page file for writing
   - Writes complete page data
   - Validates write operation success
   - Closes file handle

3. **Error Handling:**
   - Handles disk full conditions
   - Manages file permission errors
   - Provides detailed error logging
   - Maintains system stability

### 7. Page Management Functions

#### Page Initialization (`init_page`)
**Parameters:**
- Random seed (4 bytes)
- Denomination identifier (1 byte)
- Page number (4 bytes)

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Creates and initializes a new page file with default authentication numbers.

**Process:**
1. **File Existence Check:**
   - Checks if page file already exists
   - Returns success if file present
   - Proceeds with creation if missing

2. **Directory Creation:**
   - Creates denomination directory structure
   - Handles MSB-based subdirectories
   - Sets appropriate permissions

3. **Authentication Number Generation:**
   - Generates secure authentication numbers using SHA-256
   - Uses seed, denomination, page, record, and server ID
   - Ensures cryptographic security
   - Initializes MFS values to 0 (available)

4. **File Creation:**
   - Writes complete page data to disk
   - Validates write operation
   - Sets file permissions
   - Creates atomic operation

#### Page Reservation (`reserve_page`)
**Parameters:**
- Page structure pointer
- Session ID (4 bytes)

**Returns:** None

**Purpose:** Reserves a page for exclusive access by a specific session.

**Process:**
1. **Reservation Setup:**
   - Sets session ID in page structure
   - Records reservation timestamp
   - Prevents concurrent access
   - Provides session-based locking

#### Page Reservation Checking (`page_is_reserved`)
**Parameters:**
- Page structure pointer

**Returns:** Boolean indicating reservation status

**Purpose:** Checks if a page is currently reserved and handles reservation timeouts.

**Process:**
1. **Reservation Validation:**
   - Checks for active reservation
   - Validates reservation timestamp
   - Handles expired reservations
   - Returns current status

2. **Timeout Handling:**
   - Calculates reservation age
   - Releases expired reservations
   - Provides automatic cleanup
   - Prevents permanent locks

#### Page Reservation Release (`release_reserved_page`)
**Parameters:**
- Page structure pointer

**Returns:** None

**Purpose:** Releases a page reservation, making it available for other sessions.

**Process:**
1. **Reservation Cleanup:**
   - Clears session ID
   - Clears reservation timestamp
   - Makes page available
   - Logs reservation release

## Data Structures and Formats

### Page Cache Configuration
- **MAX_CACHED_PAGES:** Maximum pages kept in memory (1000)
- **HASH_TABLE_SIZE:** Hash table size for lookups (2048)
- **LRU Management:** Doubly-linked list for cache ordering
- **Fine-Grained Locking:** Per-page mutexes for concurrency

### Page Structure
- **Page Data:** 17 bytes per record (16-byte AN + 1-byte MFS)
- **Page Metadata:** Denomination, page number, timestamps
- **Cache Metadata:** LRU pointers, dirty flag, reservation info
- **Thread Safety:** Per-page mutex for concurrent access

### Cache Entry Structure
- **Key:** Combined denomination and page number
- **Page Pointer:** Reference to cached page
- **Hash Chain:** Linked list for collision resolution
- **Statistics:** Access counts and timing information

### File Organization
- **Hierarchical Structure:** Denomination/MSB/Page organization
- **Binary Format:** Direct page data storage
- **Atomic Operations:** Safe concurrent file access
- **Error Recovery:** Graceful handling of I/O errors

## Performance Characteristics

### Cache Efficiency
- **Hit Rate Optimization:** LRU replacement maximizes hit rates
- **Memory Conservation:** Only active pages kept in memory
- **Scalability:** Hash table provides O(1) average lookup
- **Concurrency:** Fine-grained locking enables parallel access

### I/O Optimization
- **Lazy Loading:** Pages loaded only when needed
- **Background Persistence:** Asynchronous dirty page writing
- **Batch Operations:** Multiple pages handled efficiently
- **Error Recovery:** Graceful handling of I/O failures

### Memory Management
- **Bounded Cache:** Maximum memory usage controlled
- **Automatic Eviction:** LRU-based memory management
- **Resource Cleanup:** Proper memory deallocation
- **Thread Safety:** Safe concurrent memory operations

## Security Considerations

### Cryptographic Security
- **SHA-256 Authentication:** Secure authentication number generation
- **Random Seed Usage:** Cryptographically secure initialization
- **Key Derivation:** Secure derivation from multiple sources
- **Collision Resistance:** Cryptographically secure hash functions

### Data Integrity
- **Atomic Operations:** File operations are atomic
- **Error Detection:** Comprehensive error checking
- **Backup Mechanisms:** Safe handling of write failures
- **Consistency Checking:** Validation of data integrity

### Access Control
- **Session-Based Locking:** Prevents unauthorized access
- **Timeout Management:** Prevents permanent locks
- **Thread Safety:** Secure concurrent access
- **Resource Protection:** Prevents resource exhaustion

## Dependencies and Integration

### Required Modules
- **Threading System:** For background operations and synchronization
- **File System:** For persistent storage and I/O operations
- **Cryptographic Utilities:** For secure authentication number generation
- **Configuration System:** For paths, parameters, and settings
- **Logging System:** For debugging and monitoring

### External Constants Required
- `RECORDS_PER_PAGE`: Number of records per page
- `TOTAL_PAGES`: Total pages per denomination
- `DENOMINATION_OFFSET`: Denomination indexing offset
- `RESERVED_PAGE_RELEASE_SECONDS`: Reservation timeout
- File path and organization constants

### Used By
- **Command Handlers:** All modules requiring coin data access
- **Authentication System:** For coin verification operations
- **Administrative Tools:** For system management and monitoring
- **Statistics System:** For operation tracking and reporting

## Threading and Concurrency
- **Cache Mutex:** Global cache management synchronization
- **Page Mutexes:** Per-page fine-grained locking
- **Background Thread:** Asynchronous persistence operations
- **Thread Safety:** All operations are thread-safe

## Error Handling and Recovery
- **I/O Error Handling:** Graceful handling of disk errors
- **Memory Error Handling:** Safe handling of allocation failures
- **Corruption Detection:** Validation of data integrity
- **Recovery Mechanisms:** Automatic recovery from failures

## Configuration and Tuning
- **Cache Size:** Configurable maximum cached pages
- **Hash Table Size:** Tunable for performance optimization
- **Flush Frequency:** Configurable persistence timing
- **Timeout Values:** Configurable reservation timeouts

This database module provides the foundation for all coin data access in the RAIDA network, delivering high performance through intelligent caching while maintaining data integrity and security.