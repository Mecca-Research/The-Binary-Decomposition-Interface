#ifndef PHASE3_PMD_H
#define PHASE3_PMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pmd_driver pmd_driver_t;

int pmd_init(void);
void pmd_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
