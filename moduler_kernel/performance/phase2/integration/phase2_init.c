
/**
 * @file phase2_init.c
 * @brief Phase 2 initialization implementation
 */

#include "phase2_init.h"
#include "numa_topology.h"
#include "per_cpu_arena.h"
#include "attention.h"
#include "huge_pages.h"
#include "pcid.h"
#include "timer_wheel.h"
#include <stdio.h>
#include <string.h>

static phase2_config_t g_config = {0};
static bool g_initialized = false;

int phase2_init(const phase2_config_t* config) {
    if (g_initialized) {
        return 0;
    }
    
    printf("Initializing Phase 2: Memory & Scheduling Optimization...\n");
    
    // Set default configuration
    g_config.enable_numa = true;
    g_config.enable_attention = true;
    g_config.attention_threshold = 1000;
    g_config.enable_huge_pages = true;
    g_config.enable_pcid = true;
    g_config.enable_prefetch = true;
    g_config.enable_tickless = true;
    g_config.enable_timer_wheel = true;
    g_config.integrate_phase1 = true;
    
    // Override with user config
    if (config) {
        g_config = *config;
    }
    
    // Initialize NUMA subsystem
    if (g_config.enable_numa) {
        printf("  Initializing NUMA topology...\n");
        if (numa_topology_init() == NULL) {
            fprintf(stderr, "  Warning: NUMA topology initialization failed\n");
        } else {
            numa_topology_print();
        }
        
        printf("  Initializing per-CPU arenas...\n");
        if (per_cpu_arena_init() < 0) {
            fprintf(stderr, "  Warning: Per-CPU arena initialization failed\n");
        }
        
        if (g_config.enable_attention) {
            printf("  Initializing attention-guided allocation...\n");
            attention_config_t attn_cfg = {
                .migration_threshold = g_config.attention_threshold,
                .migration_cooldown = 1000,
                .migration_cost_factor = 0.2,
                .ema_alpha = 0.1,
                .enable_migration = true
            };
            if (attention_init(&attn_cfg) < 0) {
                fprintf(stderr, "  Warning: Attention initialization failed\n");
            }
        }
    }
    
    // Initialize prefetch subsystem
    if (g_config.enable_huge_pages) {
        printf("  Initializing huge pages...\n");
        huge_page_config_t hp_cfg = {
            .enable_2mb = true,
            .enable_1gb = true,
            .enable_thp = true,
            .promotion_threshold = 512 * 1024,
            .demotion_threshold = 80
        };
        if (huge_page_init(&hp_cfg) < 0) {
            fprintf(stderr, "  Warning: Huge page initialization failed\n");
        }
    }
    
    if (g_config.enable_pcid) {
        printf("  Initializing PCID/ASID...\n");
        pcid_config_t pcid_cfg = {
            .enable_pcid = true,
            .enable_invpcid = true,
            .eviction_threshold = 3072
        };
        if (pcid_init(&pcid_cfg) < 0) {
            fprintf(stderr, "  Warning: PCID initialization failed\n");
        }
    }
    
    // Scheduler subsystem initialization would go here
    // (timer wheel is created per-CPU, not globally)
    
    printf("Phase 2 initialization complete!\n\n");
    
    g_initialized = true;
    return 0;
}

bool phase2_is_initialized(void) {
    return g_initialized;
}

const phase2_config_t* phase2_get_config(void) {
    return &g_config;
}

void phase2_print_status(void) {
    printf("Phase 2 Status:\n");
    printf("  Initialized: %s\n", g_initialized ? "yes" : "no");
    printf("  NUMA: %s\n", g_config.enable_numa ? "enabled" : "disabled");
    printf("  Attention: %s\n", g_config.enable_attention ? "enabled" : "disabled");
    printf("  Huge Pages: %s\n", g_config.enable_huge_pages ? "enabled" : "disabled");
    printf("  PCID: %s\n", g_config.enable_pcid ? "enabled" : "disabled");
    printf("  Prefetch: %s\n", g_config.enable_prefetch ? "enabled" : "disabled");
    printf("  Tickless: %s\n", g_config.enable_tickless ? "enabled" : "disabled");
    printf("  Timer Wheel: %s\n", g_config.enable_timer_wheel ? "enabled" : "disabled");
}

void phase2_print_all_stats(void) {
    if (!g_initialized) {
        printf("Phase 2 not initialized\n");
        return;
    }
    
    printf("\n=== Phase 2 Statistics ===\n\n");
    
    if (g_config.enable_numa) {
        numa_topology_t* topo = numa_topology_get();
        if (topo) {
            for (uint32_t i = 0; i < topo->num_cpus; i++) {
                per_cpu_arena_print_stats(i);
                printf("\n");
            }
        }
    }
    
    if (g_config.enable_huge_pages) {
        huge_page_print_stats();
        printf("\n");
    }
    
    if (g_config.enable_pcid) {
        pcid_print_stats();
        printf("\n");
    }
}

void phase2_reset_all_stats(void) {
    if (!g_initialized) {
        return;
    }
    
    if (g_config.enable_numa) {
        numa_topology_t* topo = numa_topology_get();
        if (topo) {
            for (uint32_t i = 0; i < topo->num_cpus; i++) {
                per_cpu_arena_reset_stats(i);
            }
        }
    }
    
    if (g_config.enable_huge_pages) {
        huge_page_reset_stats();
    }
    
    if (g_config.enable_pcid) {
        pcid_reset_stats();
    }
}

void phase2_destroy(void) {
    if (!g_initialized) {
        return;
    }
    
    printf("Destroying Phase 2...\n");
    
    if (g_config.enable_attention) {
        attention_destroy();
    }
    
    if (g_config.enable_numa) {
        per_cpu_arena_destroy();
        numa_topology_destroy();
    }
    
    if (g_config.enable_huge_pages) {
        huge_page_destroy();
    }
    
    if (g_config.enable_pcid) {
        pcid_destroy();
    }
    
    memset(&g_config, 0, sizeof(g_config));
    g_initialized = false;
    
    printf("Phase 2 destroyed\n");
}
