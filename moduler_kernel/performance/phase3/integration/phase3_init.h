#ifndef PHASE3_INIT_H
#define PHASE3_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Phase 3 subsystems
 * @return 0 on success, negative on error
 */
int phase3_init(void);

/**
 * @brief Shutdown Phase 3 subsystems
 */
void phase3_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
