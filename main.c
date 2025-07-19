#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <openssl/sha.h>

#define DIR_MODE 0755

int folder_exists(const char *path);
int create_folder(const char *folder_name, mode_t mode);
int handle_init_command();
int handle_hash_object_command(const char *file_name);
int handle_cat_file_command(const char *hash, char mode);


int main(int argc, char *argv[]) {
    printf("Welcome to my version control program\n");
    if (argc > 1 && strcmp(argv[1], "init") == 0) {
        printf("you typed %s\n", argv[1]);
        if (handle_init_command() == 0) {
            printf("initialized empty Jit repository in .jit\n");
        } else {
            fprintf(stderr, "error: failed to initialize empty Jit repository\n");
        }
    } else if (argc > 2 && strcmp(argv[1], "hash-object") == 0) {
        printf("you typed %s\n", argv[2]);
        if (handle_hash_object_command(argv[2]) == 0) {
            printf("Success hash-object\n");
        } else {
            fprintf(stderr, "Failed hash-object\n");
        }
    } else if (argc > 3 && strcmp(argv[1], "cat-file") == 0) {
        if (strcmp(argv[2], "-p") == 0) {
            handle_cat_file_command(argv[3], 'p');
        }
    }
    return 0;
}

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


int handle_hash_object_command(const char *file_name) {
    FILE *pF = fopen(file_name, "r");
    if (pF == NULL) {
        fprintf(stderr, "error: failed opening %s %s\n", file_name, strerror(errno));
        return 1;
    }

    // Determine file size
    if (fseek(pF, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fclose(pF);
        return 1;
    }
    long file_size = ftell(pF);
    if (file_size == -1L) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fclose(pF);
        return 1;
    }

    // Read and then close file
    rewind(pF);
    char *content_buffer = malloc(file_size);
    if (content_buffer == NULL) {
        fprintf(stderr, "error: memory allocation failed");
        fclose(pF);
        return 1;
    }
    size_t bytes_read = fread(content_buffer, 1, file_size, pF);
    if (bytes_read != file_size) {
        fprintf(stderr, "error: failed reading file");
        fclose(pF);
        return 1;
    }
    fclose(pF);

    // Create header
    char header_buffer[64];
    int header_len = snprintf(header_buffer, sizeof(header_buffer), "blob %ld", file_size);
    header_buffer[header_len++] = '\0';

    size_t total_size = header_len + file_size;
    char *data_buffer = malloc(total_size);
    if(data_buffer == NULL) {
        fprintf(stderr, "error: memory allocation failed\n");
        free(content_buffer);
        return 1;
    }

    memcpy(data_buffer, header_buffer, header_len);
    memcpy(data_buffer + header_len, content_buffer, file_size);
    free(content_buffer);

    // Hash using SHA-1
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)data_buffer, total_size, hash);
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    // Create folder and file
    char dir_path[64];
    snprintf(dir_path, sizeof(dir_path), ".jit/objects/%02x", hash[0]);
    if (create_folder(dir_path, DIR_MODE) != 0) return 1;
    char hash_hex[41];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(hash_hex + (i * 2), "%02x", hash[i]);
    }
    char file_path[128];
    snprintf(file_path, sizeof(file_path), ".jit/objects/%.2s/%.38s", hash_hex, hash_hex + 2);

    // Write to file
    FILE *output_file = fopen(file_path, "wb");
    if (output_file == NULL) {
        fprintf(stderr, "error: failed creating new file %s\n", strerror(errno));
        return 1;
    }
    fwrite(data_buffer, 1, total_size, output_file);
    fclose(output_file);
    free(data_buffer);
    return 0;
}


int handle_cat_file_command(const char *hash, char mode) {
    // Open file
    char file_path[128];
    snprintf(file_path, sizeof(file_path), ".jit/objects/%.2s/%.38s", hash, hash + 2);
    FILE *input_file = fopen(file_path, "r");
    if (input_file == NULL) {
        fprintf(stderr, "error: failed opening %s %s\n", file_path, strerror(errno));
        return 1;
    }

    // Determine file size
    if (fseek(input_file, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fclose(input_file);
        return 1;
    }
    long file_size = ftell(input_file);
    if (file_size == -1L) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fclose(input_file);
        return 1;
    }

    // Read and close file
    rewind(input_file);
    char *content_buffer = malloc(file_size);
    fread(content_buffer, 1, file_size, input_file);
    fclose(input_file);

    // Get content only (no header)
    size_t i = 0;
    while (i < file_size && content_buffer[i] != '\0') {
        i++;
    }
    i++;
    size_t content_length = file_size - i;
    fwrite(content_buffer + i, 1, content_length, stdout);

    free(content_buffer);
    return 0;
}
