#ifndef PHASE3_MBUF_H
#define PHASE3_MBUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mbuf mbuf_t;

int mbuf_pool_init(void);
void mbuf_pool_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
