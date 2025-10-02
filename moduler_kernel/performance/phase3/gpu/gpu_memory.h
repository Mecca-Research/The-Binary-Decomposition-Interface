#ifndef PHASE3_GPU_MEMORY_H
#define PHASE3_GPU_MEMORY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int gpu_memory_init(void);
void gpu_memory_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
