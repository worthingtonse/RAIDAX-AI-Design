# Merkle Tree Integrity System Header (integrity.h)

## Module Purpose
This header defines the interface and data structures for the Merkle tree-based integrity verification system. It provides type definitions, function declarations, and constants needed for cryptographic data integrity checking across the RAIDA network. The interface supports hierarchical hash trees for efficient verification and network synchronization.

## Core Type Definitions

### 1. Hash Size Constant
**Definition:** `HASH_SIZE` (32 bytes)
**Purpose:** Defines the size of SHA-256 hash values used throughout the integrity system
**Usage:** All hash buffers and operations use this constant for consistency
**Security:** Ensures consistent cryptographic hash size across all operations

### 2. Merkle Node Structure (`merkle_node_s`)
**Purpose:** Represents a single node in the Merkle tree with hash value and child pointers
**Fields:**
- `hash`: 32-byte array containing SHA-256 hash value
- `left`: Pointer to left child node (null for leaf nodes)
- `right`: Pointer to right child node (null for leaf nodes)

**Usage:** Building blocks for constructing hierarchical hash trees
**Memory:** Dynamically allocated and linked into tree structure

### 3. Merkle Tree Structure (`merkle_tree_t`)
**Purpose:** Represents a complete Merkle tree for a single denomination
**Fields:**
- `root`: Pointer to root node of the tree
- `leaf_count`: Number of leaf nodes (pages) in the tree

**Usage:** Container for complete denomination integrity information
**Lifecycle:** Created by tree building, cached for access, freed when updated

## Public Function Interface

### 1. System Initialization (`init_integrity_system`)
**Parameters:**
- None

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Initializes the integrity system and starts background synchronization thread

**Usage:** Called once during system startup to enable integrity verification
**Dependencies:** Threading system, configuration system
**Error Handling:** Returns failure code if thread creation fails

### 2. Background Synchronization Thread (`merkle_sync_thread`)
**Parameters:**
- Thread argument pointer (unused)

**Returns:** Thread result (null)

**Purpose:** Main function for background thread that periodically builds Merkle trees

**Usage:** Executed continuously in background thread
**Threading:** Designed for long-running background operation
**Lifecycle:** Runs until system shutdown signal received

### 3. Root Hash Retrieval (`get_merkle_root`)
**Parameters:**
- Denomination identifier (1 byte signed integer)
- Output buffer for root hash (32 bytes)

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Retrieves the root hash of a Merkle tree for network integrity verification

**Usage:** Primary interface for network synchronization and integrity checking
**Thread Safety:** Safe for concurrent access from multiple threads
**Error Conditions:** Returns failure if tree doesn't exist or isn't built

### 4. Node Hash Retrieval (`get_merkle_node`)
**Parameters:**
- Denomination identifier (1 byte signed integer)
- Tree level (4 bytes unsigned integer, 0 = leaf level)
- Node index within level (4 bytes unsigned integer)
- Output buffer for node hash (32 bytes)

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Retrieves a specific node's hash for detailed integrity verification

**Usage:** Used during synchronization to efficiently find inconsistencies
**Tree Navigation:** Level 0 = leaves, higher levels = internal nodes
**Bounds Checking:** Validates level and index parameters

## Internal Function Interface

### 1. Tree Construction (`build_merkle_tree_for_denomination`)
**Parameters:**
- Denomination identifier (1 byte signed integer)

**Returns:** Merkle tree structure pointer or null on failure

**Purpose:** Builds a complete Merkle tree by hashing all pages for a denomination

**Usage:** Called by background thread to create updated trees
**Memory Management:** Allocates memory for complete tree structure
**Error Handling:** Returns null on memory allocation or I/O failures

### 2. Tree Memory Management (`free_merkle_tree`)
**Parameters:**
- Merkle tree structure pointer

**Returns:** None

**Purpose:** Frees all memory associated with a Merkle tree including all nodes

**Usage:** Called when replacing old trees with new versions
**Memory Safety:** Handles null pointers gracefully
**Cleanup:** Ensures complete memory deallocation

## Data Structure Relationships

### Tree Hierarchy
- **Root Node:** Single node representing entire denomination
- **Internal Nodes:** Intermediate nodes combining child hashes
- **Leaf Nodes:** Bottom level nodes representing individual pages
- **Binary Structure:** Each internal node has at most 2 children

### Hash Relationships
- **Leaf Hashes:** SHA-256 of individual page data
- **Internal Hashes:** SHA-256 of concatenated child hashes
- **Root Hash:** Final hash representing entire denomination state
- **Chain Dependency:** Changes propagate up through hash chain

### Cache Organization
- **Per-Denomination:** One tree cached per denomination
- **Thread-Safe Access:** Mutex-protected cache operations
- **Atomic Updates:** Complete tree replacement during updates
- **Memory Bounded:** Fixed number of cached trees

## Memory Management

### Allocation Strategy
- **Dynamic Allocation:** Tree nodes allocated as needed
- **Hierarchical Structure:** Nodes linked into tree hierarchy
- **Cache Management:** Complete trees cached for access
- **Automatic Cleanup:** Background thread manages memory

### Deallocation Process
- **Recursive Cleanup:** Frees all nodes in tree hierarchy
- **Safe Null Handling:** Graceful handling of null pointers
- **Memory Leak Prevention:** Complete deallocation of all resources
- **Thread Safety:** Safe concurrent access during cleanup

## Security Considerations

### Cryptographic Properties
- **SHA-256 Security:** Cryptographically secure hash function
- **Collision Resistance:** Computationally infeasible to generate collisions
- **One-Way Function:** Cannot reverse-engineer data from hashes
- **Tamper Detection:** Any data change affects entire hash chain

### Integrity Verification
- **Complete Verification:** Can verify entire denomination state
- **Partial Verification:** Can verify subsets without full data
- **Efficient Comparison:** Rapid identification of differences
- **Cryptographic Proof:** Provides cryptographic evidence of integrity

### Thread Safety
- **Concurrent Access:** Safe access from multiple threads
- **Cache Synchronization:** Mutex-protected cache operations
- **Atomic Operations:** Tree updates are atomic
- **Resource Protection:** Prevents concurrent modification

## Integration Requirements

### Database Integration
- **Page Data Access:** Requires access to page files for tree construction
- **Denomination Support:** Works with all supported denominations
- **File System Coordination:** Coordinates with database file organization
- **Independence:** Operates independently of page cache system

### Network Protocol Integration
- **Command Handlers:** Provides hash data for network requests
- **Synchronization Protocol:** Supports integrity verification protocol
- **Error Reporting:** Consistent error codes for network operations
- **Thread Safety:** Safe concurrent access for network operations

### Configuration Integration
- **Timing Parameters:** Configurable rebuild frequency
- **Path Configuration:** Uses configured data directories
- **Resource Limits:** Respects memory and CPU constraints
- **Logging Integration:** Integrated with system logging

## Error Handling

### Error Codes
- **Success (0):** Operation completed successfully
- **Failure (-1):** Operation failed due to various conditions
- **Not Found:** Tree or node not found
- **Invalid Parameters:** Parameters out of acceptable range

### Error Conditions
- **Memory Allocation:** Insufficient memory for tree construction
- **File I/O Errors:** Cannot read page files for tree building
- **Threading Errors:** Background thread creation or management failures
- **Parameter Validation:** Invalid denomination, level, or index values

### Recovery Mechanisms
- **Graceful Degradation:** System continues operation despite failures
- **Resource Cleanup:** Proper cleanup on error conditions
- **Error Logging:** Detailed error reporting for debugging
- **State Consistency:** Maintains consistent state on failures

## Dependencies

### Required Headers
- **Threading Library:** For background thread operations
- **Database Types:** For denomination constants and indexing
- **Standard Libraries:** For memory allocation and basic types
- **Cryptographic Libraries:** For SHA-256 hash operations

### Required Constants
- `TOTAL_DENOMINATIONS`: Number of supported denominations
- `HASH_SIZE`: Size of SHA-256 hash (32 bytes)
- Database constants for page organization
- Configuration constants for timing and paths

### External Dependencies
- **OpenSSL or equivalent:** For SHA-256 cryptographic operations
- **POSIX threads:** For background thread management
- **File system:** For reading page data files
- **Configuration system:** For operational parameters

## Usage Patterns

### Initialization Sequence
1. Call `init_integrity_system()` during system startup
2. Background thread automatically begins tree construction
3. Trees become available for integrity verification
4. System ready for network synchronization operations

### Integrity Verification
1. Call `get_merkle_root()` to get denomination root hash
2. Compare with root hashes from other servers
3. If different, use `get_merkle_node()` to find differences
4. Identify specific pages that need synchronization

### Memory Management
1. Trees automatically built and cached by background thread
2. Old trees automatically freed when replaced
3. System handles all memory management automatically
4. No manual memory management required by users

This integrity header provides the complete interface for cryptographic data integrity verification in the RAIDA network, supporting efficient verification and network synchronization while maintaining security and performance.