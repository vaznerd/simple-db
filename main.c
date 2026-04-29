#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int ensure_dir(const char *filepath);
char *expand_tilde(const char *path);
char *get_pair(FILE *fp, char *pair);
int line_no_of_pair(FILE *fp, char *key);

int set(int argc, char **argv);
int get(char *key);
int del(char *key);
int update(char *key);
int dedupe(void);
int exists(char *key);
int keys(char *regex);
int help(void);
int list(void);

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: sdb <command> [args]\n");
        printf("Commands:\n");
        printf("    set key=value    Set key to value\n");
        printf("    get key          Get value for key\n");
        printf("    -h, --help       Show this help\n");
        return 0;
    }
    if (strcmp(argv[1], "set") == 0) {
        if (argc == 2) {
            printf("Example usage of sdb set:\n");
            printf("    sdb set key=value key2=value2\n");
            return 1;
        }
        set(argc, argv);
    } else if (strcmp(argv[1], "get") == 0) {
        if (argc == 2) {
            printf("Example usage of sdb get:\n");
            printf("    sdb get key\n");
            return 1;
        }
        get(argv[2]);
    } else {
        printf("Usage: sdb <command> [args]\n");
        printf("Commands:\n");
        printf("    set key=value    Set key to value\n");
        printf("    get key          Get value for key\n");
        printf("    -h, --help       Show this help\n");
        return 1;
    }
    return 0;
}

int set(int argc, char **argv) {
    const char *file_name = "~/.local/share/sdb/database.txt";
    char *real_path = expand_tilde(file_name);
    if (!real_path) {
        fprintf(stderr, "Cannot expand path or $HOME missing\n");
        return 1;
    }

    if (ensure_dir(real_path) != 0) {
        free(real_path);
        return 1;
    }

    FILE *fp = fopen(real_path, "a");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    for (int i = 2; i < argc; i++) {
        if (argv[i][0] == '=') {
            printf("Key's first character cannot be '='\n");
            return 1;
        }
        char *position = strchr(argv[i], '=');
        if (!position) {
            printf("Give a key value pair as key=value.\n");
            return 1;
        }
        fprintf(fp, "%s\n", argv[i]);
    }
    fclose(fp);
    return 0;
}

int get(char *key) {
    const char *file_name = "~/.local/share/sdb/database.txt";
    char *real_path = expand_tilde(file_name);
    if (!real_path) {
        fprintf(stderr, "Cannot expand path to database or $HOME missing\n");
        return 1;
    }

    if (ensure_dir(real_path) != 0) {
        free(real_path);
        fprintf(stderr, "database file does not exist\n");
        return 1;
    }

    FILE *fp = fopen(real_path, "r");
    free(real_path);
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char *pair;
    char *malloced_pair;
    malloced_pair = malloc(sizeof(char));
    while (NULL != (pair = get_pair(fp, malloced_pair))) {
        char temp_string[strlen(pair) + 1];
        strcpy(temp_string, pair);
        char *position = strchr(temp_string, '=');
        if (position != NULL) {
            *position = '\0';
        }
        if (strcmp(key, temp_string) == 0) {
            printf("%s\n", pair);
            free(pair);
            return 0;
        }
        free(pair);
    }
    printf("Pair does not exist\n");
    return 1;
}

int del(char *key) {
    const char *file_name = "~/.local/share/sdb/database.txt";
    char *real_path = expand_tilde(file_name);
    if (!real_path) {
        fprintf(stderr, "Cannot expand path to database or $HOME missing\n");
        return 1;
    }

    if (ensure_dir(real_path) != 0) {
        free(real_path);
        fprintf(stderr, "database file does not exist\n");
        return 1;
    }

    FILE *fp = fopen(real_path, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    int line_no;
    line_no = line_no_of_pair(fp, key);
    if (line_no == -1) {
        return 1;
    }
    FILE *temp_fp;
    temp_fp = fopen("/tmp/sdb/temp_db.txt", "w");
    if (!temp_fp) {
        perror("fopen in del()");
        return 1;
    }

    char *pair;
    char *malloced_pair;
    int current_line_no = 0;
    malloced_pair = malloc(sizeof(char));
    while (NULL != (pair = get_pair(fp, malloced_pair))) {
        if (line_no != current_line_no) {
            continue;
        } else {
            if (fputs(pair, fp)) {
                perror("fputs in del()");
                return 1;
            }
        }
        current_line_no++;
    }

    free(pair);
    fclose(fp);
    fclose(temp_fp);
    remove("~/.local/share/sdb/database.txt");
    rename("/tmp/sdb/temp_db.txt", "~/.local/share/sdb/database.txt");
    return 0;
}

int line_no_of_pair(FILE *fp, char *key) {
    int line_no = 0;
    char *pair;
    char *malloced_pair;
    malloced_pair = malloc(sizeof(char));
    while (NULL != (pair = get_pair(fp, malloced_pair))) {
        char temp_string[strlen(pair) + 1];
        strcpy(temp_string, pair);
        char *position = strchr(temp_string, '=');
        if (position != NULL) {
            *position = '\0';
        }
        if (strcmp(key, temp_string) == 0) {
            printf("%s\n", pair);
            free(pair);
            return line_no;
        }
        line_no++;
        free(pair);
    }
    free(pair);
    printf("Pair does not exist\n");
    return -1;
}

char *get_pair(FILE *fp, char *pair) {
    size_t len = 0;
    int character;
    size_t size = sizeof(character);
    if (!pair)
        return pair;
    while (EOF != (character = fgetc(fp)) && character != '\n') {
        pair[len++] = character;
        if (len == size) {
            size += size;
            pair = realloc(pair, size);
            if (!pair)
                return pair;
        }
    }
    pair[len++] = '\0';
    if (character == EOF) {
        return NULL;
    }
    return realloc(pair, len);
}

char *expand_tilde(const char *path) {
    if (path[0] != '~' || path[1] != '/')
        return NULL;

    const char *home_path = getenv("HOME");
    if (!home_path)
        return NULL;

    size_t home_len = strlen(home_path);
    size_t rest_len = strlen(path + 2);
    // final_path = /home/piyush + / + .local/share/sdb + \0
    char *final_path = malloc(home_len + 1 + rest_len + 1);
    if (!final_path)
        return NULL;

    strcpy(final_path, home_path);
    final_path[home_len] = '/';
    strcpy(final_path + home_len + 1, path + 2);

    return final_path;
}

int ensure_dir(const char *filepath) {
    size_t filepath_len = strlen(filepath + 1);
    char copy[filepath_len];
    strcpy(copy, filepath);

    char *last = strrchr(copy, '/');
    if (!last) {
        perror("strrchr in ensure_dir()");
        return 1;
    }

    *last = '\0';

    if (mkdir(copy, 0755) == 0) {
        printf("Created directory: %s\n", copy);
    } else if (errno != EEXIST) {
        perror("mkdir in ensure_dir()");
        return 1;
    }
    return 0;
}
