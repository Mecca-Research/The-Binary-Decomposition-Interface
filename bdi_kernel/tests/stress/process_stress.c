
/*
 * Process Stress Tests
 * Fork bomb scenarios, rapid lifecycle, capability enforcement, IPC stress
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdint.h>

#define MAX_PROCESSES 1000
#define STRESS_DURATION_SEC 60

// Statistics
static _Atomic uint64_t processes_created = 0;
static _Atomic uint64_t processes_destroyed = 0;
static _Atomic uint64_t fork_failures = 0;
static _Atomic bool stress_running = true;

// Rapid process creation/destruction
static void* stress_process_lifecycle(void *arg) {
    int thread_id = *(int*)arg;
    
    while (atomic_load(&stress_running)) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process - do minimal work and exit
            usleep(1000);
            _exit(0);
        } else if (pid > 0) {
            // Parent process
            atomic_fetch_add(&processes_created, 1);
            
            // Wait for child
            int status;
            waitpid(pid, &status, 0);
            atomic_fetch_add(&processes_destroyed, 1);
        } else {
            // Fork failed
            atomic_fetch_add(&fork_failures, 1);
            usleep(10000); // Back off on failure
        }
        
        usleep(100);
    }
    
    return NULL;
}

// Process tree stress (controlled fork bomb)
static void* stress_process_tree(void *arg) {
    int max_depth = 5;
    int current_depth = 0;
    
    while (atomic_load(&stress_running) && current_depth < max_depth) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child - create another level
            current_depth++;
            usleep(10000);
            
            if (current_depth >= max_depth) {
                _exit(0);
            }
        } else if (pid > 0) {
            atomic_fetch_add(&processes_created, 1);
            
            // Wait for child tree to complete
            int status;
            waitpid(pid, &status, 0);
            atomic_fetch_add(&processes_destroyed, 1);
            break;
        } else {
            atomic_fetch_add(&fork_failures, 1);
            break;
        }
    }
    
    return NULL;
}

// IPC stress - pipes
static void* stress_pipe_ipc(void *arg) {
    while (atomic_load(&stress_running)) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            usleep(1000);
            continue;
        }
        
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child - write to pipe
            close(pipefd[0]);
            char buffer[1024];
            for (int i = 0; i < 100; i++) {
                write(pipefd[1], buffer, sizeof(buffer));
            }
            close(pipefd[1]);
            _exit(0);
        } else if (pid > 0) {
            // Parent - read from pipe
            close(pipefd[1]);
            char buffer[1024];
            while (read(pipefd[0], buffer, sizeof(buffer)) > 0) {
                // Consume data
            }
            close(pipefd[0]);
            
            atomic_fetch_add(&processes_created, 1);
            
            int status;
            waitpid(pid, &status, 0);
            atomic_fetch_add(&processes_destroyed, 1);
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
            atomic_fetch_add(&fork_failures, 1);
        }
        
        usleep(1000);
    }
    
    return NULL;
}

// Signal stress
static void signal_handler(int signum) {
    // Simple handler
}

static void* stress_signals(void *arg) {
    signal(SIGUSR1, signal_handler);
    
    while (atomic_load(&stress_running)) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child - wait for signal
            pause();
            _exit(0);
        } else if (pid > 0) {
            atomic_fetch_add(&processes_created, 1);
            
            // Send signal to child
            usleep(1000);
            kill(pid, SIGUSR1);
            
            int status;
            waitpid(pid, &status, 0);
            atomic_fetch_add(&processes_destroyed, 1);
        } else {
            atomic_fetch_add(&fork_failures, 1);
        }
        
        usleep(1000);
    }
    
    return NULL;
}

// Resource limit enforcement test
static void* stress_resource_limits(void *arg) {
    // TODO: Test process resource limits (CPU, memory, file descriptors)
    while (atomic_load(&stress_running)) {
        sleep(1);
    }
    return NULL;
}

// Main process stress test
int run_process_stress_test(int duration_sec) {
    printf("=== Process Stress Test ===\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("\n");
    fflush(stdout);  // Flush before forking to prevent duplicate output
    
    pthread_t threads[8];
    int thread_ids[8];
    
    // Reset statistics
    atomic_store(&processes_created, 0);
    atomic_store(&processes_destroyed, 0);
    atomic_store(&fork_failures, 0);
    atomic_store(&stress_running, true);
    
    // Start stress threads
    for (int i = 0; i < 2; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, stress_process_lifecycle, &thread_ids[i]);
    }
    
    pthread_create(&threads[2], NULL, stress_process_tree, NULL);
    pthread_create(&threads[3], NULL, stress_pipe_ipc, NULL);
    pthread_create(&threads[4], NULL, stress_signals, NULL);
    pthread_create(&threads[5], NULL, stress_resource_limits, NULL);
    
    // Run for specified duration
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < duration_sec) {
        sleep(1);
        
        // Print progress every 10 seconds
        if ((time(NULL) - start_time) % 10 == 0) {
            printf("Progress: %ld/%d seconds\n", time(NULL) - start_time, duration_sec);
            printf("  Processes created: %lu\n", atomic_load(&processes_created));
            printf("  Processes destroyed: %lu\n", atomic_load(&processes_destroyed));
            printf("  Fork failures: %lu\n", atomic_load(&fork_failures));
        }
    }
    
    // Stop threads
    atomic_store(&stress_running, false);
    
    // Wait for threads to finish
    for (int i = 0; i < 6; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print final statistics
    printf("\n=== Final Statistics ===\n");
    printf("Processes created: %lu\n", atomic_load(&processes_created));
    printf("Processes destroyed: %lu\n", atomic_load(&processes_destroyed));
    printf("Fork failures: %lu\n", atomic_load(&fork_failures));
    
    // Bug Fix: Add zero-check before division to prevent division by zero
    uint64_t failures = atomic_load(&fork_failures);
    uint64_t created = atomic_load(&processes_created);
    
    if (created == 0) {
        // All fork attempts failed
        if (failures > 0) {
            printf("\nERROR: All fork attempts failed (%lu failures)\n", failures);
            printf("This may indicate resource limits (ulimit -u) or system constraints.\n");
            return 1;
        }
        // No processes created and no failures - unusual but not an error
        printf("\nWARNING: No processes were created during the test.\n");
        return 0;
    } else if (failures > created * 0.1) {
        // High failure rate (>10%)
        printf("\nWARNING: High fork failure rate (%.2f%%)\n", 
               (failures * 100.0) / created);
        return 1;
    }
    
    printf("\nProcess stress test PASSED\n");
    return 0;
}

// Entry point for standalone execution
#ifndef TEST_RUNNER_BUILD
int main(int argc, char *argv[]) {
    int duration = STRESS_DURATION_SEC;
    
    if (argc > 1) {
        duration = atoi(argv[1]);
    }
    
    return run_process_stress_test(duration);
}
#endif
