
# On-Demand Page Cache Database System (db.c)

## Module Purpose
This module implements a high-performance on-demand page caching database system for RAIDA coin storage, replacing traditional full-memory loading with intelligent caching. It provides thread-safe page access, LRU-based cache management, background persistence, page reservation systems, and efficient coin data management while minimizing memory usage through selective loading.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_CACHED_PAGES` | 1000 | Maximum number of pages kept in RAM simultaneously |
| `HASH_TABLE_SIZE` | 2048 | Hash table size for cache lookups (should be prime) |
| `RECORDS_PER_PAGE` | Variable | Number of coin records per database page |
| `TOTAL_PAGES` | Variable | Total number of pages per denomination |
| `RESERVED_PAGE_RELEASE_SECONDS` | Variable | Timeout for automatic page reservation release |
| `DENOMINATION_OFFSET` | Variable | Offset for denomination index calculations |

## Data Structures

### Page Structure
| Field | Type | Description |
|-------|------|-------------|
| `denomination` | 8-bit Integer | Coin denomination for this page |
| `no` | 16-bit Integer | Page number within denomination |
| `data[RECORDS_PER_PAGE * 17]` | Byte Array | Coin records (16-byte AN + 1-byte MFS each) |
| `is_dirty` | Boolean | Flag indicating page needs persistence |
| `reserved_at` | Timestamp | When page was reserved (0 = not reserved) |
| `reserved_by` | 32-bit Integer | Session ID that reserved this page |
| `mtx` | Mutex | Page-specific lock for thread safety |
| `prev` | Page Pointer | Previous page in LRU list |
| `next` | Page Pointer | Next page in LRU list |

### Cache Entry Structure
| Field | Type | Description |
|-------|------|-------------|
| `page` | Page Pointer | Pointer to cached page |
| `key` | 32-bit Integer | Cache key (denomination << 16) | page_number |
| `next` | Cache Entry Pointer | Next entry in hash table chain |

### Coin Record Format (17 bytes per record)
| Field | Size | Description |
|-------|------|-------------|
| Authentication Number | 16 bytes | Unique coin identifier/ownership proof |
| MFS (Months From Start) | 1 byte | Coin status/timestamp (0 = available) |

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

### 1. Initialize Database (`init_db`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Initializes the on-demand page cache system, creates necessary directory structures, and starts background persistence threads.

**Process:**
1. **Cache Initialization:**
   - Initializes hash table for cache lookups
   - Sets up LRU list pointers
   - Initializes cache statistics and counters

2. **Directory Structure Creation:**
   - Creates directory hierarchy for all denominations
   - Ensures page files exist or creates them with default data
   - Validates file system permissions and accessibility

3. **Background Thread Startup:**
   - Launches persistence and eviction thread
   - Configures thread for background operation
   - Establishes thread cleanup and shutdown procedures

**Used By:** Server initialization, system startup

**Dependencies:** File system access, threading system

### 2. Get Page by Serial Number with Lock (`get_page_by_sn_lock`)
**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer)

**Returns:** Locked page pointer (NULL on failure)

**Purpose:** Primary interface for accessing coin data with automatic cache management, lazy loading, and thread-safe locking.

**Process:**
1. **Cache Key Calculation:**
   - Calculates page number from serial number
   - Constructs cache key from denomination and page number
   - Prepares for hash table lookup

2. **Cache Lookup:**
   - Searches hash table for existing cached page
   - If found (cache hit):
     - Moves page to front of LRU list
     - Locks page for caller use
     - Returns locked page pointer

3. **Cache Miss Handling:**
   - Checks if cache is full (≥ MAX_CACHED_PAGES)
   - If full, evicts least recently used page
   - Loads page from disk storage
   - Adds new page to cache and LRU list

4. **Thread Safety:**
   - Acquires cache-wide mutex for cache operations
   - Acquires page-specific mutex for caller
   - Returns locked page to ensure exclusive access

**Performance Features:**
- **O(1) Average Lookup:** Hash table provides constant-time cache lookups
- **LRU Efficiency:** Most recently used pages remain available
- **Lazy Loading:** Only requested pages loaded into memory
- **Cache Eviction:** Automatic cleanup when memory limits reached

**Used By:** All coin access operations, authentication, ownership transfer

**Dependencies:** File system for page loading, cache management

### 3. Unlock Page (`unlock_page`)
**Parameters:**
- Page pointer

**Returns:** None

**Purpose:** Releases page-specific lock after operations are complete, enabling other threads to access the page.

**Process:**
1. **Lock Release:**
   - Releases page-specific mutex
   - Allows other threads to access page
   - Maintains cache consistency

**Used By:** All functions that acquire page locks

### 4. Persistence and Eviction Thread (`persistence_and_eviction_thread`)
**Parameters:**
- Thread argument (unused)

**Returns:** Thread result

**Purpose:** Background thread that periodically saves dirty pages to disk and manages cache eviction to maintain system responsiveness.

**Process:**
1. **Periodic Operation:**
   - Sleeps for configured flush frequency
   - Wakes up to perform maintenance tasks
   - Continues until system shutdown

2. **Dirty Page Synchronization:**
   - Scans all cached pages for dirty flag
   - Writes modified pages to disk storage
   - Clears dirty flag after successful write
   - Counts and reports synchronization statistics

3. **Cache Management:**
   - Monitors cache size and usage patterns
   - Manages eviction decisions based on LRU policy
   - Ensures cache stays within memory limits

**Performance Features:**
- **Asynchronous Persistence:** Doesn't block main operations
- **Batch Writes:** Efficient disk I/O through batched operations
- **Memory Management:** Prevents cache from consuming excessive memory

**Used By:** Background system maintenance

**Dependencies:** File system for persistence, configuration for timing

### 5. Cache Management Functions

#### Add to Cache (`cache_add`)
**Parameters:**
- Cache key (32-bit integer)
- Page pointer

**Returns:** None

**Purpose:** Adds newly loaded page to cache hash table and LRU list.

**Process:**
1. **Hash Table Insertion:**
   - Calculates hash table index from cache key
   - Creates new cache entry
   - Links entry into hash table chain

2. **LRU List Management:**
   - Adds page to front of LRU list (most recently used)
   - Updates LRU list pointers
   - Increments cached page counter

#### Cache Lookup (`cache_lookup`)
**Parameters:**
- Cache key (32-bit integer)

**Returns:** Page pointer (NULL if not found)

**Purpose:** Searches cache hash table for requested page.

**Process:**
1. **Hash Calculation:**
   - Calculates hash table index
   - Searches hash table chain for matching key

2. **Key Comparison:**
   - Compares cache keys for exact match
   - Returns page pointer if found

#### Cache Eviction (`cache_evict`)
**Parameters:** None

**Returns:** None

**Purpose:** Removes least recently used page from cache to free memory.

**Process:**
1. **LRU Selection:**
   - Selects page from tail of LRU list (least recently used)
   - Checks if page needs persistence before eviction

2. **Persistence Check:**
   - If page is dirty, writes to disk before eviction
   - Ensures no data loss during eviction

3. **Cache Removal:**
   - Removes page from hash table
   - Removes from LRU list
   - Frees page memory
   - Decrements cached page counter

### 6. Page Loading from Disk (`load_page_from_disk`)
**Parameters:**
- Denomination (8-bit integer)
- Page number (16-bit integer)

**Returns:** Page pointer (NULL on failure)

**Purpose:** Loads page data from disk storage into memory with complete initialization.

**Process:**
1. **File Path Construction:**
   - Builds hierarchical file path from denomination and page number
   - Uses MSB-based directory structure for organization

2. **File System Access:**
   - Opens page file in read-only mode
   - Reads complete page data (RECORDS_PER_PAGE × 17 bytes)
   - Handles file system errors gracefully

3. **Page Initialization:**
   - Allocates memory for page structure
   - Initializes page metadata (denomination, number, flags)
   - Initializes page-specific mutex
   - Sets up LRU list pointers

**Used By:** Cache miss handling, page loading operations

**Dependencies:** File system access, memory management

## Page Reservation System

### Reserve Page (`reserve_page`)
**Parameters:**
- Page pointer
- Session ID (32-bit integer)

**Returns:** None

**Purpose:** Reserves page for exclusive access by specific session, preventing concurrent modifications.

**Process:**
1. **Reservation Setup:**
   - Records session ID as page owner
   - Sets reservation timestamp
   - Logs reservation for debugging

### Check Page Reservation (`page_is_reserved`)
**Parameters:**
- Page pointer

**Returns:** Boolean (true if reserved)

**Purpose:** Checks if page is currently reserved and handles automatic timeout.

**Process:**
1. **Reservation Status Check:**
   - Checks if page has active reservation
   - Calculates time since reservation
   - Automatically releases expired reservations

2. **Timeout Handling:**
   - Compares elapsed time with timeout threshold
   - Calls release function for expired reservations
   - Returns current reservation status

### Release Reserved Page (`release_reserved_page`)
**Parameters:**
- Page pointer

**Returns:** None

**Purpose:** Manually releases page reservation, making page available for other sessions.

**Process:**
1. **Reservation Cleanup:**
   - Clears session ID
   - Resets reservation timestamp
   - Logs release for debugging

## File System Integration

### Initialize Page (`init_page`)
**Parameters:**
- Random seed (integer)
- Denomination (8-bit integer)  
- Page number (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Creates page file on disk with default authentication numbers if it doesn't exist.

**Process:**
1. **File Existence Check:**
   - Checks if page file already exists
   - Returns success if file present

2. **Directory Creation:**
   - Creates MSB-based directory structure if needed
   - Handles directory creation errors

3. **Default Content Generation:**
   - Generates default authentication numbers using legacy MD5 hash
   - Creates deterministic but unique content for each coin slot
   - Sets all MFS values to 0 (available)

4. **File Creation:**
   - Writes page content to disk file
   - Validates write operation success

**Security Features:**
- **Deterministic Generation:** Uses seed and position for reproducible content
- **Legacy Compatibility:** Uses MD5 for default ANs to maintain compatibility

### Synchronize Page (`sync_page`)
**Parameters:**
- Page pointer

**Returns:** None

**Purpose:** Writes page content to disk storage, ensuring data persistence.

**Process:**
1. **File Path Construction:**
   - Builds complete file path from page metadata
   - Uses hierarchical directory structure

2. **Atomic Write:**
   - Opens file in write-only mode
   - Writes complete page content
   - Validates write completion

3. **Error Handling:**
   - Logs write errors for debugging
   - Ensures data integrity through validation

## Utility Functions

### Get Denomination Index (`get_den_idx`)
**Parameters:**
- Denomination (8-bit integer)

**Returns:** Denomination index (integer)

**Purpose:** Converts denomination value to array index for internal use.

### Get Denomination by Index (`get_den_by_idx`)
**Parameters:**
- Denomination index (integer)

**Returns:** Denomination (8-bit integer)

**Purpose:** Converts array index back to denomination value.

## Performance Characteristics

### Memory Efficiency
- **Selective Loading:** Only accessed pages loaded into memory
- **LRU Management:** Most useful pages remain cached
- **Bounded Memory:** Maximum cache size prevents memory exhaustion
- **Efficient Eviction:** Least useful pages removed first

### I/O Optimization
- **Lazy Loading:** Pages loaded only when needed
- **Batched Persistence:** Dirty pages written in batches
- **Async Operations:** Background thread handles disk I/O
- **Cache Hits:** Frequent accesses served from memory

### Concurrency Performance
- **Fine-Grained Locking:** Page-level locks minimize contention
- **Cache-Level Locking:** Brief cache locks for management operations
- **Parallel Access:** Multiple threads can access different pages
- **Lock-Free Reads:** Cache lookups use minimal locking

## Security Considerations

### Data Integrity
- **Atomic Operations:** Page modifications are atomic
- **Consistent State:** Cache eviction ensures data persistence
- **Validation:** File operations validated for correctness

### Access Control
- **Reservation System:** Prevents concurrent modifications
- **Session Isolation:** Reservations tied to specific sessions
- **Timeout Protection:** Automatic release prevents deadlocks

### Thread Safety
- **Mutex Protection:** All cache operations protected by mutexes
- **Page Locking:** Individual pages locked for exclusive access
- **Safe Eviction:** Eviction process handles concurrent access safely

## Error Handling and Recovery

### File System Errors
- **Missing Files:** Graceful handling of non-existent pages
- **Permission Errors:** Proper error reporting for access issues
- **Disk Full:** Handling of disk space exhaustion

### Memory Management
- **Allocation Failures:** Graceful handling of memory exhaustion
- **Cache Cleanup:** Proper cleanup on error conditions
- **Resource Leaks:** Prevention of memory and file handle leaks

### Concurrency Errors
- **Deadlock Prevention:** Consistent lock ordering prevents deadlocks
- **Timeout Handling:** Automatic timeout prevents indefinite blocking
- **Error Recovery:** Proper cleanup maintains system consistency

## Dependencies and Integration

### Required Modules
- **Configuration System:** Directory paths, timing parameters, server settings
- **File System:** Directory creation, file I/O operations
- **Threading System:** Mutex operations, thread creation and management
- **Utilities Module:** Hash functions for default authentication number generation

### Used By
- **Authentication System:** Coin verification and ownership operations
- **Executive Commands:** Coin creation and management operations
- **Healing System:** Coin recovery and synchronization operations
- **Change Making:** Break and join operations for denomination conversion
- **Shard Operations:** Cross-shard coin migration and management
- **Integrity System:** Merkle tree construction and verification

### Cross-File Dependencies
- **Configuration Module:** Working directory, flush frequency, server identification
- **Utilities Module:** MD5 hash generation for default authentication numbers
- **File System Interface:** Directory and file operations
- **Threading Infrastructure:** Mutex and thread management

## Threading and Concurrency Design

### Multi-Level Locking Strategy
- **Cache Mutex:** Protects cache data structures during modification
- **Page Mutexes:** Individual page locks for fine-grained concurrency
- **LRU List Protection:** Cache mutex protects LRU list modifications
- **Hash Table Protection:** Cache mutex protects hash table operations

### Lock Ordering and Deadlock Prevention
- **Consistent Ordering:** Always acquire cache mutex before page mutexes
- **Brief Critical Sections:** Cache mutex held only for cache operations
- **Long-Term Locks:** Page mutexes held for duration of coin operations
- **Automatic Release:** Page locks automatically released by callers

### Thread-Safe Operations
- **Atomic Cache Updates:** Cache modifications are atomic operations
- **Safe Eviction:** Eviction process coordinates with active operations
- **Reservation Safety:** Page reservations work correctly with concurrent access
- **Background Safety:** Persistence thread operates safely with active cache

## Cache Performance Optimization

### Hash Table Design
- **Prime Table Size:** Reduces clustering and improves distribution
- **Chaining Resolution:** Simple chaining handles hash collisions
- **Load Factor Management:** Cache size limits maintain reasonable load factors
- **Key Distribution:** Cache keys designed for good hash distribution

### LRU List Efficiency
- **O(1) Operations:** All LRU operations are constant time
- **Minimal Pointer Updates:** Efficient list manipulation
- **Cache Hit Optimization:** Recently used pages moved to front efficiently
- **Eviction Efficiency:** LRU victim selection is immediate

### Memory Access Patterns
- **Locality of Reference:** Cached pages improve temporal locality
- **Reduced I/O:** Cache hits eliminate disk access
- **Prefetch Opportunities:** Sequential access patterns benefit from caching
- **Working Set Management:** Cache size tuned for typical working sets

### Page File Format
- **Fixed Size:** Each page file is exactly RECORDS_PER_PAGE × 17 bytes
- **Binary Format:** Direct binary storage for efficiency
- **No Headers:** Raw coin record data for minimal overhead
- **Atomic Writes:** Complete page writes ensure consistency

### Directory Management
- **Lazy Creation:** Directories created as needed for coins
- **MSB Organization:** High-order page bits determine directory
- **Scalable Structure:** Structure scales to large page counts
- **File System Compatibility:** Works with standard file systems

## Backup and Recovery

### Data Persistence Strategy
- **Write-Through Caching:** Modified pages written to disk periodically
- **Dirty Flag Tracking:** Only modified pages written to disk
- **Atomic File Operations:** Complete page writes prevent corruption
- **Background Persistence:** Asynchronous writes don't block operations

### Recovery Mechanisms
- **Self-Healing:** Missing page files created with default content
- **Corruption Detection:** File size validation detects corruption
- **Graceful Degradation:** System continues operating with some missing pages
- **Administrative Tools:** Support for manual recovery operations

### Consistency Guarantees
- **Page Atomicity:** Individual page operations are atomic
- **Cache Consistency:** Cache always reflects disk state correctly
- **Reservation Consistency:** Page reservations work correctly across failures
- **Background Sync:** Periodic sync ensures bounded data loss

## Performance Monitoring and Tuning

### Cache Statistics
- **Hit Rate Tracking:** Monitor cache effectiveness
- **Eviction Monitoring:** Track cache pressure and eviction frequency
- **Memory Usage:** Monitor total cache memory consumption
- **I/O Statistics:** Track disk read and write operations

### Tuning Parameters
- **Cache Size:** MAX_CACHED_PAGES tunable for memory/performance trade-off
- **Hash Table Size:** HASH_TABLE_SIZE tunable for lookup performance
- **Flush Frequency:** Background sync frequency tunable for persistence/performance
- **Reservation Timeout:** Tunable for session management

### Performance Characteristics
- **Cache Hit Ratio:** Typically >90% for normal workloads
- **Page Load Time:** Single disk seek + read time for cache misses
- **Concurrent Throughput:** Scales with number of cached pages
- **Memory Footprint:** Bounded by MAX_CACHED_PAGES × page size

## Migration and Compatibility

### Legacy System Compatibility
- **File Format Compatibility:** Maintains compatibility with existing page files
- **MD5 Default Generation:** Uses legacy MD5 for default authentication numbers
- **Directory Structure:** Maintains existing directory organization
- **Gradual Migration:** Can operate alongside existing systems

### Upgrade Path
- **In-Place Upgrade:** Can replace existing database systems without data migration
- **Configuration Migration:** Existing configurations work with minimal changes
- **Performance Improvement:** Immediate memory usage reduction
- **Feature Preservation:** All existing functionality preserved

## Administrative Interface

### Cache Management
- **Cache Statistics:** Provides visibility into cache performance
- **Manual Eviction:** Administrative tools can force cache eviction
- **Memory Monitoring:** Real-time memory usage monitoring
- **Performance Tuning:** Runtime adjustment of cache parameters

### Debugging Support
- **Page State Monitoring:** Visibility into page reservation and dirty states
- **Cache Dump:** Ability to examine cache contents for debugging
- **Operation Tracing:** Detailed logging of cache operations
- **Performance Metrics:** Comprehensive performance measurement

This database module provides a modern, efficient foundation for RAIDA coin storage, dramatically reducing memory requirements while maintaining high performance through intelligent caching, providing thread-safe concurrent access, and ensuring data integrity through robust persistence mechanisms.