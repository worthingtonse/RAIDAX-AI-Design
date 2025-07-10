# Specification:Legacy Common Header Interface (common.h)

## 1. Module Purpose
This header file defines the interface for legacy CloudCoin system operations and database interactions within the RAIDAX cryptocurrency system, part of the CloudCoinConsortium project. It provides function declarations for legacy coin detection, deletion, value calculation, and database operations with both deprecated and secure SQL interfaces.

## 2. System Architecture Overview

### 2.1 Interface Components/functionality
- **Legacy Coin Operations**: Version 1 coin system detection, deletion, and calculation
- **Database Management**: Connection initialization and cleanup operations
- **Deprecated SQL Interface**: Legacy SQL functions marked for removal
- **Secure SQL Interface**: Prepared statement-based SQL operations
- **Configuration Management**: Database configuration path definitions

### 2.2 Security Migration Strategy
- **Deprecation Model**: Legacy insecure functions marked for removal
- **Secure Replacement**: New prepared statement functions to prevent SQL injection
- **Migration Path**: Clear separation between deprecated and secure interfaces
- **Documentation**: Explicit warnings about deprecated function security risks

## 3. Header File Structure

### 3.1 Include Guards
- **Header Protection**: LEGACY_COMMON_H identifier for inclusion protection
- **Conditional Compilation**: Prevents multiple header inclusion
- **Naming Convention**: Legacy prefix indicates version 1 compatibility layer

### 3.2 Required Dependencies
- **Standard Integer Types**: Required for 32-bit unsigned integer definitions
- **Type Compatibility**: Cross-platform integer type support
- **Minimal Dependencies**: Only essential system type includes

### 3.3 Interface Categories
- **Legacy Operations**: Version 1 coin system compatibility functions
- **Database Operations**: Connection management and SQL execution
- **Security Functions**: Secure SQL operations with injection prevention
- **Configuration**: System configuration path definitions

## 4. Legacy Coin Operation Interface

### 4.1 Legacy Detection Function Interface
**Function Purpose**: Provides interface for detecting legacy version 1 coins.

**Parameter Requirements**:
- **Input Buffer**: Pointer to unsigned character data containing coin information
- **Buffer Length**: Integer specifying input buffer size
- **Output Parameters**: Multiple integer pointers for detection results
- **Result Buffer**: Pointer to unsigned character buffer for detection output

**Return Specification**:
- **Return Type**: Integer status code
- **Success Indication**: Zero or positive value for successful detection
- **Failure Indication**: Negative value for detection failure
- **Usage Context**: Legacy coin system compatibility

### 4.2 Legacy Deletion Function Interface
**Function Purpose**: Provides interface for deleting legacy version 1 coins.

**Parameter Requirements**:
- **First Buffer**: Pointer to unsigned character data for coin identification
- **Second Buffer**: Pointer to unsigned character data for deletion parameters
- **Length Parameter**: Integer specifying buffer size or operation parameters

**Return Specification**:
- **Return Type**: Integer status code
- **Success Value**: Zero indicates successful deletion
- **Error Value**: Negative value indicates deletion failure
- **Operation Context**: Legacy coin removal from system

### 4.3 Legacy Total Calculation Interface
**Function Purpose**: Provides interface for calculating total value of legacy coins.

**Parameter Requirements**:
- **Coin Buffer**: Pointer to unsigned character data containing coin information
- **Count Parameter**: Integer specifying number of coins or buffer size

**Return Specification**:
- **Return Type**: 32-bit unsigned integer value
- **Return Purpose**: Total calculated value of legacy coins
- **Value Format**: Standard monetary unit representation
- **Calculation Context**: Legacy coin valuation system

## 5. Database Management Interface

### 5.1 Database Initialization Interface
**Function Purpose**: Provides interface for establishing legacy database connection.

**Parameter Requirements**:
- **No Parameters**: Function requires no input parameters
- **Configuration Source**: Uses predefined configuration paths

**Return Specification**:
- **Return Type**: Integer status code
- **Success Value**: Zero indicates successful initialization
- **Failure Value**: Negative value indicates initialization failure
- **Connection Context**: Establishes database connectivity for legacy operations

### 5.2 Database Cleanup Interface
**Function Purpose**: Provides interface for properly closing legacy database connection.

**Parameter Requirements**:
- **No Parameters**: Function requires no input parameters
- **Resource Management**: Handles all internal cleanup operations

**Return Specification**:
- **Return Type**: No return value
- **Operation Guarantee**: Function ensures proper resource cleanup
- **Connection Context**: Terminates database connectivity cleanly

## 6. Deprecated SQL Interface (Legacy)

### 6.1 Single Field Retrieval Interface (Deprecated)
**Function Purpose**: Provides legacy interface for single database field retrieval.

**Security Warning**: Function vulnerable to SQL injection attacks and scheduled for removal.

**Parameter Requirements**:
- **SQL Query**: Character pointer to SQL query string
- **Output Buffer**: Pointer to unsigned character buffer for result
- **Buffer Length**: Integer specifying maximum buffer size

**Return Specification**:
- **Return Type**: Integer status code
- **Success Value**: Zero indicates successful query execution
- **Failure Value**: Negative value indicates query failure
- **Security Risk**: Direct SQL string execution without parameter binding

**Deprecation Status**: 
- **Usage Restriction**: Do not use in new development
- **Replacement Available**: Use secure prepared statement functions instead
- **Removal Timeline**: Function will be removed in future versions

### 6.2 SQL Execution Interface (Deprecated)
**Function Purpose**: Provides legacy interface for general SQL command execution.

**Security Warning**: Function vulnerable to SQL injection attacks and scheduled for removal.

**Parameter Requirements**:
- **SQL Command**: Character pointer to SQL command string

**Return Specification**:
- **Return Type**: Integer status code
- **Success Value**: Zero indicates successful command execution
- **Failure Value**: Negative value indicates command failure
- **Security Risk**: Direct SQL string execution without validation

**Deprecation Status**:
- **Usage Restriction**: Prohibited for new development
- **Security Vulnerability**: SQL injection vulnerability present
- **Migration Required**: Existing usage must migrate to secure functions

## 7. Secure SQL Interface (Current)

### 7.1 Secure Single Field Retrieval Interface
**Function Purpose**: Provides secure interface for single database field retrieval using prepared statements.

**Security Features**: Prevents SQL injection through parameter binding and query template separation.

**Parameter Requirements**:
- **SQL Template**: Constant character pointer to SQL query with placeholder
- **Parameter Value**: 32-bit unsigned integer for placeholder binding
- **Output Buffer**: Pointer to unsigned character buffer for result storage
- **Buffer Length**: Integer specifying maximum buffer capacity

**Template Format**:
- **Placeholder Usage**: Question mark character as parameter placeholder
- **Single Parameter**: Template supports one parameter binding
- **Query Type**: SELECT queries returning single field from single row

**Return Specification**:
- **Success Value**: Zero indicates successful query execution and data retrieval
- **Failure Value**: Negative one indicates query failure or no results
- **Security Guarantee**: No SQL injection vulnerability through parameter binding

### 7.2 Secure SQL Execution Interface
**Function Purpose**: Provides secure interface for UPDATE and INSERT operations using prepared statements.

**Security Features**: Prevents SQL injection through parameter binding and command template separation.

**Parameter Requirements**:
- **SQL Template**: Constant character pointer to SQL command with placeholder
- **Parameter Value**: 32-bit unsigned integer for placeholder binding

**Template Format**:
- **Placeholder Usage**: Question mark character as parameter placeholder
- **Single Parameter**: Template supports one parameter binding
- **Command Types**: UPDATE and INSERT commands that do not return result sets

**Return Specification**:
- **Success Value**: Zero indicates successful command execution
- **Failure Value**: Negative one indicates command execution failure
- **Security Guarantee**: No SQL injection vulnerability through parameter separation

**Operation Context**: Suitable for data modification operations without result retrieval

## 8. Configuration Management Interface

### 8.1 Database Configuration Paths
**Configuration Purpose**: Defines standard locations for database configuration files.

**Primary Configuration Path**:
- **Location**: User home directory configuration file
- **Usage**: Primary configuration file location
- **Access Context**: User-specific configuration settings

**Secondary Configuration Path**:
- **Location**: Web server directory configuration file
- **Usage**: Alternative configuration file location
- **Access Context**: System-wide configuration settings

**Path Selection Strategy**:
- **Priority Order**: Primary path checked before secondary path
- **Fallback Mechanism**: Secondary path used if primary unavailable
- **Configuration Format**: Both paths use identical configuration file format

## 9. Security Implementation Requirements

### 9.1 SQL Injection Prevention
- **Prepared Statements**: All secure functions use prepared statement mechanisms
- **Parameter Binding**: Separate SQL templates from parameter data
- **Input Validation**: Validate all parameter inputs before binding
- **Template Verification**: Ensure SQL templates contain only expected placeholders

## 10. Documentation and Usage Guidelines

### 10.1 Function Usage Documentation
- **Secure Function Guidance**: Clear documentation on secure function usage
- **Parameter Examples**: Examples of proper parameter formatting
- **Error Handling**: Guidance on proper error condition handling
- **Performance Considerations**: Optimization guidance for database operations

### 10.2 Security Documentation
- **Threat Model**: Documentation of relevant security threats and mitigations
- **Compliance Requirements**: Security compliance requirements for database operations

This specification provides complete interface definition guidance for the RAIDAX legacy common header file while emphasizing the critical security migration from vulnerable SQL functions to secure prepared statement implementations essential for cryptocurrency system security.