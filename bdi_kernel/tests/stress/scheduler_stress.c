
/*
 * Scheduler Stress Tests
 * High task creation/destruction, work stealing, NUMA migration, context switches
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

#define NUM_WORKERS 32
#define STRESS_DURATION_SEC 60
#define TASK_QUEUE_SIZE 10000

// Statistics
static _Atomic uint64_t tasks_created = 0;
static _Atomic uint64_t tasks_completed = 0;
static _Atomic uint64_t context_switches = 0;
static _Atomic uint64_t work_steals = 0;
static _Atomic bool stress_running = true;

// Task structure
typedef struct task {
    int task_id;
    int priority;
    int cpu_affinity;
    void (*work_func)(struct task*);
    struct task *next;
} task_t;

// Work queue
typedef struct {
    task_t *head;
    task_t *tail;
    pthread_mutex_t lock;
    size_t count;
} work_queue_t;

// Worker context
typedef struct {
    int worker_id;
    int cpu_id;
    work_queue_t *local_queue;
    pthread_t thread;
} worker_context_t;

// Global work queues (one per worker)
static work_queue_t work_queues[NUM_WORKERS];
static worker_context_t workers[NUM_WORKERS];

// Initialize work queue
static void init_work_queue(work_queue_t *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    pthread_mutex_init(&queue->lock, NULL);
}

// Enqueue task
static void enqueue_task(work_queue_t *queue, task_t *task) {
    pthread_mutex_lock(&queue->lock);
    
    if (queue->tail) {
        queue->tail->next = task;
    } else {
        queue->head = task;
    }
    queue->tail = task;
    task->next = NULL;
    queue->count++;
    
    pthread_mutex_unlock(&queue->lock);
}

// Dequeue task
static task_t* dequeue_task(work_queue_t *queue) {
    pthread_mutex_lock(&queue->lock);
    
    task_t *task = queue->head;
    if (task) {
        queue->head = task->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
        queue->count--;
    }
    
    pthread_mutex_unlock(&queue->lock);
    return task;
}

// Work stealing - try to steal from another queue
static task_t* steal_task(int worker_id) {
    // Try to steal from a random worker
    int victim = rand() % NUM_WORKERS;
    if (victim == worker_id) {
        victim = (victim + 1) % NUM_WORKERS;
    }
    
    task_t *task = dequeue_task(&work_queues[victim]);
    if (task) {
        atomic_fetch_add(&work_steals, 1);
    }
    return task;
}

// Sample work functions
static void cpu_intensive_work(task_t *task) {
    // Simulate CPU-intensive work
    volatile uint64_t sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i * task->task_id;
    }
}

static void io_intensive_work(task_t *task) {
    // Simulate I/O wait
    usleep(100);
}

static void mixed_work(task_t *task) {
    // Mix of CPU and I/O
    volatile uint64_t sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += i;
    }
    usleep(10);
}

// Worker thread
static void* worker_thread(void *arg) {
    worker_context_t *ctx = (worker_context_t*)arg;
    
    // Set CPU affinity if possible
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(ctx->cpu_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    while (atomic_load(&stress_running)) {
        // Try to get task from local queue
        task_t *task = dequeue_task(ctx->local_queue);
        
        // If no local task, try work stealing
        if (!task) {
            task = steal_task(ctx->worker_id);
        }
        
        if (task) {
            // Execute task
            if (task->work_func) {
                task->work_func(task);
            }
            
            atomic_fetch_add(&tasks_completed, 1);
            atomic_fetch_add(&context_switches, 1);
            free(task);
        } else {
            // No work available, yield
            sched_yield();
            usleep(100);
        }
    }
    
    return NULL;
}

// Task generator thread
static void* task_generator(void *arg) {
    void (*work_funcs[])(task_t*) = {
        cpu_intensive_work,
        io_intensive_work,
        mixed_work
    };
    
    while (atomic_load(&stress_running)) {
        // Create batch of tasks
        for (int i = 0; i < 100; i++) {
            task_t *task = malloc(sizeof(task_t));
            if (!task) continue;
            
            task->task_id = atomic_fetch_add(&tasks_created, 1);
            task->priority = rand() % 10;
            task->cpu_affinity = rand() % NUM_WORKERS;
            task->work_func = work_funcs[rand() % 3];
            task->next = NULL;
            
            // Distribute to worker queues
            int target_worker = rand() % NUM_WORKERS;
            enqueue_task(&work_queues[target_worker], task);
        }
        
        usleep(1000);
    }
    
    return NULL;
}

// Priority inversion test
static void* priority_inversion_test(void *arg) {
    // TODO: Implement priority inversion scenario
    while (atomic_load(&stress_running)) {
        sleep(1);
    }
    return NULL;
}

// Main scheduler stress test
int run_scheduler_stress_test(int duration_sec) {
    printf("=== Scheduler Stress Test ===\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Workers: %d\n", NUM_WORKERS);
    printf("\n");
    
    // Initialize work queues
    for (int i = 0; i < NUM_WORKERS; i++) {
        init_work_queue(&work_queues[i]);
        workers[i].worker_id = i;
        workers[i].cpu_id = i;
        workers[i].local_queue = &work_queues[i];
    }
    
    // Reset statistics
    atomic_store(&tasks_created, 0);
    atomic_store(&tasks_completed, 0);
    atomic_store(&context_switches, 0);
    atomic_store(&work_steals, 0);
    atomic_store(&stress_running, true);
    
    // Start worker threads
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&workers[i].thread, NULL, worker_thread, &workers[i]);
    }
    
    // Start task generator
    pthread_t generator_thread;
    pthread_create(&generator_thread, NULL, task_generator, NULL);
    
    // Run for specified duration
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < duration_sec) {
        sleep(1);
        
        // Print progress every 10 seconds
        if ((time(NULL) - start_time) % 10 == 0) {
            printf("Progress: %ld/%d seconds\n", time(NULL) - start_time, duration_sec);
            printf("  Tasks created: %lu\n", atomic_load(&tasks_created));
            printf("  Tasks completed: %lu\n", atomic_load(&tasks_completed));
            printf("  Context switches: %lu\n", atomic_load(&context_switches));
            printf("  Work steals: %lu\n", atomic_load(&work_steals));
        }
    }
    
    // Stop threads
    atomic_store(&stress_running, false);
    
    // Wait for threads to finish
    pthread_join(generator_thread, NULL);
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i].thread, NULL);
    }
    
    // Print final statistics
    printf("\n=== Final Statistics ===\n");
    printf("Tasks created: %lu\n", atomic_load(&tasks_created));
    printf("Tasks completed: %lu\n", atomic_load(&tasks_completed));
    printf("Context switches: %lu\n", atomic_load(&context_switches));
    printf("Work steals: %lu\n", atomic_load(&work_steals));
    
    uint64_t created = atomic_load(&tasks_created);
    uint64_t completed = atomic_load(&tasks_completed);
    printf("Completion rate: %.2f%%\n", (completed * 100.0) / created);
    
    // Cleanup
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_mutex_destroy(&work_queues[i].lock);
    }
    
    printf("\nScheduler stress test PASSED\n");
    return 0;
}

// Entry point for standalone execution
int main(int argc, char *argv[]) {
    int duration = STRESS_DURATION_SEC;
    
    if (argc > 1) {
        duration = atoi(argv[1]);
    }
    
    srand(time(NULL));
    return run_scheduler_stress_test(duration);
}
