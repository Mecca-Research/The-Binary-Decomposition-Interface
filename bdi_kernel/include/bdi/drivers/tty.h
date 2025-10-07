
#ifndef BDI_TTY_H
#define BDI_TTY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* TTY buffer sizes */
#define TTY_BUF_SIZE        4096
#define TTY_INPUT_BUF_SIZE  1024
#define TTY_OUTPUT_BUF_SIZE 4096

/* TTY flags */
#define TTY_FLAG_ECHO       (1 << 0)
#define TTY_FLAG_CANON      (1 << 1)
#define TTY_FLAG_ISIG       (1 << 2)
#define TTY_FLAG_RAW        (1 << 3)

/* Special characters */
#define TTY_CHAR_EOF        0x04  /* Ctrl-D */
#define TTY_CHAR_EOL        0x0A  /* \n */
#define TTY_CHAR_ERASE      0x08  /* Backspace */
#define TTY_CHAR_KILL       0x15  /* Ctrl-U */
#define TTY_CHAR_INTR       0x03  /* Ctrl-C */
#define TTY_CHAR_QUIT       0x1C  /* Ctrl-\ */
#define TTY_CHAR_SUSP       0x1A  /* Ctrl-Z */

/* Forward declarations */
struct tty_device;
struct tty_driver;
struct tty_ldisc;

/* TTY operations */
typedef struct tty_operations {
    int (*open)(struct tty_device *tty);
    int (*close)(struct tty_device *tty);
    ssize_t (*write)(struct tty_device *tty, const uint8_t *buf, size_t count);
    ssize_t (*read)(struct tty_device *tty, uint8_t *buf, size_t count);
    int (*ioctl)(struct tty_device *tty, uint32_t cmd, uintptr_t arg);
    void (*flush)(struct tty_device *tty);
    void (*hangup)(struct tty_device *tty);
} tty_operations_t;

/* Line discipline operations */
typedef struct tty_ldisc_ops {
    int (*open)(struct tty_device *tty);
    void (*close)(struct tty_device *tty);
    ssize_t (*read)(struct tty_device *tty, uint8_t *buf, size_t count);
    ssize_t (*write)(struct tty_device *tty, const uint8_t *buf, size_t count);
    int (*ioctl)(struct tty_device *tty, uint32_t cmd, uintptr_t arg);
    void (*receive_buf)(struct tty_device *tty, const uint8_t *buf, size_t count);
} tty_ldisc_ops_t;

/* Line discipline */
typedef struct tty_ldisc {
    const char *name;
    uint32_t id;
    tty_ldisc_ops_t *ops;
    void *private_data;
} tty_ldisc_t;

/* TTY buffer */
typedef struct tty_buffer {
    uint8_t *data;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
} tty_buffer_t;

/* TTY device */
typedef struct tty_device {
    uint32_t id;
    const char *name;
    struct tty_driver *driver;
    tty_ldisc_t *ldisc;
    tty_operations_t *ops;
    
    /* Buffers */
    tty_buffer_t input_buf;
    tty_buffer_t output_buf;
    
    /* Flags and state */
    uint32_t flags;
    bool is_open;
    uint32_t open_count;
    
    /* Process group and session */
    int32_t pgrp;
    int32_t session;
    
    /* Termios settings */
    struct termios termios;
    
    /* Private data */
    void *driver_data;
    void *ldisc_data;
} tty_device_t;

/* TTY driver */
typedef struct tty_driver {
    const char *name;
    uint32_t major;
    uint32_t minor_start;
    uint32_t num_devices;
    tty_device_t **devices;
    tty_operations_t *ops;
    struct tty_driver *next;
} tty_driver_t;

/* TTY subsystem API */
int tty_init(void);
int tty_register_driver(tty_driver_t *driver);
int tty_unregister_driver(tty_driver_t *driver);
tty_device_t *tty_alloc_device(tty_driver_t *driver, uint32_t index);
void tty_free_device(tty_device_t *tty);

/* TTY device operations */
int tty_open(tty_device_t *tty);
int tty_close(tty_device_t *tty);
ssize_t tty_write(tty_device_t *tty, const uint8_t *buf, size_t count);
ssize_t tty_read(tty_device_t *tty, uint8_t *buf, size_t count);
int tty_ioctl(tty_device_t *tty, uint32_t cmd, uintptr_t arg);

/* TTY buffer operations */
int tty_buffer_init(tty_buffer_t *buf, size_t size);
void tty_buffer_free(tty_buffer_t *buf);
size_t tty_buffer_write(tty_buffer_t *buf, const uint8_t *data, size_t count);
size_t tty_buffer_read(tty_buffer_t *buf, uint8_t *data, size_t count);
size_t tty_buffer_available(tty_buffer_t *buf);
size_t tty_buffer_space(tty_buffer_t *buf);
void tty_buffer_flush(tty_buffer_t *buf);

/* Line discipline */
int tty_register_ldisc(tty_ldisc_t *ldisc);
int tty_set_ldisc(tty_device_t *tty, uint32_t ldisc_id);
tty_ldisc_t *tty_get_ldisc(uint32_t ldisc_id);

/* Input processing */
void tty_input_char(tty_device_t *tty, uint8_t c);
void tty_input_string(tty_device_t *tty, const uint8_t *str, size_t len);

/* Output processing */
void tty_output_char(tty_device_t *tty, uint8_t c);
void tty_output_string(tty_device_t *tty, const uint8_t *str, size_t len);

/* Console interface */
tty_device_t *tty_get_console(void);
int tty_set_console(tty_device_t *tty);

#endif /* BDI_TTY_H */
