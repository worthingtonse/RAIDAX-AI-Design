# Database Header Definitions - Updated (db.h)

## Module Purpose
This header file defines the updated database interface with on-demand page cache system and free pages bitmap integration. It provides the complete interface for the re-architected database layer that dramatically reduces memory usage while maintaining high performance through intelligent caching and real-time bitmap management.

## Denomination System Constants

### Denomination Enumeration
| Denomination | Value | Decimal Equivalent |
|--------------|-------|-------------------|
| `DEN_0_00000001` | -8 | 0.00000001 |
| `DEN_0_0000001` | -7 | 0.0000001 |
| `DEN_0_000001` | -6 | 0.000001 |
| `DEN_0_00001` | -5 | 0.00001 |
| `DEN_0_0001` | -4 | 0.0001 |
| `DEN_0_001` | -3 | 0.001 |
| `DEN_0_01` | -2 | 0.01 |
| `DEN_0_1` | -1 | 0.1 |
| `DEN_1` | 0 | 1 |
| `DEN_10` | 1 | 10 |
| `DEN_100` | 2 | 100 |
| `DEN_1000` | 3 | 1000 |
| `DEN_10000` | 4 | 10000 |
| `DEN_100000` | 5 | 100000 |
| `DEN_1000000` | 6 | 1000000 |

### System Configuration Constants
| Constant | Value | Description |
|----------|-------|-------------|
| `MIN_DENOMINATION` | DEN_0_00000001 | Smallest supported denomination |
| `MAX_DENOMINATION` | DEN_1000000 | Largest supported denomination |
| `TOTAL_DENOMINATIONS` | 15 | Total number of supported denominations |
| `DENOMINATION_OFFSET` | 8 | Offset for converting denomination to array index |
| `TOTAL_PAGES` | 1000 | Number of pages per denomination |
| `RECORDS_PER_PAGE` | 1024 | Number of coin records per page |
| `RESERVED_PAGE_RELEASE_SECONDS` | 16 | Timeout for page reservations in seconds |

## Core Data Structures

### Page Structure (`page_s`)
**Purpose:** Represents a single page of coin data loaded into memory as part of the on-demand cache system.

**Fields:**
| Field | Type | Size | Description |
|-------|------|------|-------------|
| `data` | Byte Array | RECORDS_PER_PAGE × 17 | Raw coin data (16-byte AN + 1-byte MFS) |
| `denomination` | 8-bit Integer | 1 byte | Denomination this page belongs to |
| `no` | 16-bit Integer | 2 bytes | Page number within denomination |
| `mtx` | Mutex | Platform-specific | Thread safety lock for page operations |
| `is_dirty` | Integer | 4 bytes | Flag indicating unsaved changes |
| `reserved_at` | Timestamp | Platform-specific | Reservation timestamp (0 if not reserved) |
| `reserved_by` | 32-bit Integer | 4 bytes | Session ID that reserved this page |
| `prev` | Page Pointer | Pointer size | Previous page in LRU doubly-linked list |
| `next` | Page Pointer | Pointer size | Next page in LRU doubly-linked list |

**Architecture Features:**
- **Cache Node:** Designed as node in hash table and LRU list
- **Thread Safety:** Individual mutex per page for fine-grained locking
- **Dirty Tracking:** Automatic tracking of modifications for persistence
- **Reservation System:** Client-based reservation for multi-step operations
- **LRU Management:** Doubly-linked list pointers for efficient LRU operations

## Primary Interface Functions

### 1. Database Initialization
**Function Name:** Initialize Database

**Purpose:** Initializes complete database system including page cache and background threads

**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Description:** Comprehensive database initialization including cache structures, page file validation, background persistence thread startup, and free pages bitmap initialization.

### 2. Page File Initialization
**Function Name:** Initialize Page

**Purpose:** Creates physical page file if it doesn't exist with default coin data

**Parameters:**
- Seed value (integer) for deterministic default data generation
- Denomination (8-bit integer)
- Page number (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Description:** Ensures page file exists on disk with proper default coin data, creating directory structure as needed.

### 3. **NEW: Free Pages Bitmap Functions**

#### Initialize Free Pages Bitmap
**Function Name:** Initialize Free Pages Bitmap

**Purpose:** Initializes in-memory bitmap system for tracking free coins

**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Description:** **RE-ARCHITECTED FEATURE** - Creates and populates in-memory bitmap by scanning all coin data, providing instant free coin discovery without disk I/O.

#### Update Free Pages Bitmap
**Function Name:** Update Free Pages Bitmap

**Purpose:** Updates bitmap when coin status changes

**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer) 
- Free status (integer: 1 for free, 0 for not free)

**Returns:** None

**Description:** **PERFORMANCE CRITICAL** - Maintains real-time synchronization between coin data and bitmap for instant availability queries.

#### Get Available SNs from Bitmap
**Function Name:** Get Available SNs from Bitmap

**Purpose:** Instantly retrieves available serial numbers from in-memory bitmap

**Parameters:**
- Denomination (8-bit integer)
- Output array for serial numbers (32-bit integer pointer)
- Maximum number of serial numbers to return (integer)

**Returns:** Integer (number of serial numbers found)

**Description:** **PERFORMANCE BREAKTHROUGH** - Eliminates "mega I/O read problem" by providing sub-millisecond free coin discovery from memory.

## Page Access and Management

### 4. **MAIN: Get Page by Serial Number with Lock**
**Function Name:** Get Page by Serial Number with Lock

**Purpose:** Primary page access function providing thread-safe cached page access

**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer)

**Returns:** Locked page pointer (NULL on failure)

**Description:** **CORE FUNCTION** - Provides on-demand page loading with automatic caching, LRU management, and thread-safe access. Pages loaded only when needed, dramatically reducing memory usage.

### 5. Page Lock Management
**Function Name:** Unlock Page

**Purpose:** Releases page lock after operations complete

**Parameters:**
- Page pointer

**Returns:** None

**Description:** Essential for proper resource management and enabling other threads to access pages.

## Page Reservation System

### 6. Page Reservation Check
**Function Name:** Page Is Reserved

**Purpose:** Checks if page is currently reserved with automatic timeout handling

**Parameters:**
- Page pointer

**Returns:** Integer (1 if reserved, 0 if not reserved)

**Description:** Implements automatic timeout management for page reservations, releasing expired reservations automatically.

### 7. Reserve Page
**Function Name:** Reserve Page

**Purpose:** Reserves page for exclusive access by specific session

**Parameters:**
- Page pointer
- Session ID (32-bit integer)

**Returns:** None

**Description:** Implements session-based page reservation for multi-step operations, preventing concurrent modification conflicts.

### 8. Release Reserved Page
**Function Name:** Release Reserved Page

**Purpose:** Manually releases page reservation before timeout

**Parameters:**
- Page pointer

**Returns:** None

**Description:** Enables explicit reservation release for efficient resource management.

## Background Processing

### 9. **NEW: Persistence and Eviction Thread**
**Function Name:** Persistence and Eviction Thread

**Purpose:** Background thread managing cache persistence and memory management

**Parameters:**
- Thread argument (unused)

**Returns:** Thread result

**Description:** **CRITICAL BACKGROUND PROCESS** - Manages both writing dirty pages to disk and evicting least-recently-used pages from cache when memory limits exceeded. Fixed critical race conditions in page persistence logic.

### 10. Page Synchronization
**Function Name:** Sync Page

**Purpose:** Writes single page data to disk with resilient disk-write logic

**Parameters:**
- Page pointer

**Returns:** None

**Description:** **UPDATED** - Added resilient disk-write logic with retry mechanisms to prevent desynchronization between memory and disk.

## Utility Functions

### 11. Denomination Index Conversion
**Function Name:** Get Denomination Index

**Purpose:** Converts signed denomination to array index

**Parameters:**
- Denomination (8-bit integer)

**Returns:** Integer array index

**Description:** Handles negative denomination values by adding DENOMINATION_OFFSET for array indexing.

### 12. Index to Denomination Conversion
**Function Name:** Get Denomination by Index

**Purpose:** Converts array index back to signed denomination

**Parameters:**
- Denomination index (integer)

**Returns:** 8-bit signed integer denomination

**Description:** Reverse conversion from array index to signed denomination value.

## Architectural Improvements

### Memory Usage Revolution
- **On-Demand Loading:** Pages loaded only when accessed, reducing memory from GBs to MBs
- **Intelligent Caching:** LRU-based cache management with configurable limits
- **Bounded Memory:** Maximum memory usage controlled and predictable
- **Cache Efficiency:** Hash table + LRU list for O(1) access and efficient eviction

### Performance Enhancements
- **Bitmap Speed:** Sub-millisecond free coin discovery eliminates I/O bottleneck
- **Cache Utilization:** Frequently accessed pages stay in memory for fast access
- **Background Persistence:** Asynchronous disk writes don't block operations
- **Lock Optimization:** Fine-grained locking reduces contention

### Reliability Improvements
- **Race Condition Fixes:** Fixed critical race conditions in persistence logic
- **Resilient Writes:** Added retry logic for disk write operations
- **Atomic Operations:** All operations maintain data consistency
- **Error Recovery:** Comprehensive error handling and recovery

### Scalability Features
- **Linear Performance:** Performance scales linearly with cache size
- **Concurrent Access:** Multiple threads can access different pages simultaneously
- **Dynamic Allocation:** Memory allocated based on actual usage patterns
- **Load Distribution:** Even distribution of cache operations

## Thread Safety Design

### Multi-Level Locking
- **Global Cache Mutex:** Protects cache data structures during modification
- **Individual Page Mutexes:** Fine-grained locks for page-specific operations
- **Bitmap Mutexes:** Per-denomination bitmap protection
- **Lock Ordering:** Consistent lock ordering prevents deadlocks

### Concurrency Optimization
- **Reader Concurrency:** Multiple readers can access different pages
- **Writer Isolation:** Writers have exclusive access to pages
- **Lock-Free Reads:** Some operations designed for minimal locking
- **Deadlock Prevention:** Careful lock ordering and timeout management

## Integration Points

### System Integration
- **Configuration System:** Database paths, cache sizes, flush frequencies
- **Logging System:** Comprehensive logging of database operations
- **Statistics System:** Performance metrics and operational statistics
- **Network Layer:** Provides data access for all network operations

### Application Integration
- **Command Handlers:** All command handlers use database interface
- **Administrative Tools:** Administrative operations through database layer
- **Healing System:** Integrity verification and repair operations
- **Index Systems:** Locker and crossover indexes built on database layer

## Error Handling and Recovery

### Comprehensive Error Handling
- **File System Errors:** Graceful handling of disk I/O failures
- **Memory Errors:** Safe handling of memory allocation failures
- **Concurrency Errors:** Proper handling of lock contention and timeouts
- **Data Corruption:** Detection and handling of corrupted data

### Recovery Mechanisms
- **Automatic Retry:** Resilient disk operations with automatic retry
- **State Recovery:** System can recover from various failure states
- **Consistency Checking:** Automatic consistency verification
- **Self-Healing:** System can repair minor inconsistencies automatically

This updated database header provides the complete interface for a modern, high-performance database system with revolutionary memory usage optimization, real-time bitmap integration, comprehensive thread safety, and robust error handling, serving as the foundation for all coin storage and retrieval operations in the RAIDA system.