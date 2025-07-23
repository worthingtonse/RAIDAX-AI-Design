# On-Demand Page Cache Database System (db)

## Module Purpose
This module implements a high-performance on-demand page cache database layer for RAIDA coin storage. It provides dramatic memory usage reduction through intelligent caching, thread-safe concurrent access, automatic persistence, and efficient page management. The system eliminates the need to keep all coin data in memory while maintaining high performance through strategic caching.

## Constants and Configuration

### Cache Management
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_CACHED_PAGES` | 1000 | Maximum number of pages kept in memory cache |
| `HASH_TABLE_SIZE` | 2048 | Size of hash table for cache lookup (prime number for distribution) |
| `RECORDS_PER_PAGE` | Variable | Number of coin records stored per page file |
| `RESERVED_PAGE_RELEASE_SECONDS` | Variable | Timeout for page reservations in seconds |

### Database Structure
| Constant | Value | Description |
|----------|-------|-------------|
| `TOTAL_PAGES` | Variable | Total number of pages per denomination |
| `MIN_DENOMINATION` | Variable | Minimum supported denomination value |
| `MAX_DENOMINATION` | Variable | Maximum supported denomination value |
| `DENOMINATION_OFFSET` | Variable | Offset for denomination index calculations |

## Data Structures

### Page Structure
| Field | Type | Description |
|-------|------|-------------|
| `data` | Byte Array[RECORDS_PER_PAGE × 17] | Raw coin record data (16-byte AN + 1-byte MFS) |
| `denomination` | 8-bit Integer | Denomination this page belongs to |
| `no` | 16-bit Integer | Page number within the denomination |
| `is_dirty` | Boolean | Flag indicating page has unsaved changes |
| `reserved_at` | Timestamp | Time when page was reserved (0 if not reserved) |
| `reserved_by` | 32-bit Integer | Session ID that reserved this page |
| `mtx` | Mutex | Thread safety lock for page operations |
| `prev` | Page Pointer | Previous page in LRU list |
| `next` | Page Pointer | Next page in LRU list |

### Cache Entry Structure
| Field | Type | Description |
|-------|------|-------------|
| `page` | Page Pointer | Pointer to cached page |
| `key` | 32-bit Integer | Cache key (denomination index << 16 | page number) |
| `next` | Cache Entry Pointer | Next entry in hash table chain |

### Cache Management State
| Field | Type | Description |
|-------|------|-------------|
| `page_cache_hash_table` | Cache Entry Pointer Array[HASH_TABLE_SIZE] | Hash table for O(1) page lookup |
| `lru_head` | Page Pointer | Most recently used page in LRU list |
| `lru_tail` | Page Pointer | Least recently used page in LRU list |
| `cached_pages_count` | Integer | Current number of pages in cache |
| `cache_mutex` | Mutex | Global cache structure protection |

## Core Functionality

### 1. Initialize Database (`init_db`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Initializes the complete database system including cache structures, page file validation, and background persistence thread.

**Process:**
1. **Cache Initialization:**
   - Initializes hash table to all NULL entries
   - Sets up LRU list pointers (head and tail to NULL)
   - Initializes cache statistics and mutex

2. **Page File Validation:**
   - Iterates through all denominations (MIN_DENOMINATION to MAX_DENOMINATION)
   - For each denomination, validates all page files exist
   - Creates missing page files with default coin data using random seed
   - Ensures complete file system structure integrity

3. **Background Thread Startup:**
   - Launches persistence and eviction thread
   - Thread handles periodic dirty page synchronization
   - Configures thread for continuous background operation

4. **System Readiness:**
   - Logs successful initialization
   - Database ready for concurrent page access

**Used By:** Server initialization

**Dependencies:** File system, threading system, configuration

### 2. Get Page by Serial Number with Lock (`get_page_by_sn_lock`)
**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer)

**Returns:** Locked page pointer (NULL on failure)

**Purpose:** Main page access function that provides thread-safe access to coin pages through intelligent caching with automatic eviction and loading.

**Process:**
1. **Page Identification:**
   - Calculates page number from serial number (sn / RECORDS_PER_PAGE)
   - Generates cache key from denomination index and page number
   - Prepares for cache lookup operation

2. **Cache Access (Under Global Lock):**
   - Acquires cache mutex for thread-safe access
   - Performs hash table lookup for existing cached page
   - If found: moves page to front of LRU list and updates access order

3. **Cache Miss Handling:**
   - If page not in cache and cache full: identifies LRU victim for eviction
   - Removes victim from cache structures (hash table and LRU list)
   - Loads requested page from disk into memory
   - Adds new page to cache and LRU list front

4. **Critical Race Condition Fix:**
   - Releases global cache mutex BEFORE handling evicted page
   - Handles dirty page synchronization outside global lock
   - Prevents deadlock between cache mutex and page mutex

5. **Page Lock Acquisition:**
   - Locks the individual page mutex before returning
   - Ensures caller has exclusive access to page data
   - Page remains locked until caller explicitly unlocks

**Performance Features:**
- **O(1) Cache Lookup:** Hash table provides constant-time access
- **LRU Management:** Automatic eviction of least recently used pages
- **Lock Ordering:** Prevents deadlocks through proper lock acquisition order
- **Memory Bounds:** Cache size limits prevent memory exhaustion

**Thread Safety:**
- **Two-Level Locking:** Global cache mutex + individual page mutexes
- **Deadlock Prevention:** Careful lock ordering and release timing
- **Race Condition Prevention:** Fixed critical race in eviction logic

**Used By:** All coin data access operations

**Dependencies:** File system, hash table, LRU management

### 3. Unlock Page (`unlock_page`)
**Parameters:**
- Page pointer

**Returns:** None

**Purpose:** Releases page lock after operations are complete, enabling other threads to access the page.

**Process:**
1. **Validation:**
   - Checks page pointer is not NULL
   - Ensures safe operation

2. **Lock Release:**
   - Releases page-specific mutex
   - Allows other threads to acquire page lock

**Used By:** All functions that acquire page locks

**Dependencies:** Threading system

### 4. Persistence and Eviction Thread (`persistence_and_eviction_thread`)
**Parameters:**
- Thread argument (unused)

**Returns:** Thread result

**Purpose:** Background thread that periodically synchronizes dirty pages to disk with fixed race condition prevention.

**Process:**
1. **Periodic Cycle:**
   - Sleeps for configured flush frequency
   - Wakes up to perform dirty page synchronization
   - Continues until system shutdown

2. **Critical Race Condition Fix - Step 1:**
   - Acquires global cache mutex
   - Builds list of dirty pages while holding lock
   - Copies page pointers to local array (up to MAX_CACHED_PAGES)
   - Releases global cache mutex BEFORE starting I/O operations

3. **Critical Race Condition Fix - Step 2:**
   - Processes identified dirty pages one by one
   - Each page sync operation uses page-specific mutex
   - No global cache lock held during slow disk I/O
   - Prevents deadlock with page access operations

4. **Page Synchronization:**
   - For each dirty page: calls sync_page function
   - sync_page handles individual page locking internally
   - Marks pages as clean after successful write
   - Logs synchronization activity for debugging

**Performance Features:**
- **Deadlock Prevention:** Fixed critical race condition in lock ordering
- **I/O Efficiency:** Batches dirty page synchronization
- **Non-Blocking:** Doesn't block page access during sync operations

**Used By:** Background maintenance

**Dependencies:** File system, timing functions

### 5. Cache Management Functions

#### LRU Move to Front (`lru_move_to_front`)
**Parameters:**
- Page pointer

**Returns:** None (static function)

**Purpose:** Moves accessed page to front of LRU list to reflect recent usage.

**Process:**
1. **Early Exit:**
   - If page is already at head, no operation needed
   - Optimizes for common case

2. **List Manipulation:**
   - Removes page from current position in doubly-linked list
   - Updates previous and next pointers of neighboring pages
   - Handles special cases (tail page, middle page)

3. **Front Insertion:**
   - Inserts page at front of LRU list
   - Updates head pointer and neighboring page links
   - Maintains list integrity

**Used By:** Cache lookup operations

**Dependencies:** None (self-contained)

#### Cache Add (`cache_add`)
**Parameters:**
- Cache key (32-bit integer)
- Page pointer

**Returns:** None (static function)

**Purpose:** Adds new page to both hash table and LRU list structures.

**Process:**
1. **Hash Table Insertion:**
   - Calculates hash index from cache key
   - Creates new cache entry structure
   - Inserts at head of hash chain (most recent first)

2. **LRU List Insertion:**
   - Adds page to front of LRU list
   - Updates head and tail pointers as needed
   - Maintains doubly-linked list integrity

3. **Statistics Update:**
   - Increments cached page count
   - Updates cache utilization metrics

**Used By:** Page loading operations

**Dependencies:** Memory allocation

#### Cache Lookup (`cache_lookup`)
**Parameters:**
- Cache key (32-bit integer)

**Returns:** Page pointer (NULL if not found, static function)

**Purpose:** Performs fast hash table lookup for cached pages.

**Process:**
1. **Hash Calculation:**
   - Calculates hash index from cache key
   - Accesses appropriate hash table bucket

2. **Chain Traversal:**
   - Iterates through hash chain for matching key
   - Returns page pointer if found
   - Returns NULL if not found in chain

**Performance:** O(1) average case, O(n) worst case for hash collisions

**Used By:** Page access operations

**Dependencies:** None (self-contained)

#### Cache Evict and Get Victim (`cache_evict_and_get_victim`)
**Parameters:** None

**Returns:** Evicted page pointer (NULL if cache empty, static function)

**Purpose:** Selects LRU victim page, removes from cache structures, and returns for handling.

**Process:**
1. **Victim Selection:**
   - Selects least recently used page (LRU tail)
   - Logs eviction for debugging
   - Handles empty cache case

2. **LRU List Removal:**
   - Removes victim from tail of LRU list
   - Updates tail pointer and neighboring links
   - Maintains list integrity

3. **Hash Table Removal:**
   - Calculates victim's hash index
   - Traverses hash chain to find entry
   - Removes entry and frees chain node
   - Updates hash table structure

4. **Statistics Update:**
   - Decrements cached page count
   - Updates cache utilization metrics

**Critical Design:** Must be called while holding cache_mutex

**Used By:** Cache full scenarios during page loading

**Dependencies:** Hash table management

### 6. Disk I/O Operations

#### Load Page from Disk (`load_page_from_disk`)
**Parameters:**
- Denomination (8-bit integer)
- Page number (16-bit integer)

**Returns:** Page pointer (NULL on failure, static function)

**Purpose:** Loads page data from disk file into memory structure.

**Process:**
1. **File Path Construction:**
   - Builds hierarchical path: Data/[den]/[msb]/[page].bin
   - Uses page MSB (most significant byte) for directory organization
   - Provides scalable file system structure

2. **File Access:**
   - Opens page file in read-only mode
   - Handles file not found and access errors
   - Validates file descriptor

3. **Data Loading:**
   - Reads complete page data (RECORDS_PER_PAGE × 17 bytes)
   - Validates read operation success
   - Ensures complete data transfer

4. **Page Structure Initialization:**
   - Allocates page structure in memory
   - Initializes denomination, page number, and flags
   - Sets up page mutex and LRU pointers
   - Marks page as clean (not dirty)

**Used By:** Cache miss handling

**Dependencies:** File system operations

#### Sync Page (`sync_page`)
**Parameters:**
- Page pointer

**Returns:** None

**Purpose:** Writes page data to disk with proper thread safety and race condition prevention.

**Process:**
1. **File Path Construction:**
   - Builds hierarchical path from page denomination and number
   - Uses same directory structure as loading

2. **Critical Race Condition Fix:**
   - Acquires page-specific mutex BEFORE file operations
   - Prevents torn writes and data corruption
   - Ensures atomic page synchronization

3. **File Write Operation:**
   - Opens page file in write-only mode
   - Writes complete page data atomically
   - Validates write operation success

4. **Lock Release:**
   - Releases page mutex after write completion
   - Enables other threads to access page
   - Handles error cases with proper cleanup

**Thread Safety:** Fixed critical race condition through proper mutex usage

**Used By:** Persistence thread, eviction operations

**Dependencies:** File system operations

### 7. Page File Management

#### Initialize Page (`init_page`)
**Parameters:**
- Seed (integer)
- Denomination (8-bit integer)
- Page number (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Creates page file with default coin data if it doesn't exist.

**Process:**
1. **Directory Structure:**
   - Creates hierarchical directory structure if needed
   - Uses denomination and page MSB for organization
   - Handles directory creation errors gracefully

2. **File Existence Check:**
   - Checks if page file already exists
   - Returns success immediately if file present
   - Only creates new files when necessary

3. **Default Data Generation:**
   - Generates default authentication numbers using legacy MD5 hash
   - Uses seed, denomination, page number, and record index as input
   - Creates deterministic but unpredictable default data
   - Sets MFS (months from start) to 0 for new coins

4. **File Creation:**
   - Writes complete page data in single operation
   - Validates write operation success
   - Sets appropriate file permissions

**Used By:** Database initialization

**Dependencies:** File system, cryptographic functions

### 8. Page Reservation System

#### Reserve Page (`reserve_page`)
**Parameters:**
- Page pointer
- Session ID (32-bit integer)

**Returns:** None

**Purpose:** Reserves page for exclusive access by specific session to prevent conflicts during multi-page operations.

**Process:**
1. **Reservation Setup:**
   - Sets reserved_by field to session ID
   - Records current timestamp in reserved_at
   - Logs reservation for debugging

2. **Session Association:**
   - Associates page with specific client session
   - Enables session-based page management
   - Supports multi-page transaction safety

**Used By:** Executive operations, multi-page transactions

**Dependencies:** Timing functions

#### Page Is Reserved (`page_is_reserved`)
**Parameters:**
- Page pointer

**Returns:** Integer (1 if reserved, 0 if not reserved)

**Purpose:** Checks if page is currently reserved and handles automatic timeout.

**Process:**
1. **Reservation Check:**
   - Checks if page has reservation timestamp and session ID
   - Returns 0 immediately if no reservation

2. **Timeout Handling:**
   - Calculates time since reservation
   - Compares against configured timeout value
   - Automatically releases expired reservations

3. **Status Return:**
   - Returns 1 if page is actively reserved
   - Returns 0 if not reserved or reservation expired

**Used By:** Page access validation

**Dependencies:** Timing functions

#### Release Reserved Page (`release_reserved_page`)
**Parameters:**
- Page pointer

**Returns:** None

**Purpose:** Manually releases page reservation before timeout.

**Process:**
1. **Reservation Cleanup:**
   - Clears reserved_by session ID (sets to 0)
   - Clears reserved_at timestamp (sets to 0)
   - Logs release for debugging

**Used By:** Session cleanup, explicit release operations

**Dependencies:** None

### 9. Denomination Index Utilities

#### Get Denomination Index (`get_den_idx`)
**Parameters:**
- Denomination (8-bit integer)

**Returns:** Integer denomination index

**Purpose:** Converts signed denomination to array index for internal use.

**Process:**
1. **Index Calculation:**
   - Adds DENOMINATION_OFFSET to handle negative denominations
   - Converts signed denomination to positive array index
   - Enables array-based denomination storage

**Used By:** All denomination-based operations

**Dependencies:** None

#### Get Denomination by Index (`get_den_by_idx`)
**Parameters:**
- Denomination index (integer)

**Returns:** 8-bit signed integer denomination

**Purpose:** Converts array index back to signed denomination value.

**Process:**
1. **Denomination Calculation:**
   - Subtracts DENOMINATION_OFFSET from index
   - Converts positive array index to signed denomination
   - Enables reverse lookup from index to denomination

**Used By:** All index-based denomination operations

**Dependencies:** None


### Page File Format
- **Fixed Size:** Each page file is exactly RECORDS_PER_PAGE × 17 bytes
- **Binary Format:** Direct binary storage for efficiency
- **Record Structure:** 16-byte authentication number + 1-byte MFS (months from start)
- **No Headers:** Raw coin record data for minimal overhead
- **Atomic Writes:** Complete page writes ensure consistency

### Directory Management
- **Lazy Creation:** Directories created as needed for coins
- **MSB Organization:** High-order page bits determine directory
- **Scalable Structure:** Structure scales to large page counts
- **File System Compatibility:** Works with standard file systems

## Threading and Concurrency Design

### Multi-Level Locking Strategy
- **Global Cache Mutex:** Protects cache data structures during modification
- **Individual Page Mutexes:** Fine-grained locks for page-specific operations
- **LRU List Protection:** Cache mutex protects LRU list modifications
- **Hash Table Protection:** Cache mutex protects hash table operations

### Lock Ordering and Deadlock Prevention
- **Consistent Ordering:** Always acquire cache mutex before page mutexes when both needed
- **Brief Critical Sections:** Cache mutex held only for cache structure operations
- **Long-Term Locks:** Page mutexes held for duration of coin operations
- **Automatic Release:** Page locks automatically released by callers

### Thread-Safe Operations
- **Atomic Cache Updates:** Cache modifications are atomic operations
- **Safe Eviction:** Eviction process coordinates with active operations
- **Reservation Safety:** Page reservations work correctly with concurrent access
- **Background Safety:** Persistence thread operates safely with active cache

### Critical Race Condition Fixes
- **Eviction Race Fix:** Victim page handling moved outside global cache lock
- **Persistence Race Fix:** Dirty page identification separated from synchronization
- **Page Sync Race Fix:** Individual page locks prevent torn writes

## Cache Performance Optimization

### Hash Table Design
- **Prime Table Size:** HASH_TABLE_SIZE uses prime number to reduce clustering
- **Chaining Resolution:** Simple chaining handles hash collisions
- **Load Factor Management:** Cache size limits maintain reasonable load factors
- **Key Distribution:** Cache keys designed for good hash distribution

### LRU List Efficiency
- **O(1) Operations:** All LRU operations are constant time
- **Doubly-Linked List:** Efficient insertion and removal operations
- **Cache Hit Optimization:** Recently used pages moved to front efficiently
- **Eviction Efficiency:** LRU victim selection is immediate

### Memory Access Patterns
- **Locality of Reference:** Cached pages improve temporal locality
- **Reduced I/O:** Cache hits eliminate disk access
- **Prefetch Opportunities:** Sequential access patterns benefit from caching
- **Working Set Management:** Cache size tuned for typical working sets

## Performance Characteristics

### Memory Usage Reduction
- **Dramatic Savings:** Reduces memory usage from GBs to MBs
- **Bounded Usage:** Maximum memory usage controlled by MAX_CACHED_PAGES
- **Efficient Caching:** Only frequently accessed pages kept in memory
- **Automatic Management:** No manual memory management required

### Access Performance
- **Cache Hit Performance:** O(1) access time for cached pages
- **Cache Miss Performance:** Single disk read plus cache insertion
- **Sequential Access:** Good performance for sequential page access
- **Random Access:** Hash table provides efficient random access

### I/O Efficiency
- **Lazy Loading:** Pages loaded only when accessed
- **Batch Persistence:** Background thread batches dirty page writes
- **Atomic Operations:** Complete page reads and writes prevent corruption
- **Minimal Overhead:** Direct binary format minimizes I/O overhead

## Dependencies and Integration

### Required Modules
- **Configuration System:** Working directory, flush frequency, server identification
- **Utilities Module:** Hash functions for default authentication number generation
- **File System Interface:** Directory and file operations
- **Threading Infrastructure:** Mutex and thread management

### External Dependencies
- **File System:** POSIX file operations for page storage
- **Threading System:** POSIX threading for synchronization and background operations
- **Timing System:** Time functions for reservation management
- **Cryptographic Library:** MD5 hash for default authentication number generation

### Used By
- **Authentication System:** Coin verification and ownership operations
- **Executive Commands:** Coin creation and management operations
- **Healing System:** Coin recovery and synchronization operations
- **Change Making:** Break and join operations for denomination conversion
- **Shard Operations:** Cross-shard coin migration and management
- **Integrity System:** Merkle tree construction and verification

### Cross-File Dependencies
- **Configuration Module:** Working directory, flush frequency, server identification
- **Utilities Module:** Hash generation for default authentication numbers
- **File System Interface:** Directory and file operations
- **Threading Infrastructure:** Mutex and thread management

## Error Handling and Recovery

### File System Error Handling
- **Missing Files:** Automatic creation of missing page files with default data
- **Directory Creation:** Automatic creation of missing directory structure
- **I/O Errors:** Graceful handling of disk I/O failures
- **Permission Errors:** Clear error reporting for access issues

### Memory Management
- **Allocation Failures:** Graceful handling of memory allocation failures
- **Cache Overflow:** Automatic eviction prevents memory exhaustion
- **Resource Leaks:** Systematic cleanup prevents memory leaks
- **Thread Safety:** All memory operations are thread-safe

### Data Integrity
- **Atomic Operations:** Page operations are atomic to prevent corruption
- **Validation:** File size and content validation during loading
- **Recovery:** Self-healing creation of missing or corrupted files
- **Consistency:** Cache always reflects disk state correctly

This database module provides a modern, efficient foundation for RAIDA coin storage, dramatically reducing memory requirements while maintaining high performance through intelligent caching, providing thread-safe concurrent access, and ensuring data integrity through robust persistence mechanisms and critical race condition fixes.