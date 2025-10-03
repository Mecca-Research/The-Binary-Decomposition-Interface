
#ifndef BDI_SHELL_H
#define BDI_SHELL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

// C23: Use constexpr for shell limits
#define SHELL_MAX_COMMAND_LENGTH 1024
#define SHELL_MAX_ARGS 64
#define SHELL_MAX_HISTORY 100
#define SHELL_MAX_JOBS 32
#define SHELL_MAX_PIPES 8

// C23: Static assertions for buffer sizes
_Static_assert(SHELL_MAX_COMMAND_LENGTH >= 256, "Command buffer too small");
_Static_assert(SHELL_MAX_ARGS >= 16, "Argument array too small");
_Static_assert(SHELL_MAX_HISTORY >= 10, "History buffer too small");

// Command history entry
typedef struct {
    char command[SHELL_MAX_COMMAND_LENGTH];
    uint64_t timestamp;
} ShellHistoryEntry;

// Job control structure
typedef struct {
    uint32_t job_id;
    uint32_t pid;
    char command[SHELL_MAX_COMMAND_LENGTH];
    bool is_background;
    bool is_running;
    bool is_stopped;
} ShellJob;

// Shell state
typedef struct {
    // Command history
    ShellHistoryEntry history[SHELL_MAX_HISTORY];
    uint32_t history_count;
    uint32_t history_index;
    
    // Job control
    ShellJob jobs[SHELL_MAX_JOBS];
    _Atomic uint32_t job_count;
    
    // Shell state
    bool running;
    char current_dir[256];
    char prompt[64];
} ShellState;

// External shell state for access from other modules
extern ShellState shell_state;

// C23: [[nodiscard]] for shell functions
[[nodiscard]] int shell_init(void);
[[nodiscard]] int shell_run(void);
void shell_cleanup(void);

// Command execution
[[nodiscard]] int shell_execute_command(const char* command);
[[nodiscard]] int shell_parse_command(const char* input, char** args);
[[nodiscard]] int shell_execute_builtin(char** args);

// History functions
void shell_add_history(const char* command);
[[nodiscard]] const char* shell_get_history(int offset);
void shell_clear_history(void);

// Tab completion
[[nodiscard]] char* shell_tab_complete(const char* partial);

// Job control
[[nodiscard]] int shell_launch_job(char** args, bool background);
[[nodiscard]] int shell_fg(uint32_t job_id);
[[nodiscard]] int shell_bg(uint32_t job_id);
void shell_list_jobs(void);

// Pipes and redirection
[[nodiscard]] int shell_execute_pipeline(char* commands[], int num_commands);
[[nodiscard]] int shell_redirect_io(const char* input_file, const char* output_file, bool append);

#endif // BDI_SHELL_H
