#ifndef PHASE3_GPU_KERNEL_H
#define PHASE3_GPU_KERNEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int gpu_kernel_init(void);
void gpu_kernel_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
