
#include "profile_serializer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t timestamp;
    uint64_t function_count;
    uint64_t total_execution_time_ns;
} ProfileFileHeader;

bool profile_serializer_save(const ProfileData *data, const char *filename) {
    if (!data || !filename) {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Write header
    ProfileFileHeader header = {
        .magic = BDI_PROFILE_MAGIC,
        .version = BDI_PROFILE_VERSION,
        .timestamp = (uint64_t)time(NULL),
        .function_count = data->function_count,
        .total_execution_time_ns = data->total_execution_time_ns
    };

    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    // Write function stats
    if (data->function_count > 0) {
        if (fwrite(data->function_stats, sizeof(FunctionStats), 
                   data->function_count, fp) != data->function_count) {
            fclose(fp);
            return false;
        }
    }

    // Write memory stats
    if (fwrite(&data->memory_stats, sizeof(MemoryStats), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    // Write cache stats
    if (fwrite(&data->cache_stats, sizeof(CacheStats), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    // Write branch stats
    if (fwrite(&data->branch_stats, sizeof(BranchStats), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}

ProfileData* profile_serializer_load(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    // Read header
    ProfileFileHeader header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    // Verify magic and version
    if (header.magic != BDI_PROFILE_MAGIC || header.version != BDI_PROFILE_VERSION) {
        fclose(fp);
        return NULL;
    }

    // Allocate profile data
    ProfileData *data = calloc(1, sizeof(ProfileData));
    if (!data) {
        fclose(fp);
        return NULL;
    }

    data->function_count = header.function_count;
    data->total_execution_time_ns = header.total_execution_time_ns;

    // Read function stats
    if (data->function_count > 0) {
        data->function_stats = calloc(data->function_count, sizeof(FunctionStats));
        if (!data->function_stats) {
            free(data);
            fclose(fp);
            return NULL;
        }

        if (fread(data->function_stats, sizeof(FunctionStats), 
                  data->function_count, fp) != data->function_count) {
            free(data->function_stats);
            free(data);
            fclose(fp);
            return NULL;
        }
    }

    // Read memory stats
    if (fread(&data->memory_stats, sizeof(MemoryStats), 1, fp) != 1) {
        free(data->function_stats);
        free(data);
        fclose(fp);
        return NULL;
    }

    // Read cache stats
    if (fread(&data->cache_stats, sizeof(CacheStats), 1, fp) != 1) {
        free(data->function_stats);
        free(data);
        fclose(fp);
        return NULL;
    }

    // Read branch stats
    if (fread(&data->branch_stats, sizeof(BranchStats), 1, fp) != 1) {
        free(data->function_stats);
        free(data);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return data;
}

bool profile_serializer_save_session(const ProfileSession *session, const char *filename) {
    if (!session || !filename) {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Write session metadata
    if (fwrite(&session->event_count, sizeof(size_t), 1, fp) != 1 ||
        fwrite(&session->start_time_ns, sizeof(uint64_t), 1, fp) != 1 ||
        fwrite(&session->end_time_ns, sizeof(uint64_t), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    // Write events
    if (session->event_count > 0) {
        if (fwrite(session->events, sizeof(ProfileEvent), 
                   session->event_count, fp) != session->event_count) {
            fclose(fp);
            return false;
        }
    }

    fclose(fp);
    return true;
}

ProfileSession* profile_serializer_load_session(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    ProfileSession *session = calloc(1, sizeof(ProfileSession));
    if (!session) {
        fclose(fp);
        return NULL;
    }

    // Read session metadata
    if (fread(&session->event_count, sizeof(size_t), 1, fp) != 1 ||
        fread(&session->start_time_ns, sizeof(uint64_t), 1, fp) != 1 ||
        fread(&session->end_time_ns, sizeof(uint64_t), 1, fp) != 1) {
        free(session);
        fclose(fp);
        return NULL;
    }

    // Read events
    if (session->event_count > 0) {
        session->events = calloc(session->event_count, sizeof(ProfileEvent));
        if (!session->events) {
            free(session);
            fclose(fp);
            return NULL;
        }

        if (fread(session->events, sizeof(ProfileEvent), 
                  session->event_count, fp) != session->event_count) {
            free(session->events);
            free(session);
            fclose(fp);
            return NULL;
        }
    }

    session->event_capacity = session->event_count;
    session->is_active = false;

    fclose(fp);
    return session;
}
