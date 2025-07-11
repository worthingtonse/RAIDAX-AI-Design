
# Database Layer Header Definitions (db.h)

## Module Purpose
This header file defines the interface and data structures for the on-demand page cache database system. It establishes the page structure for the new caching architecture, defines denomination constants, declares the main database access functions, and provides the interface for the revolutionary memory-efficient database layer that loads pages on-demand rather than preloading everything into memory.

## Database Architecture Constants

### Page Organization
- **TOTAL_PAGES:** 1000 - Maximum number of pages per denomination
- **RECORDS_PER_PAGE:** 1024 - Number of coin records stored in each page
- **Page Size:** Each page contains exactly 17,408 bytes (1024 records × 17 bytes per record)

### Denomination System
**Purpose:** Defines the complete range of coin denominations supported by the system

#### Denomination Enumeration
- **DEN_0_00000001:** -8 (smallest denomination: 0.00000001 coins)
- **DEN_0_0000001:** -7 (0.0000001 coins)
- **DEN_0_000001:** -6 (0.000001 coins)
- **DEN_0_00001:** -5 (0.00001 coins)
- **DEN_0_0001:** -4 (0.0001 coins)
- **DEN_0_001:** -3 (0.001 coins)
- **DEN_0_01:** -2 (0.01 coins)
- **DEN_0_1:** -1 (0.1 coins)
- **DEN_1:** 0 (1 coin - base denomination)
- **DEN_10:** 1 (10 coins)
- **DEN_100:** 2 (100 coins)
- **DEN_1000:** 3 (1,000 coins)
- **DEN_10000:** 4 (10,000 coins)
- **DEN_100000:** 5 (100,000 coins)
- **DEN_1000000:** 6 (1,000,000 coins - largest denomination)

#### Denomination Constants
- **MIN_DENOMINATION:** DEN_0_00000001 (-8)
- **MAX_DENOMINATION:** DEN_1000000 (6)
- **TOTAL_DENOMINATIONS:** 15 (complete range of supported denominations)
- **DENOMINATION_OFFSET:** 8 (conversion constant for array indexing)

#### time out constant
| Constant Name                   | Value | Description                                          |
| ------------------------------- | ----- | ---------------------------------------------------- |
| `RESERVED_PAGE_RELEASE_SECONDS` | `16`  | Timeout after which a reserved page is auto-released |


### derived constants

| Name                 | Value                     | Purpose                                                                      |
| -------------------- | ------------------------- | ---------------------------------------------------------------------------- |
| `RECORD_SIZE`        | `17` bytes                | 16 bytes AN + 1 byte MFS                                                     |
| `PAGE_FILE_SIZE`     | `17 * 1024 = 17408` bytes | Size of binary file for each page                                            |
| `DENOM_HEX_RANGE`    | `00` to `0e` (0–14)       | Folder mapping for denominations (hexadecimal)                               |
| `PAGE_MSB_DIR_RANGE` | `00` to `03`              | Since only 1000 pages used (0x0000–0x03e7), directory prefix only goes to 03 |


## Page Structure Definition

### Re-Architected Page Structure (`page_s`)
**Purpose:** Represents a single page of coin data in the new cache-based system, designed as a node in both a hash map and doubly-linked LRU list.

#### Core Data Storage
- **data:** Array of RECORDS_PER_PAGE × 17 bytes containing actual coin information
  - Each 17-byte record: 16 bytes for Authentication Number (AN) + 1 byte for MFS (Months From Start)
- **denomination:** 8-bit signed integer identifying the coin type
- **no:** 16-bit unsigned integer page number within the denomination

#### Concurrency and State Management
- **mtx:** Page-specific mutex for thread-safe access to individual pages
- **is_dirty:** Integer flag indicating page has been modified since loading from disk

#### Client Reservation System
- **reserved_at:** Timestamp when page was reserved for exclusive access
- **reserved_by:** 32-bit unsigned integer session ID that reserved the page

#### NEW: LRU Cache Management
- **prev:** Pointer to previous page in the LRU (Least Recently Used) linked list
- **next:** Pointer to next page in the LRU linked list

**Architecture Notes:**
- Pages are dynamically allocated only when accessed (on-demand loading)
- LRU pointers enable efficient cache eviction when memory limits are reached
- Page-level locking allows fine-grained concurrency control
- Dirty flag enables efficient background synchronization to disk

## Function Interface Declarations

### Core Database Functions

#### `init_db`
**Parameters:** None
**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Initializes the on-demand page cache system and background threads

**Functionality:**
- Sets up hash table for page cache lookups
- Initializes LRU list management structures
- Starts background persistence and eviction thread
- Creates directory structure for page files if needed

#### `init_page`
**Parameters:**
- Seed value (integer for random generation)
- Denomination identifier (8-bit signed integer)
- Page number (integer)

**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Creates physical page file on disk if it doesn't exist

**Functionality:**
- Generates initial page data using cryptographic methods
- Creates hierarchical directory structure as needed
- Writes new page file with proper permissions

### NEW: Primary Page Access Function

#### `get_page_by_sn_lock`
**Parameters:**
- Denomination identifier (8-bit signed integer)
- Serial number (32-bit unsigned integer)

**Returns:** Pointer to page structure (NULL on failure)
**Purpose:** Main interface for page access with intelligent caching

**Revolutionary Functionality:**
- Implements cache-first lookup strategy
- Loads pages from disk only when not in cache (cache miss)
- Updates LRU ordering for cache efficiency
- Returns page in locked state for thread safety
- Handles cache eviction when memory limits reached

**Performance Characteristics:**
- **Cache Hit:** O(1) hash lookup + O(1) LRU update
- **Cache Miss:** O(1) hash + disk I/O + cache insertion
- **Thread Safety:** Returns pre-locked page for exclusive access

#### `unlock_page`
**Parameters:**
- Page structure pointer

**Returns:** None
**Purpose:** Releases exclusive lock on a page after use

**Thread Safety:** Essential counterpart to `get_page_by_sn_lock` for proper concurrency

### Background Processing

#### `persistence_and_eviction_thread`
**Parameters:** Thread argument pointer (unused)
**Returns:** Thread result pointer
**Purpose:** Background thread managing cache persistence and memory management

**Functionality:**
- Periodically writes dirty pages to disk
- Manages cache eviction when memory pressure exists
- Balances performance with data durability
- Runs continuously until server shutdown

### Page Synchronization

#### `sync_page`
**Parameters:**
- Page structure pointer

**Returns:** None
**Purpose:** Writes individual page data to corresponding disk file

**Usage:**
- Called by background thread for dirty pages
- Can be called manually for immediate synchronization
- Handles file I/O errors gracefully

### Client Reservation System

#### `page_is_reserved`
**Parameters:**
- Page structure pointer

**Returns:** Integer boolean (1 if reserved, 0 if available)
**Purpose:** Checks if page is currently reserved with automatic timeout handling

#### `reserve_page`
**Parameters:**
- Page structure pointer
- Session ID (32-bit unsigned integer)

**Returns:** None
**Purpose:** Reserves page for exclusive access by specific session

#### `release_reserved_page`
**Parameters:**
- Page structure pointer

**Returns:** None
**Purpose:** Manually releases page reservation

**Reservation Features:**
- **Automatic Timeout:** Reservations expire after RESERVED_PAGE_RELEASE_SECONDS
- **Session Tracking:** Prevents unauthorized access to reserved pages
- **Used By:** Shard operations requiring exclusive page control

### Utility Functions

#### `get_den_idx`
**Parameters:**
- Denomination value (8-bit signed integer)

**Returns:** Integer array index (0-14)
**Purpose:** Converts denomination value to array index

#### `get_den_by_idx`
**Parameters:**
- Array index (integer)

**Returns:** Denomination value (8-bit signed integer)
**Purpose:** Converts array index back to denomination value

**Conversion Formula:**
- Index = Denomination + DENOMINATION_OFFSET
- Denomination = Index - DENOMINATION_OFFSET

## Memory Architecture Comparison

### Traditional Approach (Removed)
- **Memory Usage:** All pages loaded at startup (~17GB for full database)
- **Startup Time:** Several minutes to load complete database
- **Memory Efficiency:** Poor - most pages never accessed
- **Scalability:** Limited by available RAM

### NEW: On-Demand Cache Architecture
- **Memory Usage:** Only accessed pages cached (~17MB for 1000-page cache)
- **Startup Time:** Seconds - no preloading required
- **Memory Efficiency:** Excellent - only active data in memory
- **Scalability:** Excellent - memory usage independent of database size

## Cache Configuration Constants

### Performance Tuning
- **MAX_CACHED_PAGES:** Maximum pages to keep in memory cache
- **HASH_TABLE_SIZE:** Size of hash table for page lookups
- **RESERVED_PAGE_RELEASE_SECONDS:** 16 seconds - automatic reservation timeout

### Cache Behavior
- **LRU Eviction:** Least recently used pages removed when cache is full
- **Dirty Page Tracking:** Modified pages marked for background synchronization
- **Hash Distribution:** Efficient key distribution for optimal lookup performance

## Threading and Concurrency Model

### Synchronization Strategy
- **Cache-Level Mutex:** Protects hash table and LRU list modifications
- **Page-Level Mutexes:** Individual locks for page data access
- **Lock Ordering:** Cache mutex acquired first, then page mutex

### Thread Safety Guarantees
- **Concurrent Access:** Multiple threads can access different pages simultaneously
- **Atomic Cache Operations:** Cache modifications are atomic and consistent
- **No Race Conditions:** Proper synchronization prevents data corruption

### Performance Optimization
- **Fine-Grained Locking:** Minimizes lock contention between threads
- **Lock-Free Page Access:** Page data access doesn't require cache-level locking
- **Background Processing:** Dirty page writes don't block page access

## Integration Dependencies

### Required External Definitions
- **Threading Types:** Mutex types from threading system
- **Time Types:** Timestamp types for reservation management
- **Memory Management:** Allocation and deallocation functions
- **File System:** I/O operations and path management

### Provided to Other Modules
- **Page Access Interface:** Primary database access through `get_page_by_sn_lock`
- **Page Structure:** Direct access to coin data after locking
- **Denomination Utilities:** Conversion between values and indices
- **Reservation System:** Exclusive page access for critical operations

### Used By
- **Command Handlers:** All operations requiring coin data access
- **Locker Systems:** Coin storage and retrieval operations
- **Authentication Systems:** Coin ownership verification
- **Administrative Commands:** System monitoring and maintenance

## File System Organization

### Directory Structure
```
{base_path}/Data/{denomination_hex}/{page_msb_hex}/{page_number_hex}.bin
```

### Path Components
- **Denomination Directory:** Two-digit hex (00-0E for denominations -8 to +6)
- **Page MSB Directory:** Upper 8 bits of page number for organization
- **Page File:** Four-digit hex page number with .bin extension

### File Format
- **Size:** Exactly RECORDS_PER_PAGE × 17 bytes
- **Content:** Sequential coin records without headers or metadata
- **Permissions:** Created with restricted access (0640)

## Performance Characteristics

### Access Patterns
- **Sequential Access:** Efficient for operations on consecutive serial numbers
- **Random Access:** Optimized through hash-based cache lookups
- **Batch Operations:** Multiple pages can be accessed concurrently

### I/O Optimization
- **Read-Ahead:** Potential for predictive page loading
- **Write Batching:** Background thread batches dirty page writes
- **Cache Locality:** LRU policy keeps hot data in memory

This header defines a revolutionary database architecture that dramatically improves memory efficiency while maintaining high performance through intelligent caching and on-demand loading strategies.