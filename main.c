#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define DIR_MODE 0755


int folder_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}


int create_folder(const char *folder_name, mode_t mode) {
    if (mkdir(folder_name, mode) == -1) {
        fprintf(stderr, "error: failed to create folder '%s': %s\n", folder_name, strerror(errno));
        return 1;
    }
    return 0;
}


int handle_init_command() {
    if (folder_exists(".jit")) {
        fprintf(stderr, "error: .jit repository already exists\n");
        return 1;
    }
    if (create_folder(".jit", DIR_MODE) != 0) return 1;
    if (create_folder(".jit/objects", DIR_MODE) != 0) return 1;
    if (create_folder(".jit/refs", DIR_MODE) != 0) return 1;
    if (create_folder(".jit/refs/heads", DIR_MODE) != 0) return 1;
    FILE *fh_output = fopen(".jit/HEAD", "w");
    if (fh_output == NULL) {
        fprintf(stderr, "error: failed creating .jit/HEAD: %s\n", strerror(errno));
        return 1;
    }
    fputs("ref: refs/heads/main\n", fh_output);
    fclose(fh_output);
    return 0;
}


int main(int argc, char *argv[]) {
    printf("Welcome to my version control program\n");
    if (argc > 1 && strcmp(argv[1], "init") == 0) {
        printf("you typed %s\n", argv[1]);
        if (handle_init_command() == 0) {
            printf("initialized empty Jit repository in .jit\n");
        } else {
            fprintf(stderr, "error: failed to initialize empty Jit repository\n");
        }
    }
    return 0;
}
