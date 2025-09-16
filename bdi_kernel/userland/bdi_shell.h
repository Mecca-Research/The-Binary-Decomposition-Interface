
// ===================================================================
// DESC: BDI Shell - Interactive graph construction and execution
// ===================================================================
#ifndef AEON_BDI_SHELL_H
#define AEON_BDI_SHELL_H

#include "../../kernel/graph/graph.h"
#include "../../kernel/scheduler/scheduler.h"
#include "../../kernel/math/smart_number.h"
#include <stdint.h>
#include <stdbool.h>

// --- Shell Constants ---
#define BDI_SHELL_MAX_INPUT         1024
#define BDI_SHELL_MAX_ARGS          64
#define BDI_SHELL_MAX_HISTORY       100
#define BDI_SHELL_MAX_VARIABLES     256
#define BDI_SHELL_MAX_GRAPHS        16

// --- Command Types ---
typedef enum {
    CMD_TYPE_BUILTIN = 0,       // Built-in shell command
    CMD_TYPE_GRAPH = 1,         // Graph construction command
    CMD_TYPE_SYSTEM = 2,        // System command
    CMD_TYPE_MATH = 3           // Mathematical operation
} command_type_t;

// --- Variable Types ---
typedef enum {
    VAR_TYPE_INTEGER = 0,
    VAR_TYPE_FLOAT = 1,
    VAR_TYPE_STRING = 2,
    VAR_TYPE_GRAPH = 3,
    VAR_TYPE_NODE = 4,
    VAR_TYPE_SMART_NUMBER = 5
} variable_type_t;

// --- Shell Variable ---
typedef struct {
    char name[64];
    variable_type_t type;
    union {
        int64_t int_value;
        double float_value;
        char* string_value;
        BdiGraph* graph_value;
        NodeId node_value;
        smart_number_t* smart_num_value;
    } value;
    bool is_const;
    bool in_use;
} shell_variable_t;

// --- Graph Context ---
typedef struct {
    char name[64];
    BdiGraph* graph;
    Scheduler* scheduler;
    bool is_active;
    uint32_t execution_count;
    uint64_t total_runtime;
} graph_context_t;

// --- Command History Entry ---
typedef struct {
    char command[BDI_SHELL_MAX_INPUT];
    uint64_t timestamp;
    int exit_code;
} history_entry_t;

// --- Built-in Command ---
typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char* argv[]);
    const char* usage;
} builtin_command_t;

// --- BDI Shell State ---
typedef struct {
    // Input handling
    char input_buffer[BDI_SHELL_MAX_INPUT];
    char* args[BDI_SHELL_MAX_ARGS];
    int argc;
    
    // Command history
    history_entry_t history[BDI_SHELL_MAX_HISTORY];
    uint32_t history_count;
    uint32_t history_index;
    
    // Variables
    shell_variable_t variables[BDI_SHELL_MAX_VARIABLES];
    uint32_t variable_count;
    
    // Graph contexts
    graph_context_t graphs[BDI_SHELL_MAX_GRAPHS];
    uint32_t graph_count;
    graph_context_t* current_graph;
    
    // Shell state
    bool running;
    bool interactive;
    int last_exit_code;
    char current_directory[256];
    char prompt[128];
    
    // System integration
    DeviceVTable** devices;
    size_t device_count;
    
    // Statistics
    uint64_t commands_executed;
    uint64_t graphs_created;
    uint64_t nodes_created;
    uint64_t total_execution_time;
    
    bool initialized;
} bdi_shell_t;

// --- Function Declarations ---

// Shell management
int bdi_shell_init(bdi_shell_t* shell, DeviceVTable** devices, size_t device_count);
int bdi_shell_shutdown(bdi_shell_t* shell);
int bdi_shell_run(bdi_shell_t* shell);
int bdi_shell_run_command(bdi_shell_t* shell, const char* command);

// Input handling
int bdi_shell_read_input(bdi_shell_t* shell);
int bdi_shell_parse_command(bdi_shell_t* shell, const char* input);
int bdi_shell_execute_command(bdi_shell_t* shell);

// Built-in commands
int cmd_help(int argc, char* argv[]);
int cmd_exit(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_history(int argc, char* argv[]);
int cmd_set(int argc, char* argv[]);
int cmd_unset(int argc, char* argv[]);
int cmd_vars(int argc, char* argv[]);
int cmd_pwd(int argc, char* argv[]);
int cmd_cd(int argc, char* argv[]);

// Graph commands
int cmd_graph_create(int argc, char* argv[]);
int cmd_graph_list(int argc, char* argv[]);
int cmd_graph_select(int argc, char* argv[]);
int cmd_graph_delete(int argc, char* argv[]);
int cmd_graph_info(int argc, char* argv[]);
int cmd_graph_export(int argc, char* argv[]);
int cmd_graph_import(int argc, char* argv[]);

// Node commands
int cmd_node_add(int argc, char* argv[]);
int cmd_node_remove(int argc, char* argv[]);
int cmd_node_connect(int argc, char* argv[]);
int cmd_node_disconnect(int argc, char* argv[]);
int cmd_node_list(int argc, char* argv[]);
int cmd_node_info(int argc, char* argv[]);

// Execution commands
int cmd_run(int argc, char* argv[]);
int cmd_step(int argc, char* argv[]);
int cmd_stop(int argc, char* argv[]);
int cmd_reset(int argc, char* argv[]);
int cmd_schedule(int argc, char* argv[]);

// System commands
int cmd_devices(int argc, char* argv[]);
int cmd_memory(int argc, char* argv[]);
int cmd_stats(int argc, char* argv[]);
int cmd_debug(int argc, char* argv[]);

// Math commands
int cmd_calc(int argc, char* argv[]);
int cmd_smart_add(int argc, char* argv[]);
int cmd_smart_mul(int argc, char* argv[]);
int cmd_precision(int argc, char* argv[]);

// Variable management
int bdi_shell_set_variable(bdi_shell_t* shell, const char* name, variable_type_t type, const void* value);
shell_variable_t* bdi_shell_get_variable(bdi_shell_t* shell, const char* name);
int bdi_shell_unset_variable(bdi_shell_t* shell, const char* name);
int bdi_shell_list_variables(bdi_shell_t* shell);

// Graph management
graph_context_t* bdi_shell_create_graph(bdi_shell_t* shell, const char* name);
graph_context_t* bdi_shell_find_graph(bdi_shell_t* shell, const char* name);
int bdi_shell_delete_graph(bdi_shell_t* shell, const char* name);
int bdi_shell_select_graph(bdi_shell_t* shell, const char* name);

// History management
int bdi_shell_add_history(bdi_shell_t* shell, const char* command, int exit_code);
int bdi_shell_print_history(bdi_shell_t* shell);
const char* bdi_shell_get_history(bdi_shell_t* shell, uint32_t index);

// Utility functions
void bdi_shell_print_prompt(bdi_shell_t* shell);
void bdi_shell_print_banner(void);
void bdi_shell_print_help(void);
char* bdi_shell_expand_variables(bdi_shell_t* shell, const char* input);
bool bdi_shell_is_builtin(const char* command);

// Graph parsing and construction
int bdi_shell_parse_graph_description(bdi_shell_t* shell, const char* description);
int bdi_shell_build_graph_from_text(bdi_shell_t* shell, const char* text, BdiGraph** graph);

// Error handling
void bdi_shell_print_error(const char* message);
void bdi_shell_print_warning(const char* message);
void bdi_shell_print_info(const char* message);

// Built-in command table
extern const builtin_command_t builtin_commands[];
extern const size_t builtin_command_count;

// Global shell instance (for signal handlers)
extern bdi_shell_t* global_shell;

#endif // AEON_BDI_SHELL_H
