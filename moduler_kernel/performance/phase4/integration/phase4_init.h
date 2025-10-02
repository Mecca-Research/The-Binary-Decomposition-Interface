#ifndef PHASE4_INIT_H
#define PHASE4_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Phase 4 subsystems
 * @return 0 on success, negative on error
 */
int phase4_init(void);

/**
 * @brief Shutdown Phase 4 subsystems
 */
void phase4_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
