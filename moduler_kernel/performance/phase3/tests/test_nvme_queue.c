/**
 * @file test_nvme_queue.c
 * @brief Unit tests for NVMe queue pair management
 * 
 * Tests include:
 * - Basic queue creation and destruction
 * - Command submission and completion processing
 * - Queue full detection
 * - Submission queue head update from completions (critical bug test)
 * - Wrap-around handling
 */

#include "../nvme/nvme_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

// Mock device structure for testing
static nvme_device_t* create_mock_device(void) {
    nvme_device_t* dev = calloc(1, sizeof(nvme_device_t));
    if (!dev) return NULL;
    
    // Mock BAR0 for doorbell access
    dev->bar0 = calloc(1, 8192);
    if (!dev->bar0) {
        free(dev);
        return NULL;
    }
    
    dev->caps.doorbell_stride = 4;
    dev->numa_node = 0;
    
    return dev;
}

static void destroy_mock_device(nvme_device_t* dev) {
    if (dev) {
        free(dev->bar0);
        free(dev);
    }
}

// Test completion callback
static int completion_count = 0;
static void test_completion_cb(void* ctx, const nvme_completion_t* cpl) {
    completion_count++;
    printf("  Completion %d: CID=%u, sq_head=%u, status=0x%04x\n",
           completion_count, cpl->cid, cpl->sq_head, cpl->status);
}

/**
 * Test 1: Basic queue creation and destruction
 */
static void test_queue_creation(void) {
    printf("\n=== Test 1: Queue Creation ===\n");
    
    nvme_device_t* dev = create_mock_device();
    assert(dev != NULL);
    
    // Create admin queue pair
    nvme_qpair_t* admin_qp = nvme_create_admin_qpair(dev, 64, 64);
    assert(admin_qp != NULL);
    assert(admin_qp->qid == 0);
    assert(admin_qp->sq_size == 64);
    assert(admin_qp->cq_size == 64);
    
    // Create I/O queue pair
    nvme_qpair_t* io_qp = nvme_create_io_qpair(dev, 1, 128, 128);
    assert(io_qp != NULL);
    assert(io_qp->qid == 1);
    assert(io_qp->sq_size == 128);
    assert(io_qp->cq_size == 128);
    
    nvme_destroy_qpair(admin_qp);
    nvme_destroy_qpair(io_qp);
    destroy_mock_device(dev);
    
    printf("✓ Queue creation test passed\n");
}

/**
 * Test 2: Command submission
 */
static void test_command_submission(void) {
    printf("\n=== Test 2: Command Submission ===\n");
    
    nvme_device_t* dev = create_mock_device();
    nvme_qpair_t* qp = nvme_create_io_qpair(dev, 1, 64, 64);
    
    // Submit a few commands
    nvme_command_t cmd = {0};
    cmd.cdw0 = 0x02; // Read command
    
    int cid1 = nvme_submit_command(qp, &cmd, (void*)0x1234);
    assert(cid1 >= 0);
    printf("  Submitted command, CID=%d\n", cid1);
    
    int cid2 = nvme_submit_command(qp, &cmd, (void*)0x5678);
    assert(cid2 >= 0);
    printf("  Submitted command, CID=%d\n", cid2);
    
    // Check queue state
    assert(qp->sq_tail == 2);
    assert(qp->num_outstanding == 2);
    
    uint64_t submissions, completions, errors;
    nvme_qpair_get_stats(qp, &submissions, &completions, &errors);
    assert(submissions == 2);
    assert(completions == 0);
    
    nvme_destroy_qpair(qp);
    destroy_mock_device(dev);
    
    printf("✓ Command submission test passed\n");
}

/**
 * Test 3: CRITICAL - Submission queue head update from completions
 * 
 * This test specifically catches the bug where sq_head is not updated
 * from completion entries, causing the queue to appear full after
 * ~sq_size-1 submissions.
 */
static void test_sq_head_update_from_completions(void) {
    printf("\n=== Test 3: CRITICAL - SQ Head Update from Completions ===\n");
    
    nvme_device_t* dev = create_mock_device();
    nvme_qpair_t* qp = nvme_create_io_qpair(dev, 1, 16, 16);
    
    printf("  Queue size: %u entries\n", qp->sq_size);
    printf("  Initial sq_head=%u, sq_tail=%u\n", qp->sq_head, qp->sq_tail);
    
    // Submit commands to fill most of the queue
    nvme_command_t cmd = {0};
    cmd.cdw0 = 0x02; // Read command
    
    int num_initial_cmds = 10;
    printf("  Submitting %d commands...\n", num_initial_cmds);
    for (int i = 0; i < num_initial_cmds; i++) {
        int cid = nvme_submit_command(qp, &cmd, (void*)(uintptr_t)i);
        assert(cid >= 0);
    }
    
    printf("  After submissions: sq_head=%u, sq_tail=%u, outstanding=%u\n",
           qp->sq_head, qp->sq_tail, qp->num_outstanding);
    printf("  Free entries: %u\n", nvme_qpair_get_free_entries(qp));
    
    // Simulate completions by manually creating completion entries
    // In real hardware, the controller would write these
    printf("  Simulating %d completions...\n", 5);
    for (int i = 0; i < 5; i++) {
        nvme_completion_t* cpl = &qp->cq[i];
        cpl->cid = i;
        cpl->sq_head = i + 1; // Controller has consumed up to this SQ entry
        cpl->sq_id = qp->qid;
        cpl->status = (qp->cq_phase << 0) | (0 << 1); // Success, correct phase
    }
    
    // Process completions
    completion_count = 0;
    int processed = nvme_process_completions(qp, 5, test_completion_cb);
    assert(processed == 5);
    
    printf("  After completions: sq_head=%u, sq_tail=%u, outstanding=%u\n",
           qp->sq_head, qp->sq_tail, qp->num_outstanding);
    printf("  Free entries: %u\n", nvme_qpair_get_free_entries(qp));
    
    // CRITICAL CHECK: sq_head should have been updated from completion entries
    // Without the fix, sq_head would still be 0, causing the queue to appear full
    assert(qp->sq_head == 5); // Should match last completion's sq_head
    
    // Verify we can submit more commands now
    printf("  Attempting to submit more commands...\n");
    for (int i = 0; i < 5; i++) {
        int cid = nvme_submit_command(qp, &cmd, (void*)(uintptr_t)(100 + i));
        assert(cid >= 0); // Should succeed because sq_head was updated
        printf("    Submitted command %d, CID=%d\n", i, cid);
    }
    
    printf("  Final state: sq_head=%u, sq_tail=%u, outstanding=%u\n",
           qp->sq_head, qp->sq_tail, qp->num_outstanding);
    
    nvme_destroy_qpair(qp);
    destroy_mock_device(dev);
    
    printf("✓ SQ head update test passed - Bug is FIXED!\n");
}

/**
 * Test 4: Queue full detection and recovery
 * 
 * Tests that the queue correctly detects when it's full and can accept
 * new commands after processing completions.
 */
static void test_queue_full_and_recovery(void) {
    printf("\n=== Test 4: Queue Full Detection and Recovery ===\n");
    
    nvme_device_t* dev = create_mock_device();
    nvme_qpair_t* qp = nvme_create_io_qpair(dev, 1, 8, 8);
    
    nvme_command_t cmd = {0};
    cmd.cdw0 = 0x02;
    
    // Fill the queue (size - 1 because we reserve one entry)
    printf("  Filling queue (size=%u)...\n", qp->sq_size);
    int submitted = 0;
    for (int i = 0; i < qp->sq_size; i++) {
        int cid = nvme_submit_command(qp, &cmd, (void*)(uintptr_t)i);
        if (cid >= 0) {
            submitted++;
        } else {
            printf("  Queue full after %d submissions\n", submitted);
            break;
        }
    }
    
    // Try to submit one more - should fail
    int cid = nvme_submit_command(qp, &cmd, (void*)0xDEAD);
    assert(cid == -EAGAIN); // Should fail with EAGAIN
    printf("  ✓ Correctly rejected submission when full\n");
    
    // Simulate completions
    printf("  Simulating 4 completions...\n");
    for (int i = 0; i < 4; i++) {
        nvme_completion_t* cpl = &qp->cq[i];
        cpl->cid = i;
        cpl->sq_head = i + 1;
        cpl->sq_id = qp->qid;
        cpl->status = (qp->cq_phase << 0) | (0 << 1);
    }
    
    completion_count = 0;
    int processed = nvme_process_completions(qp, 4, test_completion_cb);
    assert(processed == 4);
    
    // Now we should be able to submit more commands
    printf("  Attempting to submit after completions...\n");
    for (int i = 0; i < 4; i++) {
        cid = nvme_submit_command(qp, &cmd, (void*)(uintptr_t)(100 + i));
        assert(cid >= 0); // Should succeed
        printf("    Submitted command %d, CID=%d\n", i, cid);
    }
    
    nvme_destroy_qpair(qp);
    destroy_mock_device(dev);
    
    printf("✓ Queue full detection and recovery test passed\n");
}

/**
 * Test 5: Stress test - Submit more than queue size commands
 * 
 * This test ensures the queue can handle continuous submission and
 * completion cycles without deadlocking.
 */
static void test_continuous_submission(void) {
    printf("\n=== Test 5: Continuous Submission Stress Test ===\n");
    
    nvme_device_t* dev = create_mock_device();
    nvme_qpair_t* qp = nvme_create_io_qpair(dev, 1, 16, 16);
    
    nvme_command_t cmd = {0};
    cmd.cdw0 = 0x02;
    
    int total_to_submit = 100; // Much more than queue size
    int submitted = 0;
    int completed = 0;
    
    printf("  Attempting to submit %d commands (queue size=%u)...\n",
           total_to_submit, qp->sq_size);
    
    while (submitted < total_to_submit) {
        // Try to submit commands
        while (submitted < total_to_submit && !nvme_qpair_is_full(qp)) {
            int cid = nvme_submit_command(qp, &cmd, (void*)(uintptr_t)submitted);
            if (cid >= 0) {
                submitted++;
            } else {
                break;
            }
        }
        
        // Simulate some completions
        int to_complete = (submitted - completed) / 2; // Complete half
        if (to_complete > 0) {
            for (int i = 0; i < to_complete; i++) {
                int cpl_idx = completed % qp->cq_size;
                nvme_completion_t* cpl = &qp->cq[cpl_idx];
                cpl->cid = completed % qp->sq_size;
                cpl->sq_head = (completed + 1) % qp->sq_size;
                cpl->sq_id = qp->qid;
                
                // Handle phase bit wrap-around
                uint8_t expected_phase = ((completed / qp->cq_size) % 2) ? 0 : 1;
                cpl->status = (expected_phase << 0) | (0 << 1);
                
                completed++;
            }
            
            // Process completions
            int processed = nvme_process_completions(qp, to_complete, NULL);
            assert(processed == to_complete);
        }
        
        if (submitted % 20 == 0) {
            printf("  Progress: submitted=%d, completed=%d, outstanding=%u\n",
                   submitted, completed, qp->num_outstanding);
        }
    }
    
    printf("  Final: submitted=%d, completed=%d\n", submitted, completed);
    assert(submitted == total_to_submit);
    
    nvme_destroy_qpair(qp);
    destroy_mock_device(dev);
    
    printf("✓ Continuous submission stress test passed\n");
}

int main(void) {
    printf("========================================\n");
    printf("NVMe Queue Pair Unit Tests\n");
    printf("========================================\n");
    
    test_queue_creation();
    test_command_submission();
    test_sq_head_update_from_completions(); // CRITICAL BUG TEST
    test_queue_full_and_recovery();
    test_continuous_submission();
    
    printf("\n========================================\n");
    printf("All tests passed! ✓\n");
    printf("========================================\n");
    
    return 0;
}
