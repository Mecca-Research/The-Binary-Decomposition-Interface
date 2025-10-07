
#ifndef BDI_PROFILE_SERIALIZER_H
#define BDI_PROFILE_SERIALIZER_H

#include "profile_data.h"
#include <stdbool.h>

// .bdi-profile format magic number
#define BDI_PROFILE_MAGIC 0x42444950  // "BDIP"
#define BDI_PROFILE_VERSION 1

// Save profile data to .bdi-profile file
bool profile_serializer_save(const ProfileData *data, const char *filename);

// Load profile data from .bdi-profile file
ProfileData* profile_serializer_load(const char *filename);

// Save raw profile session to file
bool profile_serializer_save_session(const ProfileSession *session, const char *filename);

// Load raw profile session from file
ProfileSession* profile_serializer_load_session(const char *filename);

#endif // BDI_PROFILE_SERIALIZER_H
