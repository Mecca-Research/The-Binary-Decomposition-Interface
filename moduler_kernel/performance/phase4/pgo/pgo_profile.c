#include "pgo_profile.h"
#include <stdio.h>
#include <stdlib.h>

int pgo_init(void) {
    return 0;
}

void pgo_shutdown(void) {
}

int pgo_start_profiling(const char* output_file) {
    (void)output_file;
    return 0;
}

int pgo_stop_profiling(void) {
    return 0;
}

int pgo_merge_profiles(const char** input_files, size_t num_files,
                       const char* output_file) {
    (void)input_files;
    (void)num_files;
    (void)output_file;
    return 0;
}
