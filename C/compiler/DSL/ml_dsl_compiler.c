
/**
 * @file ml_dsl_compiler.c
 * @brief ML DSL Compiler Implementation
 */

#include "ml_dsl_compiler.h"
#include "ml_dsl_lexer.h"
#include "ml_dsl_parser.h"
#include "../AIBase/linear/linear_regression.h"
#include "../AIBase/tree/decision_tree.h"
#include "../AIBase/kernel/svm.h"
#include "../AIBase/clustering/kmeans.h"
#include "../AIBase/reinforcement/q_learning.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Compiler lifecycle

DSLCompiler* dsl_compiler_create(void) {
    DSLCompiler* compiler = malloc(sizeof(DSLCompiler));
    if (!compiler) return nullptr;
    
    compiler->symbol_table.capacity = 16;
    compiler->symbol_table.count = 0;
    compiler->symbol_table.names = malloc(compiler->symbol_table.capacity * sizeof(char*));
    compiler->symbol_table.models = malloc(compiler->symbol_table.capacity * sizeof(void*));
    
    if (!compiler->symbol_table.names || !compiler->symbol_table.models) {
        free(compiler->symbol_table.names);
        free(compiler->symbol_table.models);
        free(compiler);
        return nullptr;
    }
    
    compiler->has_error = false;
    compiler->error_message = nullptr;
    compiler->vm_context = ml_vm_context_create();
    
    if (!compiler->vm_context) {
        free(compiler->symbol_table.names);
        free(compiler->symbol_table.models);
        free(compiler);
        return nullptr;
    }
    
    return compiler;
}

void dsl_compiler_destroy(DSLCompiler* compiler) {
    if (!compiler) return;
    
    for (size_t i = 0; i < compiler->symbol_table.count; i++) {
        free(compiler->symbol_table.names[i]);
    }
    
    free(compiler->symbol_table.names);
    free(compiler->symbol_table.models);
    free(compiler->error_message);
    
    if (compiler->vm_context) {
        ml_vm_context_destroy(compiler->vm_context);
    }
    
    free(compiler);
}

// Symbol table

void dsl_compiler_add_model(DSLCompiler* compiler, const char* name, void* model) {
    if (!compiler || !name || !model) return;
    
    if (compiler->symbol_table.count >= compiler->symbol_table.capacity) {
        compiler->symbol_table.capacity *= 2;
        
        char** new_names = realloc(compiler->symbol_table.names,
                                  compiler->symbol_table.capacity * sizeof(char*));
        void** new_models = realloc(compiler->symbol_table.models,
                                   compiler->symbol_table.capacity * sizeof(void*));
        
        if (!new_names || !new_models) return;
        
        compiler->symbol_table.names = new_names;
        compiler->symbol_table.models = new_models;
    }
    
    compiler->symbol_table.names[compiler->symbol_table.count] = strdup(name);
    compiler->symbol_table.models[compiler->symbol_table.count] = model;
    compiler->symbol_table.count++;
}

void* dsl_compiler_get_model(DSLCompiler* compiler, const char* name) {
    if (!compiler || !name) return nullptr;
    
    for (size_t i = 0; i < compiler->symbol_table.count; i++) {
        if (strcmp(compiler->symbol_table.names[i], name) == 0) {
            return compiler->symbol_table.models[i];
        }
    }
    
    return nullptr;
}

bool dsl_compiler_has_model(DSLCompiler* compiler, const char* name) {
    return dsl_compiler_get_model(compiler, name) != nullptr;
}

// Error handling

bool dsl_compiler_has_error(const DSLCompiler* compiler) {
    return compiler ? compiler->has_error : false;
}

const char* dsl_compiler_get_error(const DSLCompiler* compiler) {
    return compiler ? compiler->error_message : nullptr;
}

void dsl_compiler_report_error(DSLCompiler* compiler, const char* message) {
    if (!compiler) return;
    
    compiler->has_error = true;
    free(compiler->error_message);
    compiler->error_message = message ? strdup(message) : nullptr;
    
    fprintf(stderr, "Compilation error: %s\n", message);
}

// Helper function to parse discrete[n] format
static bool parse_discrete_space(const char* space_str, size_t* out_size) {
    if (!space_str || !out_size) return false;
    
    // Expected format: "discrete[n]" where n is a positive integer
    const char* prefix = "discrete[";
    size_t prefix_len = strlen(prefix);
    
    if (strncmp(space_str, prefix, prefix_len) != 0) {
        return false;
    }
    
    // Find the closing bracket
    const char* bracket_pos = strchr(space_str + prefix_len, ']');
    if (!bracket_pos) {
        return false;
    }
    
    // Verify nothing comes after the closing bracket
    if (*(bracket_pos + 1) != '\0') {
        return false;
    }
    
    // Extract the number between brackets
    char num_str[32];
    size_t num_len = bracket_pos - (space_str + prefix_len);
    
    if (num_len == 0 || num_len >= sizeof(num_str)) {
        return false;
    }
    
    strncpy(num_str, space_str + prefix_len, num_len);
    num_str[num_len] = '\0';
    
    // Parse the number
    char* endptr;
    long value = strtol(num_str, &endptr, 10);
    
    // Check if parsing was successful and value is valid
    if (*endptr != '\0' || value <= 0 || value > 1000000) {
        return false;
    }
    
    *out_size = (size_t)value;
    return true;
}

// Semantic analysis

bool dsl_compiler_validate_model_decl(DSLCompiler* compiler, ASTNode* node) {
    if (!compiler || !node || node->type != AST_MODEL_DECL) return false;
    
    // Check if model already exists
    if (dsl_compiler_has_model(compiler, node->data.model_decl.name)) {
        char error[256];
        snprintf(error, sizeof(error), "Model '%s' already declared",
                node->data.model_decl.name);
        dsl_compiler_report_error(compiler, error);
        return false;
    }
    
    // Validate required parameters based on model type
    ASTParameterList* params = node->data.model_decl.parameters;
    ASTParameter* type_param = ast_parameter_list_get(params, "type");
    
    if (!type_param) {
        dsl_compiler_report_error(compiler, "Model declaration missing 'type' parameter");
        return false;
    }
    
    return true;
}

bool dsl_compiler_validate_train_stmt(DSLCompiler* compiler, ASTNode* node) {
    if (!compiler || !node || node->type != AST_TRAIN_STMT) return false;
    
    // Check if model exists
    if (!dsl_compiler_has_model(compiler, node->data.train_stmt.model_name)) {
        char error[256];
        snprintf(error, sizeof(error), "Model '%s' not declared",
                node->data.train_stmt.model_name);
        dsl_compiler_report_error(compiler, error);
        return false;
    }
    
    return true;
}

bool dsl_compiler_validate_predict_stmt(DSLCompiler* compiler, ASTNode* node) {
    if (!compiler || !node || node->type != AST_PREDICT_STMT) return false;
    
    // Check if model exists
    if (!dsl_compiler_has_model(compiler, node->data.predict_stmt.model_name)) {
        char error[256];
        snprintf(error, sizeof(error), "Model '%s' not declared",
                node->data.predict_stmt.model_name);
        dsl_compiler_report_error(compiler, error);
        return false;
    }
    
    return true;
}

// Compilation

bool dsl_compiler_compile_model_decl(DSLCompiler* compiler, ASTNode* node) {
    if (!compiler || !node || node->type != AST_MODEL_DECL) return false;
    
    if (!dsl_compiler_validate_model_decl(compiler, node)) {
        return false;
    }
    
    ASTParameterList* params = node->data.model_decl.parameters;
    ASTParameter* type_param = ast_parameter_list_get(params, "type");
    
    if (!type_param || !type_param->value.string) {
        dsl_compiler_report_error(compiler, "Invalid model type");
        return false;
    }
    
    const char* model_type = type_param->value.string;
    void* model = nullptr;
    
    // Create model based on type
    if (strcmp(model_type, "linear_regression") == 0) {
        ASTParameter* input_param = ast_parameter_list_get(params, "input");
        if (!input_param) {
            dsl_compiler_report_error(compiler, "Linear regression requires 'input' parameter");
            return false;
        }
        
        // Parse input dimension (e.g., "vector[n]")
        size_t n_features = 10; // Default
        
        LinearRegressionConfig config = linear_regression_default_config();
        
        ASTParameter* lr_param = ast_parameter_list_get(params, "learning_rate");
        if (lr_param && lr_param->value_type == PARAM_NUMBER) {
            config.learning_rate = lr_param->value.number;
        }
        
        model = linear_regression_create(n_features, config);
        
    } else if (strcmp(model_type, "decision_tree") == 0) {
        DecisionTreeConfig config = decision_tree_default_config();
        
        ASTParameter* depth_param = ast_parameter_list_get(params, "max_depth");
        if (depth_param && depth_param->value_type == PARAM_NUMBER) {
            config.max_depth = (size_t)depth_param->value.number;
        }
        
        model = decision_tree_create(config);
        
    } else if (strcmp(model_type, "svm") == 0) {
        SVMConfig config = svm_default_config();
        
        ASTParameter* kernel_param = ast_parameter_list_get(params, "kernel");
        if (kernel_param && kernel_param->value.string) {
            if (strcmp(kernel_param->value.string, "linear") == 0) {
                config.kernel_type = SVM_KERNEL_LINEAR;
            } else if (strcmp(kernel_param->value.string, "rbf") == 0) {
                config.kernel_type = SVM_KERNEL_RBF;
            }
        }
        
        ASTParameter* c_param = ast_parameter_list_get(params, "C");
        if (c_param && c_param->value_type == PARAM_NUMBER) {
            config.C = c_param->value.number;
        }
        
        // Note: n_features should be provided as a parameter or inferred from data
        // For now, using a default value - should be updated when training data is available
        size_t n_features = 10; // Default, should be overridden
        ASTParameter* features_param = ast_parameter_list_get(params, "n_features");
        if (features_param && features_param->value_type == PARAM_NUMBER) {
            n_features = (size_t)features_param->value.number;
        }
        
        model = svm_create(n_features, config);
        
    } else if (strcmp(model_type, "kmeans") == 0) {
        ASTParameter* clusters_param = ast_parameter_list_get(params, "n_clusters");
        if (!clusters_param || clusters_param->value_type != PARAM_NUMBER) {
            dsl_compiler_report_error(compiler, "K-means requires 'n_clusters' parameter");
            return false;
        }
        
        size_t n_clusters = (size_t)clusters_param->value.number;
        KMeansConfig config = kmeans_default_config(n_clusters);
        
        // Note: n_features should be provided as a parameter or inferred from data
        size_t n_features = 10; // Default, should be overridden
        ASTParameter* features_param = ast_parameter_list_get(params, "n_features");
        if (features_param && features_param->value_type == PARAM_NUMBER) {
            n_features = (size_t)features_param->value.number;
        }
        
        model = kmeans_create(n_features, config);
        
    } else if (strcmp(model_type, "qlearning") == 0) {
        ASTParameter* state_param = ast_parameter_list_get(params, "state_space");
        ASTParameter* action_param = ast_parameter_list_get(params, "action_space");
        
        if (!state_param || !action_param) {
            dsl_compiler_report_error(compiler, 
                "Q-learning requires 'state_space' and 'action_space' parameters");
            return false;
        }
        
        // Parse discrete[n] format from parameter values
        size_t n_states = 100;  // Default fallback
        size_t n_actions = 4;   // Default fallback
        
        // Parse state_space parameter
        const char* state_space_str = NULL;
        if (state_param->value_type == PARAM_STRING && state_param->value.string) {
            state_space_str = state_param->value.string;
        } else if (state_param->value_type == PARAM_TYPE && state_param->value.string) {
            state_space_str = state_param->value.string;
        }
        
        if (state_space_str) {
            if (!parse_discrete_space(state_space_str, &n_states)) {
                char error[256];
                snprintf(error, sizeof(error), 
                    "Invalid state_space format: '%s'. Expected 'discrete[n]' where n > 0",
                    state_space_str);
                dsl_compiler_report_error(compiler, error);
                return false;
            }
        } else {
            dsl_compiler_report_error(compiler, 
                "state_space parameter must be in format 'discrete[n]'");
            return false;
        }
        
        // Parse action_space parameter
        const char* action_space_str = NULL;
        if (action_param->value_type == PARAM_STRING && action_param->value.string) {
            action_space_str = action_param->value.string;
        } else if (action_param->value_type == PARAM_TYPE && action_param->value.string) {
            action_space_str = action_param->value.string;
        }
        
        if (action_space_str) {
            if (!parse_discrete_space(action_space_str, &n_actions)) {
                char error[256];
                snprintf(error, sizeof(error), 
                    "Invalid action_space format: '%s'. Expected 'discrete[n]' where n > 0",
                    action_space_str);
                dsl_compiler_report_error(compiler, error);
                return false;
            }
        } else {
            dsl_compiler_report_error(compiler, 
                "action_space parameter must be in format 'discrete[n]'");
            return false;
        }
        
        // Validate parsed values are reasonable
        if (n_states == 0 || n_actions == 0) {
            dsl_compiler_report_error(compiler, 
                "State and action space sizes must be greater than 0");
            return false;
        }
        
        if (n_states > 100000 || n_actions > 10000) {
            char error[256];
            snprintf(error, sizeof(error), 
                "State/action space too large: states=%zu, actions=%zu. Maximum: 100000/10000",
                n_states, n_actions);
            dsl_compiler_report_error(compiler, error);
            return false;
        }
        
        QLearningConfig config = qlearning_default_config();
        
        ASTParameter* alpha_param = ast_parameter_list_get(params, "learning_rate");
        if (alpha_param && alpha_param->value_type == PARAM_NUMBER) {
            config.alpha = alpha_param->value.number;
        }
        
        ASTParameter* gamma_param = ast_parameter_list_get(params, "discount_factor");
        if (gamma_param && gamma_param->value_type == PARAM_NUMBER) {
            config.gamma = gamma_param->value.number;
        }
        
        model = qlearning_create(n_states, n_actions, config);
        
    } else {
        char error[256];
        snprintf(error, sizeof(error), "Unknown model type: %s", model_type);
        dsl_compiler_report_error(compiler, error);
        return false;
    }
    
    if (!model) {
        dsl_compiler_report_error(compiler, "Failed to create model");
        return false;
    }
    
    dsl_compiler_add_model(compiler, node->data.model_decl.name, model);
    
    return true;
}

bool dsl_compiler_compile_train_stmt(DSLCompiler* compiler, ASTNode* node) {
    if (!compiler || !node || node->type != AST_TRAIN_STMT) return false;
    
    if (!dsl_compiler_validate_train_stmt(compiler, node)) {
        return false;
    }
    
    // Training would require loading dataset and calling appropriate fit function
    // For now, we just validate the statement
    
    printf("Training model '%s' with dataset '%s'\n",
           node->data.train_stmt.model_name,
           node->data.train_stmt.dataset_path);
    
    return true;
}

bool dsl_compiler_compile_predict_stmt(DSLCompiler* compiler, ASTNode* node) {
    if (!compiler || !node || node->type != AST_PREDICT_STMT) return false;
    
    if (!dsl_compiler_validate_predict_stmt(compiler, node)) {
        return false;
    }
    
    // Prediction would call the appropriate predict function
    // For now, we just validate the statement
    
    printf("Predicting with model '%s'\n", node->data.predict_stmt.model_name);
    
    return true;
}

bool dsl_compiler_compile(DSLCompiler* compiler, ASTNode* ast) {
    if (!compiler || !ast) return false;
    
    if (ast->type != AST_PROGRAM) {
        dsl_compiler_report_error(compiler, "Expected program node");
        return false;
    }
    
    // Compile each statement
    for (size_t i = 0; i < ast->data.program.statement_count; i++) {
        ASTNode* stmt = ast->data.program.statements[i];
        bool success = false;
        
        switch (stmt->type) {
            case AST_MODEL_DECL:
                success = dsl_compiler_compile_model_decl(compiler, stmt);
                break;
            case AST_TRAIN_STMT:
                success = dsl_compiler_compile_train_stmt(compiler, stmt);
                break;
            case AST_PREDICT_STMT:
                success = dsl_compiler_compile_predict_stmt(compiler, stmt);
                break;
            default:
                dsl_compiler_report_error(compiler, "Unknown statement type");
                return false;
        }
        
        if (!success) {
            return false;
        }
    }
    
    return true;
}

// High-level API

bool dsl_compile_source(const char* source, MLVMContext** out_vm_context) {
    if (!source || !out_vm_context) return false;
    
    // Lexical analysis
    Lexer* lexer = lexer_create(source);
    if (!lexer) return false;
    
    size_t token_count;
    Token* tokens = lexer_tokenize_all(lexer, &token_count);
    
    if (lexer_has_error(lexer)) {
        fprintf(stderr, "Lexer error: %s\n", lexer_get_error(lexer));
        lexer_destroy(lexer);
        return false;
    }
    
    lexer_destroy(lexer);
    
    if (!tokens) return false;
    
    // Syntax analysis
    Parser* parser = parser_create(tokens, token_count);
    if (!parser) {
        free(tokens);
        return false;
    }
    
    ASTNode* ast = parser_parse(parser);
    
    if (parser_has_error(parser)) {
        fprintf(stderr, "Parser error: %s\n", parser_get_error(parser));
        parser_destroy(parser);
        free(tokens);
        return false;
    }
    
    parser_destroy(parser);
    
    if (!ast) {
        free(tokens);
        return false;
    }
    
    // Semantic analysis and code generation
    DSLCompiler* compiler = dsl_compiler_create();
    if (!compiler) {
        ast_node_destroy(ast);
        free(tokens);
        return false;
    }
    
    bool success = dsl_compiler_compile(compiler, ast);
    
    if (!success) {
        fprintf(stderr, "Compiler error: %s\n", dsl_compiler_get_error(compiler));
        dsl_compiler_destroy(compiler);
        ast_node_destroy(ast);
        free(tokens);
        return false;
    }
    
    *out_vm_context = compiler->vm_context;
    compiler->vm_context = nullptr; // Transfer ownership
    
    dsl_compiler_destroy(compiler);
    ast_node_destroy(ast);
    free(tokens);
    
    return true;
}

bool dsl_compile_file(const char* filename, MLVMContext** out_vm_context) {
    if (!filename || !out_vm_context) return false;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return false;
    }
    
    // Read file contents
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = malloc(file_size + 1);
    if (!source) {
        fclose(file);
        return false;
    }
    
    size_t read_size = fread(source, 1, file_size, file);
    source[read_size] = '\0';
    
    fclose(file);
    
    bool success = dsl_compile_source(source, out_vm_context);
    
    free(source);
    
    return success;
}
