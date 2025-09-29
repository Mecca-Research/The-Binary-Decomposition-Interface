
/*
 * Master Memory Manager - Phase 4 Inter-Component Communication
 * Advanced messaging and coordination systems
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_COMMUNICATION_H
#define MMM_COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Message types
typedef enum {
    MMM_MSG_SYSTEM_STATUS = 1,
    MMM_MSG_OPTIMIZATION_REQUEST,
    MMM_MSG_OPTIMIZATION_RESULT,
    MMM_MSG_HEALTH_CHECK,
    MMM_MSG_ALERT,
    MMM_MSG_CONFIGURATION_UPDATE,
    MMM_MSG_SHUTDOWN_REQUEST,
    MMM_MSG_CUSTOM
} mmm_message_type_t;

// CRITICAL FIX: Use common priority definitions to avoid conflicts
#include "../../mmm_common.h"

// Communication reliability levels
typedef enum {
    MMM_COMM_BEST_EFFORT = 1,
    MMM_COMM_RELIABLE,
    MMM_COMM_GUARANTEED
} mmm_communication_reliability_t;

// Communication configuration
typedef struct {
    uint32_t message_queue_size;
    uint32_t max_components;
    uint32_t timeout_ms;
    mmm_communication_reliability_t reliability_level;
    bool encryption_enabled;
    bool compression_enabled;
    uint32_t max_message_size;
    uint32_t heartbeat_interval_ms;
} mmm_communication_config_t;

// Message structure
typedef struct {
    uint32_t message_id;
    uint32_t sender_id;
    uint32_t receiver_id;
    mmm_message_type_t message_type;
    mmm_priority_t priority;
    uint32_t data_size;
    uint8_t data[1024];  // Message payload
    struct timespec timestamp;
    struct timespec expiry_time;
    bool requires_ack;
    bool encrypted;
} mmm_message_t;

// Function declarations

/**
 * Initialize communication system
 * @param config Communication configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_communication_init(mmm_communication_config_t *config);

/**
 * Send message
 * @param message Message to send
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_send_message(mmm_message_t *message);

/**
 * Receive message
 * @param receiver_id Receiver component ID
 * @param message Output message
 * @param timeout_ms Timeout in milliseconds
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_receive_message(uint32_t receiver_id, mmm_message_t *message, uint32_t timeout_ms);

/**
 * Cleanup communication system
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_communication_cleanup(void);

#endif /* MMM_COMMUNICATION_H */
