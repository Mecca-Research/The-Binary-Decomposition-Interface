
/**
 * @file device_manager_test.c
 * @brief Unit tests for Device Manager
 */

#include "../device_manager.h"
#include "../hotplug.h"
#include "../irq.h"
#include "../driver_interface.h"
#include "../device_class.h"
#include "../backend_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test device operations */
static int test_device_probe(struct device *dev) {
    printf("[Test] Probing device: %s\n", dev->name);
    return 0;
}

static int test_device_remove(struct device *dev) {
    printf("[Test] Removing device: %s\n", dev->name);
    return 0;
}

static const struct device_ops test_device_ops = {
    .probe = test_device_probe,
    .remove = test_device_remove,
    .suspend = nullptr,
    .resume = nullptr,
    .shutdown = nullptr
};

/* Test driver operations */
static int test_driver_match(struct device *dev) {
    return (dev->type == DEVICE_TYPE_BLOCK) ? 0 : -1;
}

static int test_driver_bind(struct device *dev) {
    printf("[Test] Binding driver to device: %s\n", dev->name);
    return 0;
}

static void test_driver_unbind(struct device *dev) {
    printf("[Test] Unbinding driver from device: %s\n", dev->name);
}

/* Test hotplug handler */
static int test_hotplug_handler(const struct hotplug_event *event, void *user_data) {
    printf("[Test] Hotplug event: type=%d, device=%s\n", 
           event->type, event->device ? event->device->name : "null");
    return 0;
}

/* Test IRQ handler */
static int test_irq_handler(uint32_t irq, void *dev_id) {
    printf("[Test] IRQ %u handled\n", irq);
    return IRQ_HANDLED;
}

/* Test IRQ chip operations */
static void test_irq_enable(uint32_t irq) {
    printf("[Test] Enabling IRQ %u\n", irq);
}

static void test_irq_disable(uint32_t irq) {
    printf("[Test] Disabling IRQ %u\n", irq);
}

static void test_irq_ack(uint32_t irq) {
    printf("[Test] Acknowledging IRQ %u\n", irq);
}

static void test_irq_eoi(uint32_t irq) {
    printf("[Test] End of interrupt %u\n", irq);
}

static const struct irq_chip test_irq_chip = {
    .name = "test_irq_chip",
    .enable = test_irq_enable,
    .disable = test_irq_disable,
    .mask = nullptr,
    .unmask = nullptr,
    .ack = test_irq_ack,
    .eoi = test_irq_eoi,
    .set_type = nullptr,
    .set_affinity = nullptr,
    .setup_msi = nullptr,
    .teardown_msi = nullptr
};

/**
 * @brief Test device manager initialization
 */
static void test_device_manager_init(void) {
    printf("\n=== Test: Device Manager Initialization ===\n");
    
    int result = device_manager_init();
    assert(result == 0);
    
    uint64_t total, active;
    device_manager_get_stats(&total, &active);
    assert(total == 0);
    assert(active == 0);
    
    printf("✓ Device manager initialized successfully\n");
}

/**
 * @brief Test device registration
 */
static void test_device_registration(void) {
    printf("\n=== Test: Device Registration ===\n");
    
    struct device *dev = (struct device *)malloc(sizeof(struct device));
    assert(dev != nullptr);
    
    memset(dev, 0, sizeof(struct device));
    snprintf(dev->name, DEVICE_NAME_MAX, "test_device_0");
    snprintf(dev->path, DEVICE_PATH_MAX, "/dev/test0");
    dev->type = DEVICE_TYPE_BLOCK;
    dev->ops = &test_device_ops;
    dev->numa_node = 0;
    
    int result = device_register(dev);
    assert(result == 0);
    assert(dev->id != 0);
    
    uint64_t total, active;
    device_manager_get_stats(&total, &active);
    assert(total == 1);
    assert(active == 1);
    
    printf("✓ Device registered successfully (ID: %lu)\n", dev->id);
}

/**
 * @brief Test device discovery
 */
static void test_device_discovery(void) {
    printf("\n=== Test: Device Discovery ===\n");
    
    struct device *dev = device_find_by_name("test_device_0");
    assert(dev != nullptr);
    printf("✓ Found device by name: %s\n", dev->name);
    device_put(dev);
    
    dev = device_find_by_path("/dev/test0");
    assert(dev != nullptr);
    printf("✓ Found device by path: %s\n", dev->path);
    device_put(dev);
    
    struct device *devices[10];
    size_t count = device_get_by_type(DEVICE_TYPE_BLOCK, devices, 10);
    assert(count >= 1);
    printf("✓ Found %zu block devices\n", count);
    
    for (size_t i = 0; i < count; i++) {
        device_put(devices[i]);
    }
}

/**
 * @brief Test hotplug subsystem
 */
static void test_hotplug(void) {
    printf("\n=== Test: Hotplug Subsystem ===\n");
    
    int result = hotplug_init();
    assert(result == 0);
    
    int handler_id = hotplug_register_handler(test_hotplug_handler, nullptr,
                                             HOTPLUG_PRIORITY_NORMAL,
                                             DEVICE_TYPE_UNKNOWN);
    assert(handler_id >= 0);
    printf("✓ Registered hotplug handler (ID: %d)\n", handler_id);
    
    /* Create and register a test device to trigger hotplug */
    struct device *dev = (struct device *)malloc(sizeof(struct device));
    assert(dev != nullptr);
    
    memset(dev, 0, sizeof(struct device));
    snprintf(dev->name, DEVICE_NAME_MAX, "hotplug_test_device");
    dev->type = DEVICE_TYPE_CHAR;
    dev->ops = &test_device_ops;
    
    result = device_register(dev);
    assert(result == 0);
    
    /* Process hotplug events */
    uint32_t processed = hotplug_process_events();
    printf("✓ Processed %u hotplug events\n", processed);
    
    hotplug_unregister_handler(handler_id);
}

/**
 * @brief Test IRQ subsystem
 */
static void test_irq_subsystem(void) {
    printf("\n=== Test: IRQ Subsystem ===\n");
    
    int result = irq_init(&test_irq_chip);
    assert(result == 0);
    
    result = irq_request(10, test_irq_handler, IRQ_FLAG_SHARED, "test_irq", nullptr);
    assert(result == 0);
    printf("✓ Requested IRQ 10\n");
    
    irq_enable(10);
    printf("✓ Enabled IRQ 10\n");
    
    irq_handle(10);
    printf("✓ Handled IRQ 10\n");
    
    uint64_t count;
    result = irq_get_stats(10, &count);
    assert(result == 0);
    assert(count == 1);
    printf("✓ IRQ 10 count: %lu\n", count);
    
    irq_free(10, nullptr);
    printf("✓ Freed IRQ 10\n");
}

/**
 * @brief Test device classes
 */
static void test_device_classes(void) {
    printf("\n=== Test: Device Classes ===\n");
    
    int result = device_classes_init();
    assert(result == 0);
    
    struct device_class *class = device_class_get(DEVICE_TYPE_BLOCK);
    assert(class != nullptr);
    printf("✓ Got block device class: %s\n", class->name);
    
    struct device *dev = device_find_by_name("test_device_0");
    assert(dev != nullptr);
    
    result = device_class_add_device(dev);
    assert(result == 0);
    printf("✓ Added device to class\n");
    
    device_put(dev);
}

/**
 * @brief Test backend integration
 */
static void test_backend_integration(void) {
    printf("\n=== Test: Backend Integration ===\n");
    
    int result = backend_integration_init();
    assert(result == 0);
    
    result = backend_set_default(DEVICE_TYPE_BLOCK, BACKEND_TYPE_CPU);
    assert(result == 0);
    printf("✓ Set default backend for block devices\n");
    
    struct device *dev = device_find_by_name("test_device_0");
    assert(dev != nullptr);
    
    result = backend_set_device_affinity(dev, BACKEND_TYPE_GPU);
    assert(result == 0);
    printf("✓ Set device backend affinity\n");
    
    device_put(dev);
}

/**
 * @brief Test DMA operations
 */
static void test_dma_operations(void) {
    printf("\n=== Test: DMA Operations ===\n");
    
    struct device *dev = device_find_by_name("test_device_0");
    assert(dev != nullptr);
    
    struct dma_buffer *buf = dma_alloc_buffer(dev, 4096, DMA_BIDIRECTIONAL);
    assert(buf != nullptr);
    printf("✓ Allocated DMA buffer (size: %zu)\n", buf->size);
    
    int result = dma_map_buffer(buf, DMA_TO_DEVICE);
    assert(result == 0);
    printf("✓ Mapped DMA buffer\n");
    
    dma_sync_for_cpu(buf);
    printf("✓ Synced DMA buffer for CPU\n");
    
    dma_unmap_buffer(buf);
    printf("✓ Unmapped DMA buffer\n");
    
    dma_free_buffer(buf);
    printf("✓ Freed DMA buffer\n");
    
    device_put(dev);
}

/**
 * @brief Main test function
 */
int main(void) {
    printf("========================================\n");
    printf("Device & Hardware Abstraction Layer Tests\n");
    printf("========================================\n");
    
    test_device_manager_init();
    test_device_registration();
    test_device_discovery();
    test_hotplug();
    test_irq_subsystem();
    test_device_classes();
    test_backend_integration();
    test_dma_operations();
    
    printf("\n========================================\n");
    printf("All tests passed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
