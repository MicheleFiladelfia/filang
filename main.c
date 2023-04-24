#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "scanner.h"
#include "vm.h"

static char *readFromFile(char *fileName) {
    FILE *file = fopen(fileName, "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open file %s", fileName);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *) malloc(fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[fileSize] = '\0';

    if ((long)bytesRead < fileSize) {
        fprintf(stderr, "Could not read file %s", fileName);
        exit(1);
    }

    fclose(file);

    return buffer;
}

static void signalHandler(int) {
    printf("\nCaught Ctrl+C, terminated.");
    exit(0);
}


static void initializeReadline() {
    rl_bind_key('\t', rl_insert);
    using_history();
}

static void repl() {
    initializeReadline();

    vm.repl = true;
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        fprintf(stderr, "Could not register signal handler");
        exit(1);
    }

    while (true) {
        char *line = readline("fi>> ");

        if (line == NULL) break;

        add_history(line);

        interpret(line);

        free(line);
    }
}

static void runFile(char *fileName) {
    vm.repl = false;
    char *sourceCode = readFromFile(fileName);
    interpret(sourceCode);
}


int main(int argc, char *argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: filang <filepath>.fi\n");
        return 1;
    }

    freeVM();
    return 0;
}
