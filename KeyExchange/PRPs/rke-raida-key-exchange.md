name: "RKE (RAIDA Key Exchange) - Quantum-Safe Binary Protocol Implementation"
description: |

## Purpose
Implement a complete RKE (RAIDA Key Exchange) protocol in C as part of the RAIDAX system. This quantum-safe key exchange protocol splits cryptographic keys across multiple independent RAIDA nodes, providing security without relying on public-key cryptography like Diffie-Hellman.

## Core Principles
1. **Context is King**: Include ALL necessary documentation, examples, and caveats
2. **Validation Loops**: Provide executable tests/lints the AI can run and fix
3. **Information Dense**: Use keywords and patterns from the codebase
4. **Progressive Success**: Start simple, validate, then enhance
5. **Global rules**: Be sure to follow all rules in CLAUDE.md

---

## Goal
Implement a complete RKE (RAIDA Key Exchange) system that provides quantum-safe key exchange through the splitting and distribution of cryptographic keys across a redundant array of independent data agents (RAIDA). The implementation must integrate seamlessly with the existing RAIDAX protocol infrastructure and follow established C coding patterns.

## Why
- **Quantum-Safe Security**: RKE provides protection against quantum computing attacks by avoiding public-key cryptography
- **Decentralized Trust**: Keys are split across multiple independent RAIDA nodes, eliminating single points of failure
- **RAIDAX Integration**: Essential component for secure communication within the RAIDAX ecosystem
- **Future-Proof**: Protects against both current and emerging cryptographic threats

## What
Implement a binary protocol for RAIDA Key Exchange with the following user-visible behavior:
- Generate quantum-safe cryptographic keys
- Split keys into fragments distributed across RAIDA nodes
- Exchange key fragments securely between parties
- Reconstruct complete keys for encryption/decryption
- Authenticate parties without exposing key material
- Store and retrieve keys persistently

### Success Criteria
- [ ] Key generation produces cryptographically secure keys
- [ ] Key splitting algorithm distributes fragments across RAIDA nodes
- [ ] Binary protocol commands handle key exchange operations
- [ ] Keys can be reconstructed from sufficient fragments
- [ ] Integration with existing crypto system (crypt_ctr)
- [ ] File storage follows existing patterns (coins/, Keys/)
- [ ] All operations follow RAIDAX conn_info_t pattern
- [ ] Error handling and logging match existing conventions
- [ ] Memory management prevents leaks and buffer overflows
- [ ] Unit tests validate all critical functionality

## All Needed Context

### Documentation & References (list all context needed to implement the feature)
```yaml
# MUST READ - Include these in your context window
- url: https://www.linkedin.com/pulse/raida-key-exchange-quantum-safe-sean-worthington
  why: Core understanding of RKE quantum-safe principles and RAIDA architecture
  
- file: examples/cmd_key_exechange.c
  why: Existing key exchange patterns, crypto integration, binary protocol handling
  
- file: examples/cmd_rpc.c
  why: Command structure patterns, error handling, file I/O conventions
  
- file: CLAUDE.md
  why: C coding conventions, security priorities, file organization rules
  
- doc: https://en.wikipedia.org/wiki/Key_exchange
  section: Cryptographic key exchange fundamentals
  critical: Understanding of secure key establishment principles

- docfile: INITIAL.md
  why: Feature requirements and constraints (code must not go public)
```

### Current Codebase tree (run `tree` in the root of the project) to get an overview of the codebase
```bash
KeyExchange/
├── CLAUDE.md
├── INITIAL.md
├── PRPs/
│   └── templates/
│       └── prp_base.md
└── examples/
    ├── cmd_key_exechange.c
    └── cmd_rpc.c
```

### Desired Codebase tree with files to be added and responsibility of file
```bash
KeyExchange/
├── CLAUDE.md
├── INITIAL.md
├── Makefile                     # Build system for C compilation
├── src/
│   ├── rke/
│   │   ├── rke.h               # RKE data structures and function prototypes
│   │   ├── rke_core.c          # Key generation and splitting algorithms
│   │   ├── rke_protocol.c      # Binary protocol command implementations
│   │   ├── rke_storage.c       # Key fragment storage and retrieval
│   │   └── rke_crypto.c        # Cryptographic operations integration
│   └── common/
│       ├── protocol.h          # Shared protocol definitions
│       ├── log.h              # Logging macros and functions
│       └── utils.h            # Utility functions
├── tests/
│   ├── test_rke_core.c        # Unit tests for key generation/splitting
│   ├── test_rke_protocol.c    # Unit tests for protocol commands
│   └── test_rke_integration.c # Integration tests
├── PRPs/
│   └── templates/
│       └── prp_base.md
└── examples/
    ├── cmd_key_exechange.c
    └── cmd_rpc.c
```

### Known Gotchas of our codebase & Library Quirks
```c
// CRITICAL: All command functions must use conn_info_t *ci pattern
void cmd_rke_*(conn_info_t *ci);

// CRITICAL: Always validate packet size before processing
if (ci->body_size != expected_size) {
    error("Invalid command length: %d. Need %d", ci->body_size, expected_size);
    ci->command_status = ERROR_INVALID_PACKET_LENGTH;
    return;
}

// CRITICAL: Memory allocation pattern - always check for NULL
ci->output = (unsigned char *) malloc(size);
if (ci->output == NULL) {
    error("Can't alloc buffer for the response");
    ci->command_status = ERROR_MEMORY_ALLOC;
    return;
}

// CRITICAL: Use existing crypto function crypt_ctr for encryption
crypt_ctr(key, data, length, nonce);

// CRITICAL: File paths must use config.cwd as base
sprintf(path, "%s/Keys/%s", config.cwd, filename);

// CRITICAL: Page locking required for thread safety
page = get_page_by_sn_lock(den, sn);
// ... use page ...
unlock_page(page);

// CRITICAL: Always set command status before returning
ci->command_status = STATUS_SUCCESS;  // or appropriate error code
```

## Implementation Blueprint

### Data models and structure

Create the core data models to ensure type safety and consistency.
```c
// RKE key fragment structure
struct rke_fragment_t {
    uint8_t fragment_id;         // Fragment identifier (0-255)
    uint8_t total_fragments;     // Total number of fragments
    uint8_t threshold;           // Minimum fragments needed for reconstruction
    uint16_t fragment_size;      // Size of this fragment
    unsigned char data[256];     // Fragment data
    unsigned char checksum[32];  // SHA-256 checksum for integrity
};

// RKE key metadata
struct rke_key_metadata_t {
    unsigned char key_id[16];    // Unique key identifier
    uint8_t key_type;           // Key type (symmetric, etc.)
    uint8_t total_fragments;    // Total fragments created
    uint8_t threshold;          // Minimum fragments for reconstruction
    uint32_t timestamp;         // Creation timestamp
    uint8_t den;                // Denomination of owning coin
    uint32_t sn;                // Serial number of owning coin
};

// RKE session context
struct rke_session_t {
    unsigned char session_id[16]; // Session identifier
    unsigned char sender_id[16];  // Sender identity
    unsigned char receiver_id[16]; // Receiver identity
    uint8_t state;               // Session state
    uint32_t timeout;            // Session timeout
};
```

### list of tasks to be completed to fullfill the PRP in the order they should be completed

```yaml
Task 1:
CREATE src/rke/rke.h:
  - DEFINE all RKE data structures (rke_fragment_t, rke_key_metadata_t, rke_session_t)
  - DECLARE function prototypes for all RKE operations
  - INCLUDE necessary system headers and protocol.h dependencies
  - FOLLOW existing header guard patterns

Task 2:
CREATE src/rke/rke_core.c:
  - IMPLEMENT rke_generate_key() for quantum-safe key generation
  - IMPLEMENT rke_split_key() for Shamir's secret sharing algorithm
  - IMPLEMENT rke_reconstruct_key() for key reconstruction from fragments
  - MIRROR error handling patterns from examples/cmd_key_exechange.c

Task 3:
CREATE src/rke/rke_storage.c:
  - IMPLEMENT rke_store_fragment() following existing file path patterns
  - IMPLEMENT rke_load_fragment() with proper error handling
  - IMPLEMENT rke_store_metadata() for key metadata persistence
  - FOLLOW sprintf pattern: "%s/RKE/%02x%02x.../fragment_%03d.bin"

Task 4:
CREATE src/rke/rke_protocol.c:
  - IMPLEMENT cmd_rke_generate() - Generate and split new key
  - IMPLEMENT cmd_rke_exchange() - Exchange key fragments with peer
  - IMPLEMENT cmd_rke_reconstruct() - Reconstruct key from fragments
  - IMPLEMENT cmd_rke_query() - Query available key fragments
  - PRESERVE existing conn_info_t *ci parameter pattern

Task 5:
CREATE src/rke/rke_crypto.c:
  - IMPLEMENT rke_encrypt_fragment() using existing crypt_ctr function
  - IMPLEMENT rke_decrypt_fragment() for fragment decryption
  - IMPLEMENT rke_verify_checksum() for fragment integrity
  - INTEGRATE with existing AES and nonce handling

Task 6:
CREATE Makefile:
  - COMPILE all RKE modules with appropriate flags
  - LINK with existing crypto libraries
  - INCLUDE debug and release build targets
  - FOLLOW C compilation best practices

Task 7:
CREATE tests/test_rke_core.c:
  - IMPLEMENT test_key_generation() for key generation validation
  - IMPLEMENT test_key_splitting() for split/reconstruct cycle
  - IMPLEMENT test_fragment_integrity() for checksum validation
  - FOLLOW existing test patterns if any exist

Task 8:
CREATE tests/test_rke_protocol.c:
  - IMPLEMENT test_protocol_commands() for binary protocol validation
  - IMPLEMENT test_packet_validation() for malformed packet handling
  - IMPLEMENT test_error_conditions() for error code coverage
  - MIRROR existing command test patterns

Task 9:
MODIFY existing build system:
  - INTEGRATE RKE modules into existing compilation
  - ADD RKE library dependencies
  - ENSURE proper linking order
  - TEST compilation on target platform
```

### Per task pseudocode as needed added to each task

```c
// Task 2 - RKE Core Implementation
// Key generation using quantum-safe entropy
int rke_generate_key(unsigned char *key, uint16_t key_size) {
    // PATTERN: Validate input parameters first
    if (key == NULL || key_size == 0 || key_size > MAX_KEY_SIZE) {
        error("Invalid key generation parameters");
        return -1;
    }
    
    // CRITICAL: Use cryptographically secure random number generation
    // PATTERN: Check return values for all crypto operations
    if (secure_random_bytes(key, key_size) != 0) {
        error("Failed to generate secure random key");
        return -1;
    }
    
    debug("Generated %d-byte quantum-safe key", key_size);
    return 0;
}

// Task 3 - Storage Implementation
// Fragment storage following existing file patterns
int rke_store_fragment(struct rke_fragment_t *fragment, unsigned char *key_id) {
    char fragment_path[PATH_MAX];
    int fd, rv;
    
    // PATTERN: Construct paths using config.cwd base
    sprintf(fragment_path, "%s/RKE/%02x%02x%02x%02x/fragment_%03d.bin",
            config.cwd, key_id[0], key_id[1], key_id[2], key_id[3],
            fragment->fragment_id);
    
    // PATTERN: Create directory if needed and open file
    // CRITICAL: Use secure file permissions (0640)
    fd = open(fragment_path, O_CREAT | O_WRONLY, 0640);
    if (fd < 0) {
        error("Failed to create fragment file %s: %s", fragment_path, strerror(errno));
        return -1;
    }
    
    // PATTERN: Write data and check return values
    rv = write(fd, fragment, sizeof(struct rke_fragment_t));
    close(fd);
    
    if (rv != sizeof(struct rke_fragment_t)) {
        error("Failed to write fragment data");
        return -1;
    }
    
    return 0;
}

// Task 4 - Protocol Implementation
// RKE key generation command
void cmd_rke_generate(conn_info_t *ci) {
    unsigned char *payload = get_body_payload(ci);
    struct rke_key_metadata_t metadata;
    unsigned char key[256];
    
    debug("CMD RKE Generate");
    
    // PATTERN: Validate packet size first
    // Expected: 16-byte key_id + 1-byte key_type + 1-byte fragments + 1-byte threshold + 2-byte EOF = 21
    if (ci->body_size != 21) {
        error("Invalid command length: %d. Need 21", ci->body_size);
        ci->command_status = ERROR_INVALID_PACKET_LENGTH;
        return;
    }
    
    // PATTERN: Extract parameters from payload
    memcpy(metadata.key_id, payload, 16);
    metadata.key_type = payload[16];
    metadata.total_fragments = payload[17];
    metadata.threshold = payload[18];
    
    // CRITICAL: Validate business logic
    if (metadata.threshold > metadata.total_fragments) {
        error("Threshold %d cannot exceed total fragments %d", 
              metadata.threshold, metadata.total_fragments);
        ci->command_status = ERROR_INVALID_PARAMETER;
        return;
    }
    
    // PATTERN: Generate key and split into fragments
    if (rke_generate_key(key, 256) != 0) {
        ci->command_status = ERROR_KEY_GENERATION;
        return;
    }
    
    if (rke_split_key(key, 256, &metadata) != 0) {
        ci->command_status = ERROR_KEY_SPLITTING;
        return;
    }
    
    // PATTERN: Prepare response
    ci->output_size = 1;  // Success indicator
    ci->output = (unsigned char *) malloc(1);
    if (ci->output == NULL) {
        error("Can't alloc buffer for the response");
        ci->command_status = ERROR_MEMORY_ALLOC;
        return;
    }
    
    ci->output[0] = 0x01;  // Success
    ci->command_status = STATUS_SUCCESS;
    
    debug("CMD RKE Generate finished");
}
```

### Integration Points
```yaml
FILESYSTEM:
  - directory: "Create RKE/ subdirectory under config.cwd"
  - structure: "RKE/{key_id_prefix}/fragment_{id}.bin for fragment storage"
  - metadata: "RKE/{key_id_prefix}/metadata.bin for key metadata"
  
CRYPTO:
  - integration: "Use existing crypt_ctr() function for fragment encryption"
  - nonces: "Leverage existing nonce handling in conn_info_t"
  - checksums: "Implement SHA-256 checksums for fragment integrity"
  
PROTOCOL:
  - commands: "Add cmd_rke_* functions to existing command dispatcher"
  - packets: "Follow existing binary packet validation patterns"
  - errors: "Use existing error code constants and extend as needed"
  
DATABASE:
  - pages: "Integrate with existing page locking system if needed"
  - coins: "Associate RKE keys with coin denominations and serial numbers"
  - indexing: "Create efficient fragment lookup mechanisms"
```

## Validation Loop

### Level 1: Syntax & Style
```bash
# Run these FIRST - fix any errors before proceeding
gcc -Wall -Wextra -std=c99 -c src/rke/*.c  # Compile all RKE modules
gcc -Wall -Werror -std=c99 -c src/rke/*.c  # Strict compilation with warnings as errors

# Expected: No compilation errors or warnings
# If errors: READ the compiler output and fix syntax/style issues
```

### Level 2: Unit Tests each new feature/file/function use existing test patterns
```c
// CREATE tests/test_rke_core.c with these test cases:
int test_key_generation() {
    unsigned char key[256];
    
    // Test normal key generation
    int result = rke_generate_key(key, 256);
    assert(result == 0);
    
    // Test invalid parameters
    result = rke_generate_key(NULL, 256);
    assert(result == -1);
    
    result = rke_generate_key(key, 0);
    assert(result == -1);
    
    return 0;
}

int test_key_splitting() {
    unsigned char key[256];
    struct rke_key_metadata_t metadata;
    
    // Generate test key
    rke_generate_key(key, 256);
    
    // Setup metadata
    metadata.total_fragments = 5;
    metadata.threshold = 3;
    
    // Test key splitting
    int result = rke_split_key(key, 256, &metadata);
    assert(result == 0);
    
    // Test key reconstruction
    unsigned char reconstructed[256];
    result = rke_reconstruct_key(reconstructed, 256, &metadata);
    assert(result == 0);
    
    // Verify keys match
    assert(memcmp(key, reconstructed, 256) == 0);
    
    return 0;
}

int test_fragment_integrity() {
    struct rke_fragment_t fragment;
    
    // Setup test fragment
    fragment.fragment_id = 1;
    fragment.fragment_size = 64;
    memset(fragment.data, 0xAA, 64);
    
    // Test checksum calculation
    rke_calculate_checksum(&fragment);
    
    // Verify checksum validation
    assert(rke_verify_checksum(&fragment) == 0);
    
    // Test corruption detection
    fragment.data[0] = 0xBB;
    assert(rke_verify_checksum(&fragment) == -1);
    
    return 0;
}
```

```bash
# Run and iterate until passing:
gcc -o test_rke tests/test_rke_core.c src/rke/*.c -I src/
./test_rke
# If failing: Read error, understand root cause, fix code, re-run
```

### Level 3: Integration Test
```bash
# Compile complete system
make all

# Test RKE protocol commands
echo -e "RKE_GENERATE_TEST_PACKET" | ./rke_test_client

# Expected: Successful key generation and fragment distribution
# If error: Check debug logs for detailed error information
```

## Final validation Checklist
- [ ] All tests pass: `make test`
- [ ] No compilation errors: `gcc -Wall -Werror src/rke/*.c`
- [ ] No memory leaks: `valgrind ./test_rke`
- [ ] Protocol commands respond correctly: test with binary client
- [ ] Fragment storage/retrieval works: verify file system operations
- [ ] Crypto integration secure: review all crypt_ctr usage
- [ ] Error handling comprehensive: test all error conditions
- [ ] Logging informative: verify debug/error message clarity

---

## Anti-Patterns to Avoid
- ❌ Don't use public-key cryptography (violates quantum-safe requirement)
- ❌ Don't store complete keys in single location (violates RAIDA principles)
- ❌ Don't skip packet size validation (security vulnerability)
- ❌ Don't ignore malloc return values (causes crashes)
- ❌ Don't hardcode file paths (breaks configuration flexibility)
- ❌ Don't use weak random number generation (cryptographic weakness)
- ❌ Don't skip checksum validation (integrity vulnerability)
- ❌ Don't forget to unlock pages (causes deadlocks)

---

## Confidence Level: 9/10

This PRP provides comprehensive context for one-pass implementation success:
- ✅ Complete understanding of RKE quantum-safe principles
- ✅ Detailed analysis of existing codebase patterns  
- ✅ Specific implementation blueprint with pseudocode
- ✅ Executable validation gates for C compilation and testing
- ✅ Clear integration points with existing infrastructure
- ✅ Security-focused approach with known gotchas documented
- ✅ File organization follows CLAUDE.md conventions
- ✅ Error handling strategy based on existing patterns

The only uncertainty (1 point deduction) is the specific Shamir's secret sharing algorithm implementation details, but the overall architecture and integration approach is well-defined for successful implementation.