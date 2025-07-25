# Locker Command Handlers (cmd_locker)

## Module Purpose
This module implements locker services for coin storage and trading operations, providing secure coin storage, trade locker management, and marketplace functionality. It includes comprehensive locker operations, trading system integration, and full compatibility with the on-demand page cache system and free pages bitmap.

## Core Functionality

### 1. Store Sum Command (`cmd_store_sum`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Stores coins in a locker using XOR sum verification for batch authentication and atomic storage operations.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (55 bytes)
   - Calculates coin count: (body_size - 50) / 5
   - Each coin requires: 1 byte denomination + 4 bytes serial number

2. **Batch Authentication:**
   - **XOR Sum Calculation:** Computes XOR of all coin authentication numbers
   - For each coin:
     - Loads page using get_page_by_sn_lock
     - XORs coin's 16-byte authentication number into accumulator
     - Unlocks page immediately after use
   - **Sum Verification:** Compares calculated sum with provided sum

3. **Locker Validation:**
   - **Locker Pattern Check:** Validates proposed AN has locker signature (0xffffffff in bytes 12-15)
   - **Security Enforcement:** Ensures only valid locker patterns accepted
   - **Error Handling:** Returns ERROR_INVALID_PAN for invalid patterns

4. **Atomic Storage Process:**
   - **All-or-Nothing:** Only proceeds if all coins authenticate successfully
   - For each authenticated coin:
     - Updates authentication number to locker pattern
     - Sets MFS to current timestamp
     - Marks page as dirty for persistence
     - **Bitmap Update:** Calls update_free_pages_bitmap(den, sn, 0) to mark as not free

5. **Index Management:**
   - **Incremental Update:** Adds coins to locker index using locker_index_add_coins
   - **Performance Optimization:** Avoids full index rebuild
   - **Consistency:** Maintains index synchronization with coin data

**Security Features:**
- **Batch Authentication:** All coins must authenticate before any changes
- **Atomic Operations:** Either all coins stored or operation fails
- **Pattern Validation:** Enforces proper locker identification patterns
- **Tamper Detection:** XOR sum detects any coin substitution attempts

**Used By:** Coin storage operations, locker creation, batch coin management

**Dependencies:** Database layer, locker index system, bitmap management

### 2. Remove Command (`cmd_remove`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Removes coins from a specific locker, transferring them to new ownership with comprehensive validation.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (55 bytes)
   - Calculates coin count: (body_size - 34) / 21
   - Each coin requires: 1 byte denomination + 4 bytes SN + 16 bytes new AN

2. **Locker Verification:**
   - **Index Lookup:** Uses get_coins_from_index to find locker
   - **Existence Check:** Returns failure if locker doesn't exist
   - **Authorization:** Validates requester has access to locker

3. **Coin Ownership Verification:**
   - For each coin to remove:
     - **Index Validation:** Verifies coin exists in locker index
     - **Ownership Check:** Confirms coin is actually in the specified locker
     - **Access Control:** Ensures only authorized removals

4. **Ownership Transfer:**
   - For each verified coin:
     - Loads page using get_page_by_sn_lock
     - **AN Update:** Changes authentication number to new owner's value
     - **Timestamp Update:** Sets MFS to current timestamp
     - **Persistence:** Marks page as dirty for automatic persistence
     - **Status Preservation:** Coin remains in circulation (not freed)

5. **Index Maintenance:**
   - **Incremental Removal:** Removes coins from locker index
   - **Efficiency:** Uses locker_index_remove_coins for batch removal
   - **Consistency:** Maintains perfect index synchronization

**Ownership Model:**
- **Transfer Not Destruction:** Coins transferred to new owner, not destroyed
- **Circulation Preservation:** Coins remain in active circulation
- **Bitmap Consistency:** Bitmap status unchanged (coins still not free)
- **Audit Trail:** MFS timestamp provides ownership change audit

**Used By:** Coin retrieval operations, locker emptying, ownership transfers

**Dependencies:** Database layer, locker index, ownership validation

### 3. Peek Command (`cmd_peek`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Retrieves denomination and serial number information for all coins in a locker without modifying ownership.

**Process:**
1. **Request Validation:**
   - Validates exact request size (34 bytes)
   - Extracts locker authentication number (16 bytes)
   - Ensures request format compliance

2. **Locker Lookup:**
   - **Index Search:** Uses get_coins_from_index to locate locker
   - **Existence Verification:** Returns failure if locker not found
   - **Read-Only Access:** No modifications during peek operation

3. **Data Extraction:**
   - **Response Sizing:** Allocates buffer: num_coins × 5 bytes
   - For each coin in locker:
     - **Denomination:** 1 byte denomination value
     - **Serial Number:** 4 bytes serial number
   - **Efficient Format:** Compact representation for network efficiency

4. **Response Assembly:**
   - **Memory Management:** Allocates exact response size needed
   - **Data Packing:** Packs all coin information efficiently
   - **Success Status:** Returns STATUS_ALL_PASS with complete data

**Read-Only Features:**
- **Non-Destructive:** No changes to locker or coin data
- **Index Access:** Uses index for fast access without database queries
- **Efficient Response:** Minimal data format for network efficiency
- **Instant Response:** No disk I/O required for response

**Used By:** Locker inspection, coin inventory, administrative tools

**Dependencies:** Locker index system, memory management

### 4. Put For Sale Command (`cmd_put_for_sale`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Converts regular coins into a tradeable unit by creating a trade locker with specific trading parameters.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (184 bytes)
   - Calculates coin count: (body_size - 179) / 5
   - Extracts trading parameters and coin list

2. **Batch Authentication:**
   - **XOR Sum Verification:** Computes and verifies XOR sum of all coins
   - For each coin:
     - Loads page and extracts authentication number
     - Accumulates XOR sum for batch verification
     - Unlocks page after data extraction

3. **Trade Locker Validation:**
   - **Coin Type Check:** Validates f_coin_type using is_good_trade_coin_type
   - **Pattern Validation:** Ensures bytes 14-15 are 0xeeee (trade locker signature)
   - **Parameter Validation:** Validates all trading parameters are acceptable

4. **Trade Locker Creation:**
   - **Authentication Success Required:** Only proceeds if all coins authenticate
   - For each coin:
     - Updates authentication number with trade locker pattern
     - **Trading Metadata:** Embeds trading parameters in authentication number
     - Sets MFS to current timestamp
     - Marks page as dirty for persistence
     - **Bitmap Update:** Updates bitmap to mark coin as not free

5. **Trade Index Management:**
   - **Trade Registration:** Adds coins to trade index using trade_locker_index_add_coins
   - **Marketplace Integration:** Makes trade locker discoverable in marketplace
   - **Efficiency:** Uses incremental index updates

**Trading Features:**
- **Multiple Coin Types:** Supports CloudCoin, Bitcoin, Monero trade types
- **Atomic Creation:** Either entire trade locker created or operation fails
- **Marketplace Integration:** Automatically registered in trade marketplace
- **Metadata Embedding:** Trading parameters embedded in coin data

**Used By:** Trading operations, marketplace creation, coin commercialization

**Dependencies:** Database layer, trade index system, coin type validation

### 5. List Lockers For Sale Command (`cmd_list_lockers_for_sale`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Returns available trade lockers for a specific currency type with complete trading information.

**Process:**
1. **Request Validation:**
   - Validates exact request size (20 bytes)
   - Extracts requested coin type and maximum number of results
   - Validates coin type using is_good_trade_coin_type

2. **Trade Index Query:**
   - **Type-Based Search:** Uses load_coins_from_trade_index for specific coin type
   - **Result Limiting:** Respects requested maximum number of results
   - **Efficient Lookup:** Uses optimized index for fast marketplace queries

3. **Value Calculation:**
   - For each trade locker:
     - **Total Value:** Calculates total value using calc_coins_in_trade_locker
     - **Denomination Aggregation:** Sums value across all denominations
     - **Precision Handling:** Maintains accurate value calculations

4. **Response Construction:**
   - **Fixed Format:** Each entry is exactly 29 bytes
     - 16 bytes: Authentication number (locker identifier)
     - 1 byte: Coin type identifier
     - 8 bytes: Total coin value (big-endian)
     - 4 bytes: Price information (from bytes 9-12 of AN)
   - **Network Optimization:** Efficient binary format for network transmission

**Marketplace Features:**
- **Currency Filtering:** Results filtered by specific cryptocurrency type
- **Value Transparency:** Complete value information provided
- **Price Discovery:** Current pricing information included
- **Bulk Queries:** Supports queries for multiple trade lockers

**Used By:** Trading interfaces, marketplace browsing, price discovery

**Dependencies:** Trade index system, value calculation, network byte order

### 6. Buy Command (`cmd_buy`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Executes purchase of a trade locker, transferring ownership from seller to buyer with complete validation.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (96 bytes)
   - Extracts buyer locker key, coin type, total value, and price
   - Validates all purchase parameters

2. **Trade Locker Lookup:**
   - **Exact Match Search:** Uses get_entry_from_trade_index with precise criteria
   - **Multi-Parameter Matching:** Matches coin type, total value, and price exactly
   - **Availability Verification:** Ensures trade locker still available

3. **Ownership Transfer Process:**
   - **Bulk Transfer:** Transfers all coins from seller to buyer
   - For each coin in trade locker:
     - Loads page using get_page_by_sn_lock
     - **AN Update:** Changes authentication number to buyer's locker key
     - **Timestamp:** Updates MFS to current timestamp
     - **Persistence:** Marks page as dirty for automatic persistence

4. **Index Management:**
   - **Trade Index Removal:** Removes from trade index using trade_locker_index_remove_coins
   - **Locker Index Addition:** Adds to buyer's locker using locker_index_add_coins
   - **Atomic Transfer:** Ensures consistent index state throughout transfer

**Purchase Features:**
- **Exact Matching:** Precise matching of trade locker criteria
- **Atomic Transfer:** Either complete purchase succeeds or fails entirely
- **Index Consistency:** Maintains perfect index synchronization
- **Immediate Ownership:** Coins immediately available to buyer

**Used By:** Trading execution, marketplace purchases, automated trading

**Dependencies:** Trade index, locker index, database layer

### 7. Remove Trade Locker Command (`cmd_remove_trade_locker`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Removes a trade locker from sale, freeing all associated coins back to the available pool.

**Process:**
1. **Trade Locker Lookup:**
   - Uses get_coins_from_trade_index to locate trade locker
   - Returns failure if trade locker not found
   - Validates access permissions

2. **Coin Liberation:**
   - For each coin in trade locker:
     - Loads page using get_page_by_sn_lock
     - **Free Status:** Sets MFS to 0 (marks coin as free)
     - **Persistence:** Marks page as dirty for automatic persistence
     - **Bitmap Update:** Calls update_free_pages_bitmap(den, sn, 1) to mark as free

3. **Index Cleanup:**
   - **Trade Index Removal:** Removes from trade index completely
   - **Bulk Operation:** Uses trade_locker_index_remove_coins for efficiency
   - **Consistency:** Maintains index synchronization

**Liberation Features:**
- **Complete Removal:** Trade locker completely removed from marketplace
- **Coin Liberation:** All coins returned to available pool
- **Bitmap Synchronization:** Real-time bitmap updates for instant availability
- **Index Cleanup:** Complete cleanup of all index references

**Used By:** Trade cancellation, locker management, administrative cleanup

**Dependencies:** Trade index, database layer, bitmap management

### 8. Peek Trade Locker Command (`cmd_peek_trade_locker`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Inspects contents of a trade locker without modifying its state or ownership.

**Process:**
1. **Trade Locker Lookup:**
   - Uses get_coins_from_trade_index to locate trade locker
   - Returns failure if trade locker not found
   - Read-only access to trade data

2. **Content Extraction:**
   - **Response Sizing:** Allocates buffer: num_coins × 5 bytes
   - For each coin in trade locker:
     - Returns denomination (1 byte) and serial number (4 bytes)
   - **Efficient Format:** Compact binary representation

**Inspection Features:**
- **Non-Destructive:** No changes to trade locker or coin state
- **Complete Visibility:** Shows all coins in trade locker
- **Fast Response:** Index-based lookup without database queries
- **Efficient Format:** Minimal network overhead

**Used By:** Trade inspection, due diligence, administrative tools

**Dependencies:** Trade index system, memory management

### 9. Store Multiple Sum Command (`cmd_store_multiple_sum`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Stores multiple lockers in a single request for bulk operations with individual success/failure tracking.

**Process:**
1. **Request Format:**
   - **Header:** 1 byte number of lockers
   - **Per Locker:** 2 bytes coin count + coin data + sum + locker AN
   - **Variable Length:** Dynamic request size based on locker count and sizes

2. **Bulk Processing:**
   - For each locker in request:
     - **Individual Validation:** Validates coin count and data format
     - **XOR Sum Verification:** Computes and verifies sum for each locker
     - **Independent Processing:** Each locker processed independently

3. **Per-Locker Operations:**
   - **Authentication:** XOR sum verification for each locker
   - **Storage:** Updates all coins with locker authentication number
   - **Bitmap Updates:** Updates bitmap for each stored coin
   - **Index Updates:** Adds coins to locker index

4. **Response Generation:**
   - **Per-Locker Status:** 1 byte per locker indicating success/failure
   - **Bulk Results:** Enables partial success scenarios
   - **Status Classification:** Returns appropriate overall status

**Bulk Features:**
- **High Throughput:** Multiple lockers processed in single request
- **Individual Tracking:** Success/failure tracked per locker
- **Partial Success:** Some lockers can succeed while others fail
- **Efficient Processing:** Optimized for bulk operations

**Used By:** Bulk locker creation, high-volume operations, automated systems

**Dependencies:** Database layer, locker index, bitmap management

## Locker System Architecture

### Authentication Number Patterns
- **Regular Locker:** Bytes 12-15 = 0xffffffff
- **Trade Locker:** Bytes 14-15 = 0xeeee, byte 13 = coin type
- **Pattern Enforcement:** Strict validation of locker identification patterns
- **Security:** Prevents invalid locker creation

### Index Integration
- **Incremental Updates:** Uses incremental index updates instead of rebuilds
- **Dual Indexes:** Maintains both regular and trade locker indexes
- **Performance:** Fast locker lookup without database queries
- **Consistency:** Perfect synchronization between indexes and coin data

### Bitmap Integration
- **Real-Time Updates:** Bitmap updated immediately with locker operations
- **Status Tracking:** Tracks which coins are in lockers vs. available
- **Performance:** Enables instant availability queries
- **Consistency:** Maintains perfect sync with actual coin data

## Trading System Features

### Supported Coin Types
- **CloudCoin (SALE_TYPE_CC):** Native CloudCoin trading
- **Bitcoin (SALE_TYPE_BTC):** Bitcoin integration trading
- **Monero (SALE_TYPE_XMR):** Monero privacy coin trading
- **Extensible:** Architecture supports additional coin types

### Trading Metadata
- **Price Information:** Embedded in authentication number bytes 9-12
- **Coin Type:** Stored in authentication number byte 13
- **Trade Signature:** Bytes 14-15 identify trade lockers (0xeeee)
- **Value Calculation:** Automatic calculation of total locker value

### Marketplace Operations
- **Listing:** Automatic marketplace registration for trade lockers
- **Discovery:** Efficient search by coin type and criteria
- **Execution:** Atomic purchase operations with ownership transfer
- **Removal:** Clean removal from marketplace with coin liberation

## Performance Optimizations

### Database Integration
- **On-Demand Loading:** Pages loaded only when needed for locker operations
- **Efficient Caching:** Benefits from database page cache for active lockers
- **Minimal I/O:** Optimized to minimize disk access
- **Batch Operations:** Efficient handling of multi-coin operations

### Memory Management
- **Dynamic Allocation:** Memory allocated based on actual locker sizes
- **Efficient Copying:** Optimized data copying and buffer management
- **Resource Cleanup:** Proper cleanup prevents memory leaks
- **Buffer Validation:** Comprehensive buffer overflow prevention

### Index Performance
- **Fast Lookups:** Index-based operations avoid database queries
- **Incremental Updates:** Efficient index maintenance without rebuilds
- **Memory Efficient:** Indexes use minimal memory for maximum performance
- **Cache Friendly:** Index operations optimized for CPU cache efficiency

## Security and Validation

### Authentication Security
- **XOR Sum Verification:** Cryptographic verification of coin batches
- **Pattern Validation:** Strict enforcement of locker identification patterns
- **Ownership Verification:** Comprehensive ownership validation before operations
- **Tamper Detection:** Batch authentication detects coin substitution attempts

### Trading Security
- **Exact Matching:** Precise matching of trade locker criteria prevents manipulation
- **Atomic Operations:** All trading operations are atomic to prevent partial states
- **Value Validation:** Comprehensive validation of trade locker values
- **Type Enforcement:** Strict enforcement of supported coin types

### Data Integrity
- **Consistent Updates:** All operations maintain data consistency
- **Index Synchronization:** Perfect synchronization between indexes and coin data
- **Bitmap Consistency:** Real-time bitmap updates maintain accuracy
- **Audit Trail:** MFS timestamps provide complete audit trail

## Error Handling and Recovery

### Request Validation
- **Size Validation:** All request sizes strictly validated
- **Format Checking:** Request format validation prevents errors
- **Parameter Validation:** All parameters validated for correctness
- **Range Checking:** Numeric parameters validated against acceptable ranges

### Operation Safety
- **Atomic Operations:** All locker operations are atomic
- **Rollback Capability:** Failed operations leave consistent state
- **Error Propagation:** Clear error codes and messages
- **State Verification:** Post-operation state verification

### System Recovery
- **Data Consistency:** Operations maintain data consistency during failures
- **Index Recovery:** Indexes can be rebuilt from coin data if needed
- **Bitmap Recovery:** Bitmap can be reconstructed from actual coin data
- **Clean Failure:** Failed operations leave system in clean state

## Monitoring and Diagnostics

### Operation Metrics
- **Performance Tracking:** Response times and throughput measurement
- **Success Rates:** Success/failure ratios for all operations
- **Resource Usage:** Memory and CPU usage monitoring
- **Error Classification:** Detailed error tracking and analysis

### System Health
- **Locker Statistics:** Real-time locker utilization monitoring
- **Trade Activity:** Trading volume and activity monitoring
- **Index Health:** Index consistency and performance monitoring
- **Cache Performance:** Database cache hit rates for locker operations

### Administrative Reporting
- **Usage Analytics:** Comprehensive locker usage statistics
- **Trading Reports:** Trading volume and marketplace activity
- **Performance Analysis:** Detailed performance breakdowns
- **Error Analysis:** Error pattern analysis and reporting

## Integration Points

### Database Layer Integration
- **On-Demand Pages:** Full integration with on-demand page cache
- **Thread Safety:** Proper page locking and unlocking
- **Persistence:** Automatic persistence of locker modifications
- **Cache Efficiency:** Optimized for database cache utilization

### Index System Integration
- **Dual Indexes:** Integration with both regular and trade locker indexes
- **Incremental Updates:** Efficient index maintenance
- **Fast Lookups:** Index-based operations for performance
- **Consistency Maintenance:** Perfect synchronization with coin data

### Bitmap System Integration
- **Real-Time Updates:** Immediate bitmap updates with coin status changes
- **Consistency Guarantee:** Perfect sync between bitmap and coin data
- **Performance Benefits:** Instant availability queries
- **Memory Efficiency:** Minimal overhead for maximum performance

## Administrative Features

### Locker Management
- **Bulk Operations:** Support for high-volume locker operations
- **Administrative Access:** Administrative tools can manage all lockers
- **Audit Capabilities:** Complete audit trail for all locker operations
- **Recovery Tools:** Tools for recovering from various failure scenarios

### Trading Administration
- **Marketplace Management:** Administrative control over trade marketplace
- **Fee Management:** Support for trading fees and commissions
- **Dispute Resolution:** Tools for handling trading disputes
- **Analytics:** Comprehensive trading analytics and reporting

### System Maintenance
- **Index Maintenance:** Tools for index verification and rebuilding
- **Performance Tuning:** Monitoring and optimization tools
- **Data Validation:** Comprehensive data validation and verification
- **Backup Integration:** Compatible with backup and recovery systems

## Future Extensibility

### Protocol Evolution
- **New Coin Types:** Architecture supports additional cryptocurrency types
- **Enhanced Trading:** Support for advanced trading features
- **Improved Security:** Enhanced security and validation mechanisms
- **Performance Optimization:** Continuous performance improvements

### Feature Expansion
- **Smart Contracts:** Potential integration with smart contract systems
- **Automated Trading:** Support for automated trading systems
- **Advanced Analytics:** Enhanced analytics and reporting capabilities
- **API Evolution:** Extended API for enhanced functionality

### Integration Opportunities
- **External Exchanges:** Integration with external cryptocurrency exchanges
- **Payment Processors:** Integration with payment processing systems
- **Banking Systems:** Potential integration with traditional banking
- **Regulatory Compliance:** Enhanced compliance and reporting features

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache and coin data management
- **Index Systems:** Regular and trade locker index management
- **Bitmap System:** Real-time free coin tracking
- **Value Calculation:** Coin value computation for trading
- **Network Layer:** Communication protocols and data handling

### Used By
- **Client Applications:** Primary interface for locker operations
- **Trading Systems:** Marketplace and trading functionality
- **Administrative Tools:** Locker and trading system management
- **Automated Systems:** High-volume and automated operations

### Cross-File Dependencies
- **Database Module:** Page access and coin data management
- **Locker Index Module:** Locker indexing and lookup functionality
- **Utilities Module:** Value calculations and data conversion
- **Configuration Module:** System parameters and operational settings
- **Statistics Module:** Performance metrics and operational analytics

This locker command module provides comprehensive coin storage and trading functionality with advanced features including batch operations, marketplace integration, real-time bitmap synchronization, and robust error handling, enabling secure, efficient, and scalable locker operations for both storage and trading use cases.