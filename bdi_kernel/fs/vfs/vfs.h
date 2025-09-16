
// ===================================================================
// DESC: Virtual File System - Unified interface for multiple filesystems
// ===================================================================
#ifndef AEON_VFS_H
#define AEON_VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- File System Types ---
#define VFS_FS_FAT32    1
#define VFS_FS_EXT2     2

// --- File Types ---
#define VFS_TYPE_FILE   1
#define VFS_TYPE_DIR    2
#define VFS_TYPE_LINK   3

// --- File Permissions ---
#define VFS_PERM_READ   0x01
#define VFS_PERM_WRITE  0x02
#define VFS_PERM_EXEC   0x04

// --- Seek Origins ---
#define VFS_SEEK_SET    0
#define VFS_SEEK_CUR    1
#define VFS_SEEK_END    2

// --- Forward Declarations ---
struct vfs_node;
struct vfs_filesystem;
struct vfs_mount;

// --- VFS Node (File/Directory) ---
typedef struct vfs_node {
    char name[256];
    uint32_t inode;
    uint32_t type;
    uint32_t permissions;
    uint64_t size;
    uint64_t created_time;
    uint64_t modified_time;
    uint64_t accessed_time;
    
    struct vfs_filesystem* fs;
    struct vfs_mount* mount;
    void* fs_data; // Filesystem-specific data
    
    // Operations
    int (*read)(struct vfs_node* node, uint64_t offset, uint64_t size, void* buffer);
    int (*write)(struct vfs_node* node, uint64_t offset, uint64_t size, const void* buffer);
    struct vfs_node* (*finddir)(struct vfs_node* node, const char* name);
    struct vfs_node** (*readdir)(struct vfs_node* node, uint32_t* count);
    int (*create)(struct vfs_node* parent, const char* name, uint32_t type);
    int (*unlink)(struct vfs_node* parent, const char* name);
    int (*mkdir)(struct vfs_node* parent, const char* name);
    int (*rmdir)(struct vfs_node* parent, const char* name);
} vfs_node_t;

// --- File System Operations ---
typedef struct vfs_filesystem {
    char name[32];
    uint32_t type;
    
    // Mount/unmount
    int (*mount)(struct vfs_mount* mount, void* device);
    int (*unmount)(struct vfs_mount* mount);
    
    // Node operations
    vfs_node_t* (*get_root)(struct vfs_mount* mount);
    int (*sync)(struct vfs_mount* mount);
    
    void* fs_data; // Filesystem-specific data
} vfs_filesystem_t;

// --- Mount Point ---
typedef struct vfs_mount {
    char path[256];
    vfs_filesystem_t* fs;
    vfs_node_t* root;
    void* device; // Storage device (NVMe/AHCI controller + partition info)
    bool read_only;
    void* mount_data; // Mount-specific data
} vfs_mount_t;

// --- File Descriptor ---
typedef struct {
    vfs_node_t* node;
    uint64_t position;
    uint32_t flags;
    bool in_use;
} vfs_fd_t;

// --- Device Interface ---
typedef struct {
    int (*read_sectors)(void* device, uint64_t lba, uint32_t count, void* buffer);
    int (*write_sectors)(void* device, uint64_t lba, uint32_t count, const void* buffer);
    int (*flush)(void* device);
    uint64_t sector_count;
    uint32_t sector_size;
} vfs_device_t;

// --- VFS Management ---
int vfs_init(void);
int vfs_register_filesystem(vfs_filesystem_t* fs);
int vfs_mount(const char* path, const char* fs_type, void* device, bool read_only);
int vfs_unmount(const char* path);
vfs_mount_t* vfs_find_mount(const char* path);

// --- File Operations ---
int vfs_open(const char* path, uint32_t flags);
int vfs_close(int fd);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);
int vfs_seek(int fd, int64_t offset, int origin);
int vfs_tell(int fd, uint64_t* position);

// --- Directory Operations ---
vfs_node_t* vfs_find_node(const char* path);
vfs_node_t** vfs_list_directory(const char* path, uint32_t* count);
int vfs_create_file(const char* path);
int vfs_create_directory(const char* path);
int vfs_delete_file(const char* path);
int vfs_delete_directory(const char* path);

// --- Utility Functions ---
char* vfs_normalize_path(const char* path);
char* vfs_get_filename(const char* path);
char* vfs_get_directory(const char* path);
bool vfs_path_exists(const char* path);

// --- Error Codes ---
#define VFS_SUCCESS         0
#define VFS_ERROR_NOT_FOUND -1
#define VFS_ERROR_INVALID   -2
#define VFS_ERROR_NO_MEMORY -3
#define VFS_ERROR_IO        -4
#define VFS_ERROR_EXISTS    -5
#define VFS_ERROR_NOT_DIR   -6
#define VFS_ERROR_IS_DIR    -7
#define VFS_ERROR_NO_SPACE  -8
#define VFS_ERROR_READ_ONLY -9

#endif // AEON_VFS_H
