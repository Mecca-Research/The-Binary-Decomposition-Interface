
#include "bdi_shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

// Helper macro for safe iteration bounds
#define MIN(a, b) ((a) < (b) ? (a) : (b))

ShellState shell_state = {0};

// C23: Use nullptr instead of NULL
int shell_init(void) {
    memset(&shell_state, 0, sizeof(ShellState));
    shell_state.running = true;
    shell_state.history_count = 0;
    shell_state.history_index = 0;
    atomic_store(&shell_state.job_count, 0);
    
    // Set default prompt
    snprintf(shell_state.prompt, sizeof(shell_state.prompt), "bdi> ");
    
    // Get current directory
    if (getcwd(shell_state.current_dir, sizeof(shell_state.current_dir)) == NULL) {
        strcpy(shell_state.current_dir, "/");
    }
    
    printf("BDI Shell v1.0 - C23 Edition\n");
    printf("Type 'help' for available commands\n\n");
    
    return 0;
}

int shell_run(void) {
    char input[SHELL_MAX_COMMAND_LENGTH];
    
    while (shell_state.running) {
        // Print prompt
        printf("%s", shell_state.prompt);
        fflush(stdout);
        
        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Remove newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        // Skip empty commands
        if (strlen(input) == 0) {
            continue;
        }
        
        // Add to history
        shell_add_history(input);
        
        // Execute command
        shell_execute_command(input);
    }
    
    return 0;
}

void shell_cleanup(void) {
    shell_state.running = false;
    
    // Kill all background jobs
    uint32_t job_count = atomic_load(&shell_state.job_count);
    for (uint32_t i = 0; i < MIN(job_count, SHELL_MAX_JOBS); i++) {
        if (shell_state.jobs[i].is_running) {
            kill(shell_state.jobs[i].pid, SIGTERM);
        }
    }
    
    printf("\nBDI Shell terminated\n");
}

// Command history implementation
void shell_add_history(const char* command) {
    if (shell_state.history_count < SHELL_MAX_HISTORY) {
        uint32_t idx = shell_state.history_count;
        strncpy(shell_state.history[idx].command, command, SHELL_MAX_COMMAND_LENGTH - 1);
        shell_state.history[idx].timestamp = (uint64_t)time(NULL);
        shell_state.history_count++;
    } else {
        // Shift history
        memmove(&shell_state.history[0], &shell_state.history[1], 
                (SHELL_MAX_HISTORY - 1) * sizeof(ShellHistoryEntry));
        strncpy(shell_state.history[SHELL_MAX_HISTORY - 1].command, command, 
                SHELL_MAX_COMMAND_LENGTH - 1);
        shell_state.history[SHELL_MAX_HISTORY - 1].timestamp = (uint64_t)time(NULL);
    }
    shell_state.history_index = shell_state.history_count;
}

const char* shell_get_history(int offset) {
    if (offset < 0 || offset >= (int)shell_state.history_count) {
        return NULL;
    }
    return shell_state.history[offset].command;
}

void shell_clear_history(void) {
    shell_state.history_count = 0;
    shell_state.history_index = 0;
}

// Tab completion (basic implementation)
char* shell_tab_complete(const char* partial) {
    // TODO: Implement full tab completion
    // For now, return NULL
    return NULL;
}

// Command parsing
int shell_parse_command(const char* input, char** args) {
    char* input_copy = strdup(input);
    if (input_copy == NULL) {
        return -1;
    }
    
    int argc = 0;
    char* token = strtok(input_copy, " \t");
    
    while (token != NULL && argc < SHELL_MAX_ARGS - 1) {
        args[argc++] = strdup(token);
        token = strtok(NULL, " \t");
    }
    
    args[argc] = NULL;
    free(input_copy);
    
    return argc;
}

// Job control implementation
int shell_launch_job(char** args, bool background) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        if (background) {
            // Detach from terminal for background jobs
            setsid();
        }
        
        execvp(args[0], args);
        
        // If execvp returns, it failed
        fprintf(stderr, "bdi: command not found: %s\n", args[0]);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        if (background) {
            // Add to job list - atomically reserve a slot
            uint32_t slot = atomic_fetch_add(&shell_state.job_count, 1);
            if (slot < SHELL_MAX_JOBS) {
                // Slot is within bounds, use it
                shell_state.jobs[slot].job_id = slot + 1;
                shell_state.jobs[slot].pid = pid;
                strncpy(shell_state.jobs[slot].command, args[0], 
                        SHELL_MAX_COMMAND_LENGTH - 1);
                shell_state.jobs[slot].is_background = true;
                shell_state.jobs[slot].is_running = true;
                shell_state.jobs[slot].is_stopped = false;
                
                printf("[%u] %d\n", shell_state.jobs[slot].job_id, pid);
            } else {
                // Slot exceeds array bounds, job cannot be tracked
                fprintf(stderr, "bdi: warning: job limit reached, job %d not tracked\n", pid);
            }
        } else {
            // Wait for foreground job
            int status;
            waitpid(pid, &status, 0);
        }
        
        return 0;
    } else {
        perror("fork");
        return -1;
    }
}

int shell_fg(uint32_t job_id) {
    // Find job
    uint32_t job_count = atomic_load(&shell_state.job_count);
    for (uint32_t i = 0; i < MIN(job_count, SHELL_MAX_JOBS); i++) {
        if (shell_state.jobs[i].job_id == job_id) {
            if (shell_state.jobs[i].is_stopped) {
                // Resume stopped job
                kill(shell_state.jobs[i].pid, SIGCONT);
            }
            
            // Wait for job
            int status;
            waitpid(shell_state.jobs[i].pid, &status, 0);
            
            shell_state.jobs[i].is_running = false;
            return 0;
        }
    }
    
    fprintf(stderr, "bdi: fg: %u: no such job\n", job_id);
    return -1;
}

int shell_bg(uint32_t job_id) {
    // Find job
    uint32_t job_count = atomic_load(&shell_state.job_count);
    for (uint32_t i = 0; i < MIN(job_count, SHELL_MAX_JOBS); i++) {
        if (shell_state.jobs[i].job_id == job_id) {
            if (shell_state.jobs[i].is_stopped) {
                // Resume stopped job in background
                kill(shell_state.jobs[i].pid, SIGCONT);
                shell_state.jobs[i].is_stopped = false;
                shell_state.jobs[i].is_running = true;
                printf("[%u] %d continued\n", job_id, shell_state.jobs[i].pid);
                return 0;
            }
        }
    }
    
    fprintf(stderr, "bdi: bg: %u: no such job\n", job_id);
    return -1;
}

void shell_list_jobs(void) {
    uint32_t count = atomic_load(&shell_state.job_count);
    uint32_t safe_count = MIN(count, SHELL_MAX_JOBS);
    
    if (safe_count == 0) {
        printf("No jobs\n");
        return;
    }
    
    for (uint32_t i = 0; i < safe_count; i++) {
        if (shell_state.jobs[i].is_running || shell_state.jobs[i].is_stopped) {
            printf("[%u] %s %d %s\n",
                   shell_state.jobs[i].job_id,
                   shell_state.jobs[i].is_stopped ? "Stopped" : "Running",
                   shell_state.jobs[i].pid,
                   shell_state.jobs[i].command);
        }
    }
}

// Pipes and redirection
int shell_execute_pipeline(char* commands[], int num_commands) {
    int pipes[SHELL_MAX_PIPES][2];
    pid_t pids[SHELL_MAX_PIPES + 1];
    
    // Create pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return -1;
        }
    }
    
    // Execute commands
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {
            // Child process
            
            // Redirect input from previous pipe
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            
            // Redirect output to next pipe
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipes
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Parse and execute command
            char* args[SHELL_MAX_ARGS];
            shell_parse_command(commands[i], args);
            execvp(args[0], args);
            
            fprintf(stderr, "bdi: command not found: %s\n", args[0]);
            exit(1);
        }
    }
    
    // Close all pipes in parent
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    return 0;
}

int shell_redirect_io(const char* input_file, const char* output_file, bool append) {
    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);
        if (fd < 0) {
            perror("open");
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (output_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int fd = open(output_file, flags, 0644);
        if (fd < 0) {
            perror("open");
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    return 0;
}

// Command execution (will be extended in shell_commands.c)
int shell_execute_command(const char* command) {
    char* args[SHELL_MAX_ARGS];
    int argc = shell_parse_command(command, args);
    
    if (argc <= 0) {
        return -1;
    }
    
    // Check for background job
    bool background = false;
    if (strcmp(args[argc - 1], "&") == 0) {
        background = true;
        free(args[argc - 1]);
        args[argc - 1] = NULL;
        argc--;
    }
    
    // Check for built-in commands
    int result = shell_execute_builtin(args);
    if (result >= 0) {
        // Built-in command executed
        for (int i = 0; i < argc; i++) {
            free(args[i]);
        }
        return result;
    }
    
    // Launch as external command
    result = shell_launch_job(args, background);
    
    // Free args
    for (int i = 0; i < argc; i++) {
        free(args[i]);
    }
    
    return result;
}
