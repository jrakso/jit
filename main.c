#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <openssl/sha.h>

#define DIR_MODE 0755

typedef struct {
    char mode[7];
    char *file_name;
    unsigned char hash[20];
} TreeEntry;

int folder_exists(const char *path);
int create_folder(const char *folder_name, mode_t mode);
int handle_init_command();
unsigned char *handle_hash_object_command(const char *file_name);
int handle_cat_file_command(const char *hash, char mode);
// int handle_write_tree_command();
int handle_add_command(const char *file_name);
int get_file_mode(const char *filename);


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


long get_file_size(FILE *pF) {
    if (fseek(pF, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        return -1;
    }
    long file_size = ftell(pF);
    if (file_size == -1L) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        return -1;
    }
    rewind(pF);
    return file_size;
}


FILE *open_file(const char *file_name, const char *mode) {
    FILE *pF = fopen(file_name, mode);
    if (pF == NULL) {
        fprintf(stderr, "error: failed opening %s %s\n", file_name, strerror(errno));
        return NULL;
    }
    return pF;
}


char *read_file(FILE *pF, long file_size) {
    char *content_buffer = malloc(file_size);
    if (content_buffer == NULL) {
        fprintf(stderr, "error: memory allocation failed\n");
        return NULL;
    }
    size_t bytes_read = fread(content_buffer, 1, file_size, pF);
    if (bytes_read != file_size) {
        fprintf(stderr, "error: failed reading file\n");
        free(content_buffer);
        return NULL;
    }
    return content_buffer;
}


int get_file_mode(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        fprintf(stderr, "error: failed getting file mode\n");
        return -1;
    }
    return st.st_mode;
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
    } else if (argc > 2 && strcmp(argv[1], "hash-object") == 0) {
        unsigned char *hash = handle_hash_object_command(argv[2]);
        if (hash != NULL) {
            free(hash);
            printf("Success hash-object\n");
        } else {
            fprintf(stderr, "Failed hash-object\n");
        }
    } else if (argc > 3 && strcmp(argv[1], "cat-file") == 0) {
        if (strcmp(argv[2], "-p") == 0) {
            handle_cat_file_command(argv[3], 'p');
        }
    } else if (argc > 2 && strcmp(argv[1], "add") == 0) {
        if (handle_add_command(argv[2]) == 0) {
            printf("Add success");
        }
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

    // Create HEAD file
    FILE *head_file = open_file(".jit/HEAD", "w");
    if (head_file == NULL) return 1;
    fputs("ref: refs/heads/main\n", head_file);
    fclose(head_file);
    // Create index file
    FILE *index_file = open_file(".jit/index", "w");
    if (index_file == NULL) return 1;
    fclose(index_file);

    return 0;
}


unsigned char *handle_hash_object_command(const char *file_name) {
    FILE *input_file = open_file(file_name, "r");
    if (input_file == NULL) return NULL;

    // Get file size
    long file_size = get_file_size(input_file);
    if (file_size == -1) {
        fclose(input_file);
        return NULL;
    }

    // Read and then close file
    char *content_buffer = read_file(input_file, file_size);
    fclose(input_file);
    if (content_buffer == NULL) {
        return NULL;
    }

    // Create header
    char header_buffer[64];
    int header_len = snprintf(header_buffer, sizeof(header_buffer), "blob %ld", file_size);
    header_len++;

    size_t total_size = header_len + file_size;
    char *data_buffer = malloc(total_size);
    if(data_buffer == NULL) {
        fprintf(stderr, "error: memory allocation failed\n");
        free(content_buffer);
        return NULL;
    }

    memcpy(data_buffer, header_buffer, header_len);
    memcpy(data_buffer + header_len, content_buffer, file_size);
    free(content_buffer);

    // Hash using SHA-1
    unsigned char *hash = malloc(SHA_DIGEST_LENGTH);
    SHA1((unsigned char *)data_buffer, total_size, hash);
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    // Create folder and file
    char dir_path[64];
    snprintf(dir_path, sizeof(dir_path), ".jit/objects/%02x", hash[0]);
    if (create_folder(dir_path, DIR_MODE) != 0) {
        free(data_buffer);
        return NULL;
    }
    char hash_hex[41];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(hash_hex + (i * 2), "%02x", hash[i]);
    }
    char file_path[128];
    snprintf(file_path, sizeof(file_path), ".jit/objects/%.2s/%.38s", hash_hex, hash_hex + 2);

    // Write to file
    FILE *output_file = open_file(file_path, "wb");
    if (output_file == NULL) {
        free(data_buffer);
        return NULL;
    }
    fwrite(data_buffer, 1, total_size, output_file);
    fclose(output_file);
    free(data_buffer);
    return hash;
}


int handle_cat_file_command(const char *hash, char mode) {
    // Open file
    char file_path[128];
    snprintf(file_path, sizeof(file_path), ".jit/objects/%.2s/%.38s", hash, hash + 2);
    FILE *input_file = open_file(file_path, "r");
    if (input_file == NULL) return 1;

    // Get file size
    long file_size = get_file_size(input_file);
    if (file_size == -1) {
        fclose(input_file);
        return 1;
    }

    // Read and close file
    char *content_buffer = read_file(input_file, file_size);
    fclose(input_file);
    if (content_buffer == NULL) return 1;

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


int handle_add_command(const char *file_name) {
    // Get hash and create object entry
    unsigned char *hash = handle_hash_object_command(file_name);
    if (hash == NULL) return 1;

    // Get file mode
    int mode = get_file_mode(file_name);
    if (mode == -1) return 1;

    // Open and append to index file
    FILE *index_file = open_file(".jit/index", "a");
    if (index_file == NULL) return 1;
    fprintf(index_file, "%06o ", mode);  // Octal like 100644
    for (int i = 0; i < 20; i++) {
        fprintf(index_file, "%02x", hash[i]);
    }
    fprintf(index_file, " %s\n", file_name);

    free(hash);
    fclose(index_file);
    return 0;
}


// int handle_write_tree_command() {

// }
