// =======================
// Chimera Node Compiler Seed
// =======================
// Phase: Graph-Native Compiler Bootloader
// Goal: Build first semantic-token-aware node-compiler for Chimera-C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chimera_graph.h"
#include "chimera_codegen.h"
#include "chimera_parser.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <source_file.chi>\n", argv[0]);
        return 1;
    }

    // Step 1: Load Source
    const char *source_path = argv[1];
    char *source_code = load_source_file(source_path);
    if (!source_code) {
        fprintf(stderr, "Failed to load source code: %s\n", source_path);
        return 1;
    }

    // Step 2: Tokenize + Parse into Chimera Node Graph
    ChimeraGraph *graph = chimera_parse(source_code);
    if (!graph) {
        fprintf(stderr, "Parse error.\n");
        free(source_code);
        return 1;
    }

    // Step 3: Codegen from Node Graph
    if (!chimera_codegen(graph)) {
        fprintf(stderr, "Code generation failed.\n");
        free(source_code);
        destroy_chimera_graph(graph);
        return 1;
    }

    printf("\n✅ Chimera Node Compilation Complete.\n");

    // Cleanup
    free(source_code);
    destroy_chimera_graph(graph);
    return 0;
}
