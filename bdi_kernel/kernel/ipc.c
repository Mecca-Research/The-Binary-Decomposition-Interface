
/**
 * @file ipc.c
 * @brief Core IPC framework implementation
 * 
 * Phase 4: Zero-Copy IPC & Communication
 */

#include "ipc.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Global IPC state */
static struct ipc_global_state g_ipc_state = {0};

/* IPC handle table (simple array for now) */
static struct ipc_handle *g_ipc_handles[IPC_MAX_HANDLES] = {0};
static _Atomic uint32_t g_handle_count = 0;

/* ===================================================================
 * Internal Helper Functions
 * =================================================================== */

/**
 * @brief Allocate IPC handle
 */
static struct ipc_handle *ipc_alloc_handle(void)
{
    struct ipc_handle *handle = alloc_memory(sizeof(struct ipc_handle), 64);
    if (!handle) {
        return NULL;
    }
    
    memset(handle, 0, sizeof(struct ipc_handle));
    
    /* Assign unique ID */
    handle->id = atomic_fetch_add_explicit(&g_ipc_state.next_id, 1,
                                           memory_order_relaxed);
    
    /* Initialize atomic fields */
    atomic_store_explicit(&handle->state, IPC_STATE_CREATED,
                         memory_order_relaxed);
    atomic_store_explicit(&handle->ref_count, 1, memory_order_relaxed);
    
    return handle;
}

/**
 * @brief Free IPC handle
 */
static void ipc_free_handle(struct ipc_handle *handle)
{
    if (!handle) {
        return;
    }
    
    free_memory(handle, sizeof(struct ipc_handle));
}

/**
 * @brief Find IPC handle by name
 */
static struct ipc_handle *ipc_find_by_name(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    uint32_t count = atomic_load_explicit(&g_handle_count,
                                         memory_order_acquire);
    
    for (uint32_t i = 0; i < count && i < IPC_MAX_HANDLES; i++) {
        struct ipc_handle *handle = g_ipc_handles[i];
        if (handle && strcmp(handle->name, name) == 0) {
            return handle;
        }
    }
    
    return NULL;
}

/**
 * @brief Add handle to global table
 */
static int ipc_add_handle(struct ipc_handle *handle)
{
    uint32_t count = atomic_load_explicit(&g_handle_count,
                                         memory_order_acquire);
    
    if (count >= IPC_MAX_HANDLES) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Find empty slot */
    for (uint32_t i = 0; i < IPC_MAX_HANDLES; i++) {
        if (g_ipc_handles[i] == NULL) {
            g_ipc_handles[i] = handle;
            atomic_fetch_add_explicit(&g_handle_count, 1,
                                     memory_order_release);
            atomic_fetch_add_explicit(&g_ipc_state.total_objects, 1,
                                     memory_order_relaxed);
            atomic_fetch_add_explicit(&g_ipc_state.active_objects, 1,
                                     memory_order_relaxed);
            return IPC_SUCCESS;
        }
    }
    
    return IPC_ERROR_NOMEM;
}

/**
 * @brief Remove handle from global table
 */
static void ipc_remove_handle(struct ipc_handle *handle)
{
    if (!handle) {
        return;
    }
    
    for (uint32_t i = 0; i < IPC_MAX_HANDLES; i++) {
        if (g_ipc_handles[i] == handle) {
            g_ipc_handles[i] = NULL;
            atomic_fetch_sub_explicit(&g_handle_count, 1,
                                     memory_order_release);
            atomic_fetch_sub_explicit(&g_ipc_state.active_objects, 1,
                                     memory_order_relaxed);
            break;
        }
    }
}

/* ===================================================================
 * Public API Implementation
 * =================================================================== */

int ipc_init(void)
{
    /* Initialize global state */
    atomic_store_explicit(&g_ipc_state.total_objects, 0,
                         memory_order_relaxed);
    atomic_store_explicit(&g_ipc_state.active_objects, 0,
                         memory_order_relaxed);
    atomic_store_explicit(&g_ipc_state.next_id, 1, memory_order_relaxed);
    
    /* Clear handle table */
    memset(g_ipc_handles, 0, sizeof(g_ipc_handles));
    atomic_store_explicit(&g_handle_count, 0, memory_order_relaxed);
    
    printf("[IPC] IPC subsystem initialized\n");
    return IPC_SUCCESS;
}

void ipc_shutdown(void)
{
    /* Close all active handles */
    uint32_t count = atomic_load_explicit(&g_handle_count,
                                         memory_order_acquire);
    
    for (uint32_t i = 0; i < IPC_MAX_HANDLES && count > 0; i++) {
        struct ipc_handle *handle = g_ipc_handles[i];
        if (handle) {
            ipc_destroy(handle);
            count--;
        }
    }
    
    printf("[IPC] IPC subsystem shutdown\n");
}

int ipc_create(struct ipc_handle **handle,
               enum ipc_type type,
               const char *name,
               uint32_t flags)
{
    if (!handle || type <= IPC_TYPE_INVALID || type >= IPC_TYPE_MAX) {
        return IPC_ERROR_INVALID;
    }
    
    /* Check if name already exists */
    if (name && ipc_find_by_name(name)) {
        return IPC_ERROR_EXISTS;
    }
    
    /* Allocate handle */
    struct ipc_handle *h = ipc_alloc_handle();
    if (!h) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Initialize handle */
    h->type = type;
    h->flags = flags;
    h->permissions = IPC_PERM_READ | IPC_PERM_WRITE | IPC_PERM_OWNER;
    h->owner_pid = 0; /* TODO: Get current process ID */
    h->numa_node = numa_current_node();
    
    if (name) {
        strncpy(h->name, name, IPC_NAME_MAX - 1);
        h->name[IPC_NAME_MAX - 1] = '\0';
    }
    
    /* Add to global table */
    int ret = ipc_add_handle(h);
    if (ret != IPC_SUCCESS) {
        ipc_free_handle(h);
        return ret;
    }
    
    /* Set state to ready */
    atomic_store_explicit(&h->state, IPC_STATE_READY, memory_order_release);
    
    *handle = h;
    return IPC_SUCCESS;
}

int ipc_destroy(struct ipc_handle *handle)
{
    if (!handle) {
        return IPC_ERROR_INVALID;
    }
    
    /* Set state to closing */
    enum ipc_state expected = IPC_STATE_READY;
    if (!atomic_compare_exchange_strong_explicit(&handle->state,
                                                 &expected,
                                                 IPC_STATE_CLOSING,
                                                 memory_order_acquire,
                                                 memory_order_relaxed)) {
        /* Already closing or closed */
        return IPC_ERROR_CLOSED;
    }
    
    /* Wait for all references to be released */
    while (atomic_load_explicit(&handle->ref_count, memory_order_acquire) > 1) {
        /* TODO: Add timeout or force cleanup */
    }
    
    /* Remove from global table */
    ipc_remove_handle(handle);
    
    /* Set state to closed */
    atomic_store_explicit(&handle->state, IPC_STATE_CLOSED,
                         memory_order_release);
    
    /* Free handle */
    ipc_free_handle(handle);
    
    return IPC_SUCCESS;
}

int ipc_open(struct ipc_handle **handle, const char *name, uint32_t flags)
{
    if (!handle || !name) {
        return IPC_ERROR_INVALID;
    }
    
    /* Find handle by name */
    struct ipc_handle *h = ipc_find_by_name(name);
    if (!h) {
        return IPC_ERROR_NOTFOUND;
    }
    
    /* Check state */
    enum ipc_state state = atomic_load_explicit(&h->state,
                                               memory_order_acquire);
    if (state != IPC_STATE_READY && state != IPC_STATE_ACTIVE) {
        return IPC_ERROR_CLOSED;
    }
    
    /* Increment reference count */
    ipc_ref(h);
    
    *handle = h;
    return IPC_SUCCESS;
}

int ipc_close(struct ipc_handle *handle)
{
    if (!handle) {
        return IPC_ERROR_INVALID;
    }
    
    /* Decrement reference count */
    uint32_t refs = ipc_unref(handle);
    
    /* If last reference, destroy handle */
    if (refs == 0) {
        return ipc_destroy(handle);
    }
    
    return IPC_SUCCESS;
}

uint32_t ipc_ref(struct ipc_handle *handle)
{
    if (!handle) {
        return 0;
    }
    
    return atomic_fetch_add_explicit(&handle->ref_count, 1,
                                    memory_order_relaxed) + 1;
}

uint32_t ipc_unref(struct ipc_handle *handle)
{
    if (!handle) {
        return 0;
    }
    
    uint32_t old = atomic_fetch_sub_explicit(&handle->ref_count, 1,
                                             memory_order_release);
    if (old > 0) {
        return old - 1;
    }
    return 0;
}

enum ipc_state ipc_get_state(const struct ipc_handle *handle)
{
    if (!handle) {
        return IPC_STATE_INVALID;
    }
    
    return atomic_load_explicit(&handle->state, memory_order_acquire);
}

bool ipc_set_state(struct ipc_handle *handle, enum ipc_state new_state)
{
    if (!handle) {
        return false;
    }
    
    atomic_store_explicit(&handle->state, new_state, memory_order_release);
    return true;
}

bool ipc_cas_state(struct ipc_handle *handle,
                   enum ipc_state expected,
                   enum ipc_state desired)
{
    if (!handle) {
        return false;
    }
    
    return atomic_compare_exchange_strong_explicit(&handle->state,
                                                   &expected,
                                                   desired,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire);
}

void ipc_get_stats(const struct ipc_handle *handle, struct ipc_stats *stats)
{
    if (!handle || !stats) {
        return;
    }
    
    stats->total_sends = atomic_load_explicit(&handle->stats.total_sends,
                                             memory_order_relaxed);
    stats->total_recvs = atomic_load_explicit(&handle->stats.total_recvs,
                                             memory_order_relaxed);
    stats->total_bytes_sent = atomic_load_explicit(&handle->stats.total_bytes_sent,
                                                   memory_order_relaxed);
    stats->total_bytes_recv = atomic_load_explicit(&handle->stats.total_bytes_recv,
                                                   memory_order_relaxed);
    stats->blocked_sends = atomic_load_explicit(&handle->stats.blocked_sends,
                                               memory_order_relaxed);
    stats->blocked_recvs = atomic_load_explicit(&handle->stats.blocked_recvs,
                                               memory_order_relaxed);
    stats->failed_sends = atomic_load_explicit(&handle->stats.failed_sends,
                                              memory_order_relaxed);
    stats->failed_recvs = atomic_load_explicit(&handle->stats.failed_recvs,
                                              memory_order_relaxed);
}

void ipc_get_global_stats(struct ipc_stats *stats)
{
    if (!stats) {
        return;
    }
    
    ipc_get_stats((const struct ipc_handle *)&g_ipc_state, stats);
}

void ipc_print_stats(const struct ipc_handle *handle)
{
    struct ipc_stats stats;
    
    if (handle) {
        ipc_get_stats(handle, &stats);
        printf("[IPC] Statistics for handle %lu (%s):\n",
               handle->id, handle->name[0] ? handle->name : "unnamed");
    } else {
        ipc_get_global_stats(&stats);
        printf("[IPC] Global statistics:\n");
    }
    
    printf("  Total sends: %lu\n", stats.total_sends);
    printf("  Total recvs: %lu\n", stats.total_recvs);
    printf("  Total bytes sent: %lu\n", stats.total_bytes_sent);
    printf("  Total bytes recv: %lu\n", stats.total_bytes_recv);
    printf("  Blocked sends: %lu\n", stats.blocked_sends);
    printf("  Blocked recvs: %lu\n", stats.blocked_recvs);
    printf("  Failed sends: %lu\n", stats.failed_sends);
    printf("  Failed recvs: %lu\n", stats.failed_recvs);
}

const char *ipc_type_name(enum ipc_type type)
{
    switch (type) {
        case IPC_TYPE_INVALID: return "INVALID";
        case IPC_TYPE_SHM: return "SHM";
        case IPC_TYPE_PIPE: return "PIPE";
        case IPC_TYPE_SOCKET: return "SOCKET";
        default: return "UNKNOWN";
    }
}

const char *ipc_state_name(enum ipc_state state)
{
    switch (state) {
        case IPC_STATE_INVALID: return "INVALID";
        case IPC_STATE_CREATED: return "CREATED";
        case IPC_STATE_READY: return "READY";
        case IPC_STATE_ACTIVE: return "ACTIVE";
        case IPC_STATE_CLOSING: return "CLOSING";
        case IPC_STATE_CLOSED: return "CLOSED";
        default: return "UNKNOWN";
    }
}

const char *ipc_error_string(int error)
{
    switch (error) {
        case IPC_SUCCESS: return "Success";
        case IPC_ERROR_INVALID: return "Invalid parameter";
        case IPC_ERROR_NOMEM: return "Out of memory";
        case IPC_ERROR_NOTFOUND: return "Not found";
        case IPC_ERROR_EXISTS: return "Already exists";
        case IPC_ERROR_PERM: return "Permission denied";
        case IPC_ERROR_BUSY: return "Resource busy";
        case IPC_ERROR_TIMEOUT: return "Operation timed out";
        case IPC_ERROR_CLOSED: return "IPC object closed";
        case IPC_ERROR_FULL: return "Buffer full";
        case IPC_ERROR_EMPTY: return "Buffer empty";
        default: return "Unknown error";
    }
}
