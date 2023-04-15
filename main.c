#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
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

    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file %s", fileName);
        exit(1);
    }

    fclose(file);

    return buffer;
}

volatile bool shouldRun = true;

void signalHandler(int) {
    shouldRun = false;
    printf("\nCaught Ctrl+C, terminated.");
}

struct sigaction sa = { .sa_handler = signalHandler };

static void repl() {
    char line[1024];

    if (sigaction(SIGINT, &sa, 0) != 0) {
        fprintf(stderr, "Could not set signal handler.");
        exit(1);
    }

    while (shouldRun) {
        printf("fi>> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        if (strcmp(line, "\n") == 0) continue;


        interpret(line);
    }
}

static void runFile(char *fileName) {
    char *sourceCode = readFromFile(fileName);


    initScanner(sourceCode);

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
