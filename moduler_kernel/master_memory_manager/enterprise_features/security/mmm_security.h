
/*
 * Master Memory Manager - Phase 4 Security Framework
 * Complete security model with encryption and access control
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_SECURITY_H
#define MMM_SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Security levels
typedef enum {
    MMM_SECURITY_LEVEL_NONE = 0,
    MMM_SECURITY_LEVEL_BASIC,
    MMM_SECURITY_LEVEL_STANDARD,
    MMM_SECURITY_LEVEL_HIGH,
    MMM_SECURITY_LEVEL_MAXIMUM
} mmm_security_level_t;

// Access control operations
typedef enum {
    MMM_ACCESS_READ = 0x01,
    MMM_ACCESS_WRITE = 0x02,
    MMM_ACCESS_EXECUTE = 0x04,
    MMM_ACCESS_DELETE = 0x08,
    MMM_ACCESS_CONFIGURE = 0x10,
    MMM_ACCESS_MONITOR = 0x20,
    MMM_ACCESS_ADMIN = 0x40,
    MMM_ACCESS_ALL = 0xFF
} mmm_access_operation_t;

// Encryption algorithms
typedef enum {
    MMM_ENCRYPTION_NONE = 0,
    MMM_ENCRYPTION_AES128,
    MMM_ENCRYPTION_AES256,
    MMM_ENCRYPTION_CHACHA20,
    MMM_ENCRYPTION_RSA2048,
    MMM_ENCRYPTION_RSA4096
} mmm_encryption_algorithm_t;

// Security configuration
typedef struct {
    uint32_t encryption_level;
    uint32_t access_control_flags;
    uint32_t audit_level;
    char security_policy[256];
    bool authentication_required;
    bool authorization_required;
    bool encryption_at_rest;
    bool encryption_in_transit;
    uint32_t session_timeout_ms;
    uint32_t max_failed_attempts;
} mmm_security_config_t;

// User credentials
typedef struct {
    uint32_t user_id;
    char username[64];
    char password_hash[128];
    uint32_t access_level;
    uint32_t permissions;
    bool active;
    struct timespec created_at;
    struct timespec last_login;
    uint32_t failed_attempts;
} mmm_user_credentials_t;

// Security context
typedef struct {
    uint32_t session_id;
    uint32_t user_id;
    uint32_t access_level;
    uint32_t permissions;
    struct timespec session_start;
    struct timespec last_activity;
    char client_ip[16];
    bool authenticated;
    bool authorized;
} mmm_security_context_t;

// Function declarations

/**
 * Initialize security framework
 * @param config Security configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_security_init(mmm_security_config_t *config);

/**
 * Validate access
 * @param user_id User ID
 * @param resource_id Resource ID
 * @param operation Operation to validate
 * @return MMM_SUCCESS if access allowed, error code if denied
 */
int mmm_validate_access(uint32_t user_id, uint32_t resource_id, uint32_t operation);

/**
 * Cleanup security framework
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_security_cleanup(void);

#endif /* MMM_SECURITY_H */
