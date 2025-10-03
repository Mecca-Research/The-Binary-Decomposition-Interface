
#include "bdi_shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Built-in command handlers
static int cmd_help(char** args);
static int cmd_exit(char** args);
static int cmd_cd(char** args);
static int cmd_pwd(char** args);
static int cmd_history(char** args);
static int cmd_jobs(char** args);
static int cmd_fg(char** args);
static int cmd_bg(char** args);
static int cmd_clear(char** args);

// System monitoring commands (will integrate with subsystems)
static int cmd_ps(char** args);
static int cmd_top(char** args);
static int cmd_mem(char** args);
static int cmd_disk(char** args);
static int cmd_net(char** args);
static int cmd_devices(char** args);

// Built-in command table
typedef struct {
    const char* name;
    int (*handler)(char** args);
    const char* description;
} BuiltinCommand;

static const BuiltinCommand builtin_commands[] = {
    {"help", cmd_help, "Display available commands"},
    {"exit", cmd_exit, "Exit the shell"},
    {"cd", cmd_cd, "Change directory"},
    {"pwd", cmd_pwd, "Print working directory"},
    {"history", cmd_history, "Show command history"},
    {"jobs", cmd_jobs, "List background jobs"},
    {"fg", cmd_fg, "Bring job to foreground"},
    {"bg", cmd_bg, "Resume job in background"},
    {"clear", cmd_clear, "Clear screen"},
    {"ps", cmd_ps, "List processes"},
    {"top", cmd_top, "System monitor"},
    {"mem", cmd_mem, "Memory usage"},
    {"disk", cmd_disk, "Disk usage"},
    {"net", cmd_net, "Network status"},
    {"devices", cmd_devices, "List devices"},
    {NULL, NULL, NULL}
};

// Execute built-in command
int shell_execute_builtin(char** args) {
    if (args[0] == NULL) {
        return -1;
    }
    
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        if (strcmp(args[0], builtin_commands[i].name) == 0) {
            return builtin_commands[i].handler(args);
        }
    }
    
    return -1; // Not a built-in command
}

// Command implementations
static int cmd_help(char** args) {
    printf("BDI Shell - Available Commands:\n\n");
    
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        printf("  %-12s - %s\n", builtin_commands[i].name, 
               builtin_commands[i].description);
    }
    
    printf("\nUse Ctrl+C to interrupt, Ctrl+D to exit\n");
    printf("Use '&' at end of command to run in background\n");
    printf("Use '|' for pipes, '>' for output redirection\n");
    
    return 0;
}

static int cmd_exit(char** args) {
    shell_cleanup();
    exit(0);
}

static int cmd_cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "bdi: cd: missing argument\n");
        return -1;
    }
    
    if (chdir(args[1]) != 0) {
        perror("cd");
        return -1;
    }
    
    return 0;
}

static int cmd_pwd(char** args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return -1;
    }
}

static int cmd_history(char** args) {
    for (uint32_t i = 0; i < shell_state.history_count; i++) {
        const char* cmd = shell_get_history(i);
        if (cmd != NULL) {
            printf("%4u  %s\n", i + 1, cmd);
        }
    }
    return 0;
}

static int cmd_jobs(char** args) {
    shell_list_jobs();
    return 0;
}

static int cmd_fg(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "bdi: fg: missing job id\n");
        return -1;
    }
    
    uint32_t job_id = (uint32_t)atoi(args[1]);
    return shell_fg(job_id);
}

static int cmd_bg(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "bdi: bg: missing job id\n");
        return -1;
    }
    
    uint32_t job_id = (uint32_t)atoi(args[1]);
    return shell_bg(job_id);
}

static int cmd_clear(char** args) {
    printf("\033[2J\033[H");
    return 0;
}

// System monitoring commands (placeholders for now)
static int cmd_ps(char** args) {
    printf("Process list (integration with process management)\n");
    // TODO: Integrate with Phase 1 (Process Management)
    return 0;
}

static int cmd_top(char** args) {
    printf("System monitor (integration with scheduler)\n");
    // TODO: Integrate with Phase 2 (Scheduler)
    return 0;
}

static int cmd_mem(char** args) {
    printf("Memory usage (integration with memory management)\n");
    // TODO: Integrate with Phase 3 (Memory Management)
    return 0;
}

static int cmd_disk(char** args) {
    printf("Disk usage (integration with storage subsystem)\n");
    // TODO: Integrate with Phase 4 (Storage)
    return 0;
}

static int cmd_net(char** args) {
    printf("Network status (integration with networking stack)\n");
    // TODO: Integrate with Phase 7 (Networking)
    return 0;
}

static int cmd_devices(char** args) {
    printf("Device list (integration with device drivers)\n");
    // TODO: Integrate with Phase 9 (Device Drivers)
    return 0;
}
