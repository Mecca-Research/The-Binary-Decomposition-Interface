
// ===================================================================
// DESC: BDI Shell - Interactive command-line interface for BDI Kernel
//       Provides user interaction with the BDI system
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Shell Constants
#define BDI_SHELL_MAX_INPUT         1024
#define BDI_SHELL_MAX_ARGS          64
#define BDI_SHELL_MAX_COMMANDS      128
#define BDI_SHELL_PROMPT            "bdi> "
#define BDI_SHELL_VERSION           "1.0.0"

// Command Return Codes
#define BDI_SHELL_SUCCESS           0
#define BDI_SHELL_ERROR             -1
#define BDI_SHELL_EXIT              1

// Built-in Command Structure
typedef struct {
    const char *name;           // Command name
    const char *description;    // Command description
    int (*handler)(int argc, char **argv); // Command handler function
} bdi_shell_command_t;

// Shell State Structure
typedef struct {
    char input_buffer[BDI_SHELL_MAX_INPUT];
    char *args[BDI_SHELL_MAX_ARGS];
    int argc;
    int running;
    int exit_code;
    char current_directory[256];
    char history[10][BDI_SHELL_MAX_INPUT];
    int history_count;
    int history_index;
} bdi_shell_state_t;

// Global shell state
static bdi_shell_state_t g_shell_state;
static int g_shell_initialized = 0;

// Built-in command handlers
int bdi_cmd_help(int argc, char **argv);
int bdi_cmd_version(int argc, char **argv);
int bdi_cmd_exit(int argc, char **argv);
int bdi_cmd_clear(int argc, char **argv);
int bdi_cmd_echo(int argc, char **argv);
int bdi_cmd_pwd(int argc, char **argv);
int bdi_cmd_cd(int argc, char **argv);
int bdi_cmd_ls(int argc, char **argv);
int bdi_cmd_cat(int argc, char **argv);
int bdi_cmd_mkdir(int argc, char **argv);
int bdi_cmd_rmdir(int argc, char **argv);
int bdi_cmd_rm(int argc, char **argv);
int bdi_cmd_cp(int argc, char **argv);
int bdi_cmd_mv(int argc, char **argv);
int bdi_cmd_ps(int argc, char **argv);
int bdi_cmd_kill(int argc, char **argv);
int bdi_cmd_mount(int argc, char **argv);
int bdi_cmd_umount(int argc, char **argv);
int bdi_cmd_df(int argc, char **argv);
int bdi_cmd_free(int argc, char **argv);
int bdi_cmd_uname(int argc, char **argv);
int bdi_cmd_date(int argc, char **argv);
int bdi_cmd_uptime(int argc, char **argv);
int bdi_cmd_history(int argc, char **argv);

// Built-in commands table
static bdi_shell_command_t g_builtin_commands[] = {
    {"help", "Display help information", bdi_cmd_help},
    {"version", "Display BDI version information", bdi_cmd_version},
    {"exit", "Exit the shell", bdi_cmd_exit},
    {"quit", "Exit the shell", bdi_cmd_exit},
    {"clear", "Clear the screen", bdi_cmd_clear},
    {"echo", "Display text", bdi_cmd_echo},
    {"pwd", "Print working directory", bdi_cmd_pwd},
    {"cd", "Change directory", bdi_cmd_cd},
    {"ls", "List directory contents", bdi_cmd_ls},
    {"dir", "List directory contents", bdi_cmd_ls},
    {"cat", "Display file contents", bdi_cmd_cat},
    {"mkdir", "Create directory", bdi_cmd_mkdir},
    {"rmdir", "Remove directory", bdi_cmd_rmdir},
    {"rm", "Remove file", bdi_cmd_rm},
    {"del", "Remove file", bdi_cmd_rm},
    {"cp", "Copy file", bdi_cmd_cp},
    {"copy", "Copy file", bdi_cmd_cp},
    {"mv", "Move/rename file", bdi_cmd_mv},
    {"move", "Move/rename file", bdi_cmd_mv},
    {"ps", "List running processes", bdi_cmd_ps},
    {"kill", "Terminate process", bdi_cmd_kill},
    {"mount", "Mount filesystem", bdi_cmd_mount},
    {"umount", "Unmount filesystem", bdi_cmd_umount},
    {"df", "Display filesystem usage", bdi_cmd_df},
    {"free", "Display memory usage", bdi_cmd_free},
    {"uname", "Display system information", bdi_cmd_uname},
    {"date", "Display current date and time", bdi_cmd_date},
    {"uptime", "Display system uptime", bdi_cmd_uptime},
    {"history", "Display command history", bdi_cmd_history},
    {NULL, NULL, NULL} // Terminator
};

// Function prototypes
int bdi_shell_init(void);
int bdi_shell_run(void);
void bdi_shell_cleanup(void);
int bdi_shell_parse_input(const char *input);
int bdi_shell_execute_command(int argc, char **argv);
bdi_shell_command_t *bdi_shell_find_command(const char *name);
void bdi_shell_add_to_history(const char *command);
void bdi_shell_print_prompt(void);
char *bdi_shell_read_line(void);
void bdi_shell_print_banner(void);

/**
 * Initialize BDI shell
 */
int bdi_shell_init(void) {
    if (g_shell_initialized) {
        return BDI_SHELL_SUCCESS;
    }
    
    // Initialize shell state
    memset(&g_shell_state, 0, sizeof(bdi_shell_state_t));
    g_shell_state.running = 1;
    g_shell_state.exit_code = 0;
    strcpy(g_shell_state.current_directory, "/");
    g_shell_state.history_count = 0;
    g_shell_state.history_index = 0;
    
    g_shell_initialized = 1;
    return BDI_SHELL_SUCCESS;
}

/**
 * Run BDI shell main loop
 */
int bdi_shell_run(void) {
    if (!g_shell_initialized) {
        if (bdi_shell_init() != BDI_SHELL_SUCCESS) {
            return BDI_SHELL_ERROR;
        }
    }
    
    // Print banner
    bdi_shell_print_banner();
    
    // Main shell loop
    while (g_shell_state.running) {
        // Print prompt
        bdi_shell_print_prompt();
        
        // Read input
        char *input = bdi_shell_read_line();
        if (!input) {
            break;
        }
        
        // Skip empty lines
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        // Add to history
        bdi_shell_add_to_history(input);
        
        // Parse and execute command
        int result = bdi_shell_parse_input(input);
        if (result == BDI_SHELL_EXIT) {
            g_shell_state.running = 0;
        }
        
        free(input);
    }
    
    return g_shell_state.exit_code;
}

/**
 * Parse shell input
 */
int bdi_shell_parse_input(const char *input) {
    if (!input) {
        return BDI_SHELL_ERROR;
    }
    
    // Copy input to buffer
    strncpy(g_shell_state.input_buffer, input, BDI_SHELL_MAX_INPUT - 1);
    g_shell_state.input_buffer[BDI_SHELL_MAX_INPUT - 1] = '\0';
    
    // Tokenize input
    g_shell_state.argc = 0;
    char *token = strtok(g_shell_state.input_buffer, " \t\n");
    
    while (token && g_shell_state.argc < BDI_SHELL_MAX_ARGS - 1) {
        g_shell_state.args[g_shell_state.argc] = token;
        g_shell_state.argc++;
        token = strtok(NULL, " \t\n");
    }
    
    g_shell_state.args[g_shell_state.argc] = NULL;
    
    if (g_shell_state.argc == 0) {
        return BDI_SHELL_SUCCESS;
    }
    
    // Execute command
    return bdi_shell_execute_command(g_shell_state.argc, g_shell_state.args);
}

/**
 * Execute shell command
 */
int bdi_shell_execute_command(int argc, char **argv) {
    if (argc == 0 || !argv[0]) {
        return BDI_SHELL_SUCCESS;
    }
    
    // Find built-in command
    bdi_shell_command_t *cmd = bdi_shell_find_command(argv[0]);
    if (cmd) {
        return cmd->handler(argc, argv);
    }
    
    // Command not found
    printf("bdi: command not found: %s\n", argv[0]);
    printf("Type 'help' for a list of available commands.\n");
    
    return BDI_SHELL_ERROR;
}

/**
 * Find built-in command
 */
bdi_shell_command_t *bdi_shell_find_command(const char *name) {
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; g_builtin_commands[i].name; i++) {
        if (strcmp(g_builtin_commands[i].name, name) == 0) {
            return &g_builtin_commands[i];
        }
    }
    
    return NULL;
}

/**
 * Add command to history
 */
void bdi_shell_add_to_history(const char *command) {
    if (!command || strlen(command) == 0) {
        return;
    }
    
    int index = g_shell_state.history_count % 10;
    strncpy(g_shell_state.history[index], command, BDI_SHELL_MAX_INPUT - 1);
    g_shell_state.history[index][BDI_SHELL_MAX_INPUT - 1] = '\0';
    
    g_shell_state.history_count++;
}

/**
 * Print shell prompt
 */
void bdi_shell_print_prompt(void) {
    printf("%s", BDI_SHELL_PROMPT);
    fflush(stdout);
}

/**
 * Read line from input
 */
char *bdi_shell_read_line(void) {
    char *line = malloc(BDI_SHELL_MAX_INPUT);
    if (!line) {
        return NULL;
    }
    
    if (fgets(line, BDI_SHELL_MAX_INPUT, stdin) == NULL) {
        free(line);
        return NULL;
    }
    
    // Remove newline
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    
    return line;
}

/**
 * Print shell banner
 */
void bdi_shell_print_banner(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    BDI Shell v%s                         ║\n", BDI_SHELL_VERSION);
    printf("║          Binary Decomposition Interface Kernel              ║\n");
    printf("║                                                              ║\n");
    printf("║  Type 'help' for available commands                         ║\n");
    printf("║  Type 'exit' or 'quit' to exit the shell                    ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * Built-in command implementations
 */

int bdi_cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("BDI Shell - Available Commands:\n\n");
    
    for (int i = 0; g_builtin_commands[i].name; i++) {
        printf("  %-12s - %s\n", g_builtin_commands[i].name, g_builtin_commands[i].description);
    }
    
    printf("\nFor more information about BDI, visit the documentation.\n");
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_version(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("BDI Shell version %s\n", BDI_SHELL_VERSION);
    printf("Binary Decomposition Interface Kernel\n");
    printf("Built on %s %s\n", __DATE__, __TIME__);
    
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_exit(int argc, char **argv) {
    int exit_code = 0;
    
    if (argc > 1) {
        exit_code = atoi(argv[1]);
    }
    
    printf("Goodbye!\n");
    g_shell_state.exit_code = exit_code;
    
    return BDI_SHELL_EXIT;
}

int bdi_cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    
    // ANSI escape sequence to clear screen
    printf("\033[2J\033[H");
    fflush(stdout);
    
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("%s\n", g_shell_state.current_directory);
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_cd(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "/";
    
    // Simple path handling (in a real implementation, this would
    // interact with the VFS layer)
    if (strcmp(path, "..") == 0) {
        // Go to parent directory
        char *last_slash = strrchr(g_shell_state.current_directory, '/');
        if (last_slash && last_slash != g_shell_state.current_directory) {
            *last_slash = '\0';
        } else {
            strcpy(g_shell_state.current_directory, "/");
        }
    } else if (path[0] == '/') {
        // Absolute path
        strncpy(g_shell_state.current_directory, path, sizeof(g_shell_state.current_directory) - 1);
    } else {
        // Relative path
        if (strcmp(g_shell_state.current_directory, "/") != 0) {
            strncat(g_shell_state.current_directory, "/", sizeof(g_shell_state.current_directory) - strlen(g_shell_state.current_directory) - 1);
        }
        strncat(g_shell_state.current_directory, path, sizeof(g_shell_state.current_directory) - strlen(g_shell_state.current_directory) - 1);
    }
    
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_ls(int argc, char **argv) {
    (void)argc; (void)argv;
    
    // Simulate directory listing
    printf("total 8\n");
    printf("drwxr-xr-x  2 root root 4096 Jan  1 12:00 .\n");
    printf("drwxr-xr-x  3 root root 4096 Jan  1 12:00 ..\n");
    printf("-rw-r--r--  1 root root  100 Jan  1 12:00 example.txt\n");
    printf("drwxr-xr-x  2 root root 4096 Jan  1 12:00 subdir\n");
    
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_history(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("Command History:\n");
    
    int start = (g_shell_state.history_count > 10) ? g_shell_state.history_count - 10 : 0;
    int count = (g_shell_state.history_count > 10) ? 10 : g_shell_state.history_count;
    
    for (int i = 0; i < count; i++) {
        int index = (start + i) % 10;
        printf("%3d  %s\n", start + i + 1, g_shell_state.history[index]);
    }
    
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_uname(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("BDI %s x86_64 BDI-Kernel\n", BDI_SHELL_VERSION);
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_date(int argc, char **argv) {
    (void)argc; (void)argv;
    
    // Simulate current date (in a real implementation, this would
    // get the actual system time)
    printf("Mon Jan  1 12:00:00 UTC 2024\n");
    return BDI_SHELL_SUCCESS;
}

int bdi_cmd_uptime(int argc, char **argv) {
    (void)argc; (void)argv;
    
    // Simulate uptime
    printf(" 12:00:00 up 1 day,  2:30,  1 user,  load average: 0.15, 0.10, 0.05\n");
    return BDI_SHELL_SUCCESS;
}

// Placeholder implementations for other commands
int bdi_cmd_cat(int argc, char **argv) { (void)argc; (void)argv; printf("cat: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_mkdir(int argc, char **argv) { (void)argc; (void)argv; printf("mkdir: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_rmdir(int argc, char **argv) { (void)argc; (void)argv; printf("rmdir: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_rm(int argc, char **argv) { (void)argc; (void)argv; printf("rm: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_cp(int argc, char **argv) { (void)argc; (void)argv; printf("cp: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_mv(int argc, char **argv) { (void)argc; (void)argv; printf("mv: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_ps(int argc, char **argv) { (void)argc; (void)argv; printf("ps: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_kill(int argc, char **argv) { (void)argc; (void)argv; printf("kill: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_mount(int argc, char **argv) { (void)argc; (void)argv; printf("mount: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_umount(int argc, char **argv) { (void)argc; (void)argv; printf("umount: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_df(int argc, char **argv) { (void)argc; (void)argv; printf("df: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }
int bdi_cmd_free(int argc, char **argv) { (void)argc; (void)argv; printf("free: command not fully implemented\n"); return BDI_SHELL_SUCCESS; }

/**
 * Cleanup shell resources
 */
void bdi_shell_cleanup(void) {
    memset(&g_shell_state, 0, sizeof(bdi_shell_state_t));
    g_shell_initialized = 0;
}

/**
 * Main shell entry point (for testing)
 */
int main(void) {
    return bdi_shell_run();
}
