#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


int handle_init_command() {
    if (mkdir(".jit", 0777) == -1) {
        printf("Error creating repo!\n");
        return 1;
    } else {
        printf("jit repo created successfully!\n");
        return 0;
    }
}


int main(int argc, char *argv[]) {
    printf("Welcome to my version control program\n");
    if (argc > 1 && strcmp(argv[1], "init") == 0) {
        printf("you typed %s\n", argv[1]);
        return handle_init_command();
    }
    return 0;
}
