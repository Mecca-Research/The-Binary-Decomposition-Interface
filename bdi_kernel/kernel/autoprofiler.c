
/**
 * BDI Kernel Autoprofiler Implementation - Phase 6
 * Automatic profiling for PGO integration
 */

#include "autoprofiler.h"
#include "optimization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* ============================================================================
 * Global Autoprofiler Context
 * ============================================================================ */

static autoprofiler_ctx_t g_autoprofiler = {
    .enabled = false,
    .collecting = false,
    .start_time = 0,
    .total_samples = 0,
    .num_points = 0,
};

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * Create directory if it doesn't exist
 */
static int create_directory(const char* path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            fprintf(stderr, "Failed to create directory %s: %s\n", 
                    path, strerror(errno));
            return -1;
        }
    }
    
    return 0;
}

/**
 * Get current timestamp in cycles
 */
static inline uint64_t get_timestamp(void) {
    return opt_rdtsc();
}

/* ============================================================================
 * Autoprofiler API Implementation
 * ============================================================================ */

int autoprofiler_init(void) {
    /* Reset context */
    memset(&g_autoprofiler, 0, sizeof(g_autoprofiler));
    
    /* Create PGO data directory */
    if (create_directory("./pgo-data") != 0) {
        return -1;
    }
    
    g_autoprofiler.enabled = true;
    
    printf("Autoprofiler initialized\n");
    return 0;
}

int autoprofiler_start(void) {
    if (!g_autoprofiler.enabled) {
        fprintf(stderr, "Autoprofiler not initialized\n");
        return -1;
    }
    
    if (g_autoprofiler.collecting) {
        fprintf(stderr, "Autoprofiler already collecting\n");
        return -1;
    }
    
    g_autoprofiler.collecting = true;
    g_autoprofiler.start_time = get_timestamp();
    g_autoprofiler.total_samples = 0;
    
    printf("Autoprofiler started\n");
    return 0;
}

int autoprofiler_stop(void) {
    if (!g_autoprofiler.enabled) {
        fprintf(stderr, "Autoprofiler not initialized\n");
        return -1;
    }
    
    if (!g_autoprofiler.collecting) {
        fprintf(stderr, "Autoprofiler not collecting\n");
        return -1;
    }
    
    g_autoprofiler.collecting = false;
    
    uint64_t end_time = get_timestamp();
    uint64_t duration = end_time - g_autoprofiler.start_time;
    
    printf("Autoprofiler stopped\n");
    printf("Duration: %lu cycles\n", duration);
    printf("Total samples: %lu\n", g_autoprofiler.total_samples);
    
    return 0;
}

int autoprofiler_save(const char* filename) {
    if (!g_autoprofiler.enabled) {
        fprintf(stderr, "Autoprofiler not initialized\n");
        return -1;
    }
    
    const char* output_file = filename ? filename : AUTOPROFILER_DATA_FILE;
    
    FILE* fp = fopen(output_file, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", 
                output_file, strerror(errno));
        return -1;
    }
    
    /* Write header */
    uint32_t magic = 0x50524F46; /* "PROF" */
    uint32_t version = 1;
    fwrite(&magic, sizeof(magic), 1, fp);
    fwrite(&version, sizeof(version), 1, fp);
    fwrite(&g_autoprofiler.num_points, sizeof(g_autoprofiler.num_points), 1, fp);
    
    /* Write profiling points */
    for (uint32_t i = 0; i < g_autoprofiler.num_points; i++) {
        profile_point_t* point = &g_autoprofiler.points[i];
        
        /* Write strings */
        uint32_t func_len = strlen(point->function_name) + 1;
        uint32_t file_len = strlen(point->file_name) + 1;
        
        fwrite(&func_len, sizeof(func_len), 1, fp);
        fwrite(point->function_name, 1, func_len, fp);
        
        fwrite(&file_len, sizeof(file_len), 1, fp);
        fwrite(point->file_name, 1, file_len, fp);
        
        /* Write data */
        fwrite(&point->line_number, sizeof(point->line_number), 1, fp);
        fwrite(&point->type, sizeof(point->type), 1, fp);
        fwrite(&point->hit_count, sizeof(point->hit_count), 1, fp);
        fwrite(&point->total_cycles, sizeof(point->total_cycles), 1, fp);
        fwrite(&point->min_cycles, sizeof(point->min_cycles), 1, fp);
        fwrite(&point->max_cycles, sizeof(point->max_cycles), 1, fp);
    }
    
    fclose(fp);
    
    printf("Profiling data saved to %s\n", output_file);
    return 0;
}

int autoprofiler_load(const char* filename) {
    const char* input_file = filename ? filename : AUTOPROFILER_DATA_FILE;
    
    FILE* fp = fopen(input_file, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for reading: %s\n", 
                input_file, strerror(errno));
        return -1;
    }
    
    /* Read header */
    uint32_t magic, version, num_points;
    fread(&magic, sizeof(magic), 1, fp);
    fread(&version, sizeof(version), 1, fp);
    fread(&num_points, sizeof(num_points), 1, fp);
    
    if (magic != 0x50524F46) {
        fprintf(stderr, "Invalid profile data file\n");
        fclose(fp);
        return -1;
    }
    
    if (num_points > AUTOPROFILER_MAX_POINTS) {
        fprintf(stderr, "Too many profiling points in file\n");
        fclose(fp);
        return -1;
    }
    
    /* Read profiling points */
    g_autoprofiler.num_points = num_points;
    
    for (uint32_t i = 0; i < num_points; i++) {
        profile_point_t* point = &g_autoprofiler.points[i];
        
        /* Read strings */
        uint32_t func_len, file_len;
        fread(&func_len, sizeof(func_len), 1, fp);
        
        char* func_name = malloc(func_len);
        fread(func_name, 1, func_len, fp);
        point->function_name = func_name;
        
        fread(&file_len, sizeof(file_len), 1, fp);
        
        char* file_name = malloc(file_len);
        fread(file_name, 1, file_len, fp);
        point->file_name = file_name;
        
        /* Read data */
        fread(&point->line_number, sizeof(point->line_number), 1, fp);
        fread(&point->type, sizeof(point->type), 1, fp);
        fread(&point->hit_count, sizeof(point->hit_count), 1, fp);
        fread(&point->total_cycles, sizeof(point->total_cycles), 1, fp);
        fread(&point->min_cycles, sizeof(point->min_cycles), 1, fp);
        fread(&point->max_cycles, sizeof(point->max_cycles), 1, fp);
    }
    
    fclose(fp);
    
    printf("Profiling data loaded from %s\n", input_file);
    return 0;
}

void autoprofiler_reset(void) {
    for (uint32_t i = 0; i < g_autoprofiler.num_points; i++) {
        g_autoprofiler.points[i].hit_count = 0;
        g_autoprofiler.points[i].total_cycles = 0;
        g_autoprofiler.points[i].min_cycles = UINT64_MAX;
        g_autoprofiler.points[i].max_cycles = 0;
    }
    
    g_autoprofiler.total_samples = 0;
    
    printf("Profiling data reset\n");
}

int autoprofiler_register_point(
    const char* function_name,
    const char* file_name,
    uint32_t line_number,
    profile_point_type_t type
) {
    if (!g_autoprofiler.enabled) {
        return -1;
    }
    
    if (g_autoprofiler.num_points >= AUTOPROFILER_MAX_POINTS) {
        fprintf(stderr, "Maximum number of profiling points reached\n");
        return -1;
    }
    
    int point_id = g_autoprofiler.num_points++;
    profile_point_t* point = &g_autoprofiler.points[point_id];
    
    point->function_name = function_name;
    point->file_name = file_name;
    point->line_number = line_number;
    point->type = type;
    point->hit_count = 0;
    point->total_cycles = 0;
    point->min_cycles = UINT64_MAX;
    point->max_cycles = 0;
    
    return point_id;
}

void autoprofiler_record(int point_id, uint64_t cycles) {
    if (!g_autoprofiler.enabled || !g_autoprofiler.collecting) {
        return;
    }
    
    if (point_id < 0 || point_id >= (int)g_autoprofiler.num_points) {
        return;
    }
    
    profile_point_t* point = &g_autoprofiler.points[point_id];
    
    point->hit_count++;
    point->total_cycles += cycles;
    
    if (cycles < point->min_cycles) {
        point->min_cycles = cycles;
    }
    
    if (cycles > point->max_cycles) {
        point->max_cycles = cycles;
    }
    
    g_autoprofiler.total_samples++;
}

const profile_point_t* autoprofiler_get_stats(int point_id) {
    if (point_id < 0 || point_id >= (int)g_autoprofiler.num_points) {
        return NULL;
    }
    
    return &g_autoprofiler.points[point_id];
}

void autoprofiler_print_report(void) {
    printf("\n");
    printf("========================================\n");
    printf("Autoprofiler Report\n");
    printf("========================================\n");
    printf("Total profiling points: %u\n", g_autoprofiler.num_points);
    printf("Total samples: %lu\n", g_autoprofiler.total_samples);
    printf("\n");
    
    printf("%-40s %-20s %10s %15s %15s %15s\n",
           "Function", "Type", "Hits", "Avg Cycles", "Min Cycles", "Max Cycles");
    printf("----------------------------------------"
           "----------------------------------------"
           "----------------------------------------\n");
    
    for (uint32_t i = 0; i < g_autoprofiler.num_points; i++) {
        profile_point_t* point = &g_autoprofiler.points[i];
        
        if (point->hit_count == 0) {
            continue;
        }
        
        const char* type_str;
        switch (point->type) {
            case PROFILE_FUNCTION_ENTRY: type_str = "Function Entry"; break;
            case PROFILE_FUNCTION_EXIT: type_str = "Function Exit"; break;
            case PROFILE_BRANCH_TAKEN: type_str = "Branch Taken"; break;
            case PROFILE_BRANCH_NOT_TAKEN: type_str = "Branch Not Taken"; break;
            case PROFILE_LOOP_ITERATION: type_str = "Loop Iteration"; break;
            case PROFILE_CACHE_MISS: type_str = "Cache Miss"; break;
            case PROFILE_MEMORY_ACCESS: type_str = "Memory Access"; break;
            default: type_str = "Unknown"; break;
        }
        
        uint64_t avg_cycles = point->total_cycles / point->hit_count;
        
        printf("%-40s %-20s %10lu %15lu %15lu %15lu\n",
               point->function_name, type_str, point->hit_count,
               avg_cycles, point->min_cycles, point->max_cycles);
    }
    
    printf("========================================\n");
}

int autoprofiler_export_pgo(const char* output_dir) {
    if (!g_autoprofiler.enabled) {
        fprintf(stderr, "Autoprofiler not initialized\n");
        return -1;
    }
    
    const char* dir = output_dir ? output_dir : "./pgo-data";
    
    /* Create output directory */
    if (create_directory(dir) != 0) {
        return -1;
    }
    
    /* Export in GCC PGO format */
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/autoprofiler.gcda", dir);
    
    /* Note: This is a simplified export. Real GCC .gcda format is more complex */
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to create %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    /* Write basic profiling data */
    for (uint32_t i = 0; i < g_autoprofiler.num_points; i++) {
        profile_point_t* point = &g_autoprofiler.points[i];
        fwrite(&point->hit_count, sizeof(point->hit_count), 1, fp);
    }
    
    fclose(fp);
    
    printf("PGO data exported to %s\n", filename);
    return 0;
}

int autoprofiler_merge_pgo_data(const char* profile_dir) {
    /* This would merge multiple .gcda files from different runs */
    /* Implementation depends on GCC's gcov format */
    
    printf("PGO data merge not yet implemented\n");
    return 0;
}

/* ============================================================================
 * GCC PGO Integration Stubs
 * ============================================================================ */

void __gcov_init(void* info) {
    (void)info;
    /* Called by GCC instrumented code */
}

void __gcov_flush(void) {
    /* Called at program exit to flush PGO data */
    if (g_autoprofiler.enabled && g_autoprofiler.collecting) {
        autoprofiler_stop();
        autoprofiler_save(NULL);
    }
}
