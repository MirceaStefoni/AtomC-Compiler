#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    char *source = loadFile(argv[1]);
    if (!source) {
        fprintf(stderr, "Failed to load source file: %s\n", argv[1]);
        return 1;
    }

    Token *tokens = tokenize(source);
    if (!tokens) {
        fprintf(stderr, "Failed to tokenize source file: %s\n", argv[1]);
        free(source);
        return 1;
    }

    showTokens(tokens); 

    parse(tokens); 

    printf("Syntax analysis completed successfully.\n");

    // Cleanup
    free(source);
    
    Token *tk = tokens;
    while (tk) {
        Token *next = tk->next;
        if (tk->code == ID || tk->code == STRING) {
            free(tk->text);
        }
        free(tk);
        tk = next;
    }

    return 0;
}