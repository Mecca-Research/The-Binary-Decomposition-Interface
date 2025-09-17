
// ===================================================================
// DESC: Virtual File System (VFS) layer implementation for BDI Kernel
//       Provides unified interface for multiple filesystem types
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// VFS Constants
#define VFS_MAX_PATH        4096
#define VFS_MAX_NAME        256
#define VFS_MAX_FILESYSTEMS 16
#define VFS_MAX_MOUNTS      32

// File types
#define VFS_TYPE_REGULAR    1
#define VFS_TYPE_DIRECTORY  2
#define VFS_TYPE_SYMLINK    3
#define VFS_TYPE_DEVICE     4

// File permissions
#define VFS_PERM_READ       0x01
#define VFS_PERM_WRITE      0x02
#define VFS_PERM_EXECUTE    0x04

// Forward declarations
typedef struct vfs_inode vfs_inode_t;
typedef struct vfs_file vfs_file_t;
typedef struct vfs_dentry vfs_dentry_t;
typedef struct vfs_superblock vfs_superblock_t;
typedef struct vfs_filesystem vfs_filesystem_t;
typedef struct vfs_mount vfs_mount_t;

// VFS operations structures
typedef struct {
    int (*read)(vfs_file_t *file, void *buffer, size_t size, size_t offset);
    int (*write)(vfs_file_t *file, const void *buffer, size_t size, size_t offset);
    int (*open)(vfs_inode_t *inode, vfs_file_t *file);
    int (*close)(vfs_file_t *file);
    int (*seek)(vfs_file_t *file, size_t offset, int whence);
} vfs_file_operations_t;

typedef struct {
    int (*create)(vfs_inode_t *dir, vfs_dentry_t *dentry, int mode);
    int (*lookup)(vfs_inode_t *dir, vfs_dentry_t *dentry);
    int (*unlink)(vfs_inode_t *dir, vfs_dentry_t *dentry);
    int (*mkdir)(vfs_inode_t *dir, vfs_dentry_t *dentry, int mode);
    int (*rmdir)(vfs_inode_t *dir, vfs_dentry_t *dentry);
} vfs_inode_operations_t;

typedef struct {
    int (*read_super)(vfs_superblock_t *sb, void *data);
    int (*write_super)(vfs_superblock_t *sb);
    int (*alloc_inode)(vfs_superblock_t *sb);
    int (*destroy_inode)(vfs_inode_t *inode);
} vfs_super_operations_t;

// VFS data structures
struct vfs_inode {
    uint32_t i_ino;                     // Inode number
    uint16_t i_mode;                    // File mode and permissions
    uint16_t i_uid;                     // User ID
    uint16_t i_gid;                     // Group ID
    uint32_t i_size;                    // File size
    uint32_t i_atime;                   // Access time
    uint32_t i_mtime;                   // Modification time
    uint32_t i_ctime;                   // Creation time
    uint32_t i_blocks;                  // Number of blocks
    uint32_t i_nlink;                   // Number of hard links
    vfs_inode_operations_t *i_op;       // Inode operations
    vfs_file_operations_t *i_fop;       // File operations
    vfs_superblock_t *i_sb;             // Superblock
    void *i_private;                    // Filesystem-specific data
};

struct vfs_file {
    vfs_dentry_t *f_dentry;             // Directory entry
    vfs_file_operations_t *f_op;        // File operations
    uint32_t f_flags;                   // File flags
    uint32_t f_mode;                    // File mode
    size_t f_pos;                       // Current file position
    void *private_data;                 // Private data
};

struct vfs_dentry {
    char d_name[VFS_MAX_NAME];          // Entry name
    vfs_inode_t *d_inode;               // Associated inode
    vfs_dentry_t *d_parent;             // Parent directory
    vfs_dentry_t *d_child;              // First child
    vfs_dentry_t *d_sibling;            // Next sibling
    uint32_t d_flags;                   // Dentry flags
};

struct vfs_superblock {
    uint32_t s_blocksize;               // Block size
    uint32_t s_maxbytes;                // Maximum file size
    vfs_filesystem_t *s_type;           // Filesystem type
    vfs_super_operations_t *s_op;       // Superblock operations
    vfs_dentry_t *s_root;               // Root directory entry
    void *s_fs_info;                    // Filesystem-specific info
    uint32_t s_flags;                   // Mount flags
};

struct vfs_filesystem {
    char name[VFS_MAX_NAME];            // Filesystem name
    int (*mount)(vfs_superblock_t *sb, void *data); // Mount function
    int (*unmount)(vfs_superblock_t *sb);           // Unmount function
    vfs_filesystem_t *next;             // Next in list
};

struct vfs_mount {
    vfs_dentry_t *mnt_mountpoint;       // Mount point
    vfs_dentry_t *mnt_root;             // Root of mounted fs
    vfs_superblock_t *mnt_sb;           // Superblock
    char mnt_devname[VFS_MAX_NAME];     // Device name
    vfs_mount_t *next;                  // Next in list
};

// Global VFS state
static vfs_filesystem_t *g_filesystems = NULL;
static vfs_mount_t *g_mounts = NULL;
static vfs_dentry_t *g_root_dentry = NULL;
static uint32_t g_next_inode = 1;

// Function prototypes
int vfs_init(void);
int vfs_register_filesystem(vfs_filesystem_t *fs);
int vfs_unregister_filesystem(const char *name);
int vfs_mount(const char *dev_name, const char *dir_name, const char *type, void *data);
int vfs_unmount(const char *dir_name);
vfs_file_t *vfs_open(const char *path, int flags);
int vfs_close(vfs_file_t *file);
int vfs_read(vfs_file_t *file, void *buffer, size_t size);
int vfs_write(vfs_file_t *file, const void *buffer, size_t size);
int vfs_seek(vfs_file_t *file, size_t offset, int whence);
int vfs_create(const char *path, int mode);
int vfs_unlink(const char *path);
int vfs_mkdir(const char *path, int mode);
int vfs_rmdir(const char *path);
vfs_dentry_t *vfs_lookup(const char *path);
vfs_inode_t *vfs_alloc_inode(vfs_superblock_t *sb);
void vfs_destroy_inode(vfs_inode_t *inode);

/**
 * Initialize the VFS layer
 */
int vfs_init(void) {
    // Create root directory entry
    g_root_dentry = (vfs_dentry_t *)malloc(sizeof(vfs_dentry_t));
    if (!g_root_dentry) {
        return -1;
    }
    
    memset(g_root_dentry, 0, sizeof(vfs_dentry_t));
    strcpy(g_root_dentry->d_name, "/");
    
    // Create root inode
    g_root_dentry->d_inode = (vfs_inode_t *)malloc(sizeof(vfs_inode_t));
    if (!g_root_dentry->d_inode) {
        free(g_root_dentry);
        return -1;
    }
    
    memset(g_root_dentry->d_inode, 0, sizeof(vfs_inode_t));
    g_root_dentry->d_inode->i_ino = g_next_inode++;
    g_root_dentry->d_inode->i_mode = VFS_TYPE_DIRECTORY | 0755;
    g_root_dentry->d_inode->i_nlink = 2; // . and ..
    
    return 0;
}

/**
 * Register a filesystem type
 */
int vfs_register_filesystem(vfs_filesystem_t *fs) {
    if (!fs || !fs->name[0]) {
        return -1;
    }
    
    // Check if already registered
    vfs_filesystem_t *current = g_filesystems;
    while (current) {
        if (strcmp(current->name, fs->name) == 0) {
            return -1; // Already exists
        }
        current = current->next;
    }
    
    // Add to list
    fs->next = g_filesystems;
    g_filesystems = fs;
    
    return 0;
}

/**
 * Unregister a filesystem type
 */
int vfs_unregister_filesystem(const char *name) {
    if (!name) {
        return -1;
    }
    
    vfs_filesystem_t **current = &g_filesystems;
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            vfs_filesystem_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1; // Not found
}

/**
 * Mount a filesystem
 */
int vfs_mount(const char *dev_name, const char *dir_name, const char *type, void *data) {
    if (!dev_name || !dir_name || !type) {
        return -1;
    }
    
    // Find filesystem type
    vfs_filesystem_t *fs = g_filesystems;
    while (fs) {
        if (strcmp(fs->name, type) == 0) {
            break;
        }
        fs = fs->next;
    }
    
    if (!fs) {
        return -1; // Filesystem type not found
    }
    
    // Find mount point
    vfs_dentry_t *mountpoint = vfs_lookup(dir_name);
    if (!mountpoint) {
        return -1; // Mount point not found
    }
    
    // Create mount structure
    vfs_mount_t *mount = (vfs_mount_t *)malloc(sizeof(vfs_mount_t));
    if (!mount) {
        return -1;
    }
    
    memset(mount, 0, sizeof(vfs_mount_t));
    mount->mnt_mountpoint = mountpoint;
    strncpy(mount->mnt_devname, dev_name, VFS_MAX_NAME - 1);
    
    // Create superblock
    mount->mnt_sb = (vfs_superblock_t *)malloc(sizeof(vfs_superblock_t));
    if (!mount->mnt_sb) {
        free(mount);
        return -1;
    }
    
    memset(mount->mnt_sb, 0, sizeof(vfs_superblock_t));
    mount->mnt_sb->s_type = fs;
    
    // Call filesystem mount function
    if (fs->mount && fs->mount(mount->mnt_sb, data) != 0) {
        free(mount->mnt_sb);
        free(mount);
        return -1;
    }
    
    // Add to mount list
    mount->next = g_mounts;
    g_mounts = mount;
    
    return 0;
}

/**
 * Unmount a filesystem
 */
int vfs_unmount(const char *dir_name) {
    if (!dir_name) {
        return -1;
    }
    
    vfs_mount_t **current = &g_mounts;
    while (*current) {
        // In a real implementation, we'd compare the mount point path
        vfs_mount_t *to_remove = *current;
        *current = (*current)->next;
        
        // Call filesystem unmount function
        if (to_remove->mnt_sb->s_type->unmount) {
            to_remove->mnt_sb->s_type->unmount(to_remove->mnt_sb);
        }
        
        free(to_remove->mnt_sb);
        free(to_remove);
        return 0;
    }
    
    return -1; // Not found
}

/**
 * Open a file
 */
vfs_file_t *vfs_open(const char *path, int flags) {
    if (!path) {
        return NULL;
    }
    
    // Look up the file
    vfs_dentry_t *dentry = vfs_lookup(path);
    if (!dentry) {
        return NULL;
    }
    
    // Allocate file structure
    vfs_file_t *file = (vfs_file_t *)malloc(sizeof(vfs_file_t));
    if (!file) {
        return NULL;
    }
    
    memset(file, 0, sizeof(vfs_file_t));
    file->f_dentry = dentry;
    file->f_op = dentry->d_inode->i_fop;
    file->f_flags = flags;
    file->f_pos = 0;
    
    // Call filesystem open function
    if (file->f_op && file->f_op->open) {
        if (file->f_op->open(dentry->d_inode, file) != 0) {
            free(file);
            return NULL;
        }
    }
    
    return file;
}

/**
 * Close a file
 */
int vfs_close(vfs_file_t *file) {
    if (!file) {
        return -1;
    }
    
    // Call filesystem close function
    if (file->f_op && file->f_op->close) {
        file->f_op->close(file);
    }
    
    free(file);
    return 0;
}

/**
 * Read from a file
 */
int vfs_read(vfs_file_t *file, void *buffer, size_t size) {
    if (!file || !buffer) {
        return -1;
    }
    
    if (file->f_op && file->f_op->read) {
        int result = file->f_op->read(file, buffer, size, file->f_pos);
        if (result > 0) {
            file->f_pos += result;
        }
        return result;
    }
    
    return -1;
}

/**
 * Write to a file
 */
int vfs_write(vfs_file_t *file, const void *buffer, size_t size) {
    if (!file || !buffer) {
        return -1;
    }
    
    if (file->f_op && file->f_op->write) {
        int result = file->f_op->write(file, buffer, size, file->f_pos);
        if (result > 0) {
            file->f_pos += result;
        }
        return result;
    }
    
    return -1;
}

/**
 * Seek in a file
 */
int vfs_seek(vfs_file_t *file, size_t offset, int whence) {
    if (!file) {
        return -1;
    }
    
    if (file->f_op && file->f_op->seek) {
        return file->f_op->seek(file, offset, whence);
    }
    
    // Default seek implementation
    switch (whence) {
        case 0: // SEEK_SET
            file->f_pos = offset;
            break;
        case 1: // SEEK_CUR
            file->f_pos += offset;
            break;
        case 2: // SEEK_END
            file->f_pos = file->f_dentry->d_inode->i_size + offset;
            break;
        default:
            return -1;
    }
    
    return file->f_pos;
}

/**
 * Look up a path in the filesystem
 */
vfs_dentry_t *vfs_lookup(const char *path) {
    if (!path || path[0] != '/') {
        return NULL;
    }
    
    // Start from root
    vfs_dentry_t *current = g_root_dentry;
    
    // Handle root path
    if (strcmp(path, "/") == 0) {
        return current;
    }
    
    // Parse path components
    char *path_copy = strdup(path + 1); // Skip leading '/'
    if (!path_copy) {
        return NULL;
    }
    
    char *token = strtok(path_copy, "/");
    while (token && current) {
        // Look for child with matching name
        vfs_dentry_t *child = current->d_child;
        while (child) {
            if (strcmp(child->d_name, token) == 0) {
                current = child;
                break;
            }
            child = child->d_sibling;
        }
        
        if (!child) {
            current = NULL; // Not found
            break;
        }
        
        token = strtok(NULL, "/");
    }
    
    free(path_copy);
    return current;
}

/**
 * Allocate a new inode
 */
vfs_inode_t *vfs_alloc_inode(vfs_superblock_t *sb) {
    vfs_inode_t *inode = (vfs_inode_t *)malloc(sizeof(vfs_inode_t));
    if (!inode) {
        return NULL;
    }
    
    memset(inode, 0, sizeof(vfs_inode_t));
    inode->i_ino = g_next_inode++;
    inode->i_sb = sb;
    inode->i_nlink = 1;
    
    return inode;
}

/**
 * Destroy an inode
 */
void vfs_destroy_inode(vfs_inode_t *inode) {
    if (inode) {
        free(inode);
    }
}

/**
 * Cleanup VFS layer
 */
void vfs_cleanup(void) {
    // Free mount list
    while (g_mounts) {
        vfs_mount_t *mount = g_mounts;
        g_mounts = g_mounts->next;
        free(mount->mnt_sb);
        free(mount);
    }
    
    // Free filesystem list
    while (g_filesystems) {
        vfs_filesystem_t *fs = g_filesystems;
        g_filesystems = g_filesystems->next;
        free(fs);
    }
    
    // Free root dentry and inode
    if (g_root_dentry) {
        if (g_root_dentry->d_inode) {
            free(g_root_dentry->d_inode);
        }
        free(g_root_dentry);
        g_root_dentry = NULL;
    }
}
