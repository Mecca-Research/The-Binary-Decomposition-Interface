#include "net_device.h"
#include <stdlib.h>

int net_init(void) {
    return 0;
}

void net_shutdown(void) {
}

int net_probe_devices(net_device_t** devices, size_t max_devices) {
    (void)devices;
    (void)max_devices;
    return 0;
}
