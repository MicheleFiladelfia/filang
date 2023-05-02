#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "vm.h"

static char *read_from_file(char *file_path) {
    FILE *file = fopen(file_path, "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open file %s", file_path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *) malloc(fileSize + 1);
    size_t bytes_read = fread(buffer, sizeof(char), fileSize, file);
    buffer[fileSize] = '\0';

    if ((long) bytes_read < fileSize) {
        fprintf(stderr, "Could not read file %s", file_path);
        exit(1);
    }

    fclose(file);

    return buffer;
}

static void signal_handler(int) {
    printf("\nCaught Ctrl+C, terminated.");
    exit(0);
}


static void initialize_readline() {
    rl_bind_key('\t', rl_insert);
    using_history();
}

static void repl() {
    initialize_readline();

    vm.repl = true;
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
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

static void run_file(char *fileName) {
    vm.repl = false;
    char *sourceCode = read_from_file(fileName);
    interpret(sourceCode);
}


int main(int argc, char *argv[]) {
    init_vm();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: filang <filepath>.fi\n");
        return 1;
    }

    free_vm();
    return 0;
}
