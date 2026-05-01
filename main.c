#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int ensure_dir(const char *filepath);
char *expand_tilde(const char *path);
char *get_pair(FILE *fp);
int line_no_of_pair(FILE *fp, char *key);
static char *my_strdup(const char *s);

int set(int argc, char **argv);
int get(char *key);
int del(char *key);
int update(char *updated_pair, char **argv);
int dedupe(void);
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
    } else if (strcmp(argv[1], "del") == 0) {
        if (argc == 2) {
            printf("Example usage of sdb del:\n");
            printf("    sdb del key\n");
            return 1;
        }
        del(argv[2]);
    } else if (strcmp(argv[1], "update") == 0) {
        if (argc == 2) {
            printf("Example usage of sdb update:\n");
            printf("    sdb del key=value\n");
            return 1;
        }
        update(argv[2], argv);
    } else if (strcmp(argv[1], "dedupe") == 0) {
        dedupe();
    } else {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            help();
            return 0;
        }
        printf("No such subcommand: %s\n", argv[1]);
        printf("Use sdb --help for help\n");
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
    while (NULL != (pair = get_pair(fp))) {
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

    FILE *fin = fopen(real_path, "r");
    if (!fin) {
        perror("fopen in del()");
        return 1;
    }

    int line_no;
    line_no = line_no_of_pair(fin, key);
    if (line_no == -1) {
        fclose(fin);
        return 1;
    }

    const char *temp_file = "~/.local/share/sdb/tmp.txt";
    char *temp_path = expand_tilde(temp_file);
    if (!real_path) {
        fprintf(stderr, "Cannot expand path to database or $HOME missing\n");
        return 1;
    }

    FILE *fout = fopen(temp_path, "w");
    if (!fout) {
        perror("fopen in del()");
        fclose(fin);
        return 1;
    }

    char *pair;
    char *malloced_pair;
    int current_line_no = 0;
    malloced_pair = malloc(sizeof(char));
    if (!malloced_pair) {
        perror("malloc in del()");
        free(malloced_pair);
        fclose(fin);
        fclose(fout);
        return 1;
    }

    rewind(fin);
    while (NULL != (pair = get_pair(fin))) {
        if (line_no == current_line_no) {
            current_line_no++;
            continue;
        }
        if (fputs(pair, fout) == EOF) {
            perror("fputs in del()");
            free(pair);
            fclose(fin);
            fclose(fout);
            return 1;
        }
        fputc('\n', fout);
        current_line_no++;
    }
    free(pair);
    if (remove(real_path) == 0) {
        if (rename(temp_path, real_path) == -1) {
            fprintf(stderr, "Rename failed: %s (errno %d)\n", strerror(errno),
                    errno);
        }
    } else {
        perror("remove failed");
        return 1;
    }
    fclose(fin);
    fclose(fout);
    return 0;
}

int update(char *updated_pair, char **argv) {
    if (argv[2][0] == '=') {
        printf("Key's first character cannot be '='\n");
        return 1;
    }
    char *check_position = strchr(argv[2], '=');
    if (!check_position) {
        printf("Give a key value pair as key=value.\n");
        return 1;
    }
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

    FILE *fin = fopen(real_path, "r");
    if (!fin) {
        perror("fopen in del()");
        return 1;
    }

    char key[strlen(updated_pair) + 1];
    strcpy(key, updated_pair);
    char *position = strchr(key, '=');
    if (position != NULL) {
        *position = '\0';
    }

    int line_no;
    line_no = line_no_of_pair(fin, key);
    if (line_no == -1) {
        fclose(fin);
        return 1;
    }

    const char *temp_file = "~/.local/share/sdb/tmp.txt";
    char *temp_path = expand_tilde(temp_file);
    if (!real_path) {
        fprintf(stderr, "Cannot expand path to database or $HOME missing\n");
        return 1;
    }

    FILE *fout = fopen(temp_path, "w");
    if (!fout) {
        perror("fopen in del()");
        fclose(fin);
        return 1;
    }

    char *pair;
    char *malloced_pair;
    int current_line_no = 0;
    malloced_pair = malloc(sizeof(char));
    if (!malloced_pair) {
        perror("malloc in del()");
        free(malloced_pair);
        fclose(fin);
        fclose(fout);
        return 1;
    }

    rewind(fin);
    while (NULL != (pair = get_pair(fin))) {
        if (line_no == current_line_no) {
            if (fputs(updated_pair, fout) == EOF) {
                perror("fputs in del()");
                free(pair);
                fclose(fin);
                fclose(fout);
                return 1;
            }
            fputc('\n', fout);
            current_line_no++;
            continue;
        }
        if (fputs(pair, fout) == EOF) {
            perror("fputs in del()");
            free(pair);
            fclose(fin);
            fclose(fout);
            return 1;
        }
        fputc('\n', fout);
        current_line_no++;
    }
    free(pair);
    if (remove(real_path) == 0) {
        if (rename(temp_path, real_path) == -1) {
            fprintf(stderr, "Rename failed: %s (errno %d)\n", strerror(errno),
                    errno);
        }
    } else {
        perror("remove failed");
        return 1;
    }
    fclose(fin);
    fclose(fout);
    return 0;
}

int dedupe(void) {
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

    FILE *fin = fopen(real_path, "r");
    if (!fin) {
        perror("fopen in del()");
        return 1;
    }

    const char *temp_file = "~/.local/share/sdb/tmp.txt";
    char *temp_path = expand_tilde(temp_file);
    if (!temp_path) {
        fprintf(stderr, "Cannot expand path to database or $HOME missing\n");
        return 1;
    }

    FILE *fout = fopen(temp_path, "w");
    if (!fout) {
        perror("fopen in dedupe()");
        fclose(fin);
        return 1;
    }

    char *pair;
    char c;
    size_t length = 0;
    for (c = getc(fin); c != EOF; c = getc(fin))
        if (c == '\n')
            length++;
    char **seen = malloc(length * sizeof(char *));
    if (!seen) {
        perror("malloc in dedupe()");
        return 1;
    }

    size_t seen_count = 0;
    int duplicate = 0;
    rewind(fin);
    while ((pair = get_pair(fin)) != NULL) {
        duplicate = 0;
        for (size_t i = 0; i < seen_count; i++) {
            if (strcmp(seen[i], pair) == 0) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            seen[seen_count] = my_strdup(pair);
            if (!seen[seen_count++]) {
                fprintf(stderr, "strdup failed\n");
                fclose(fin);
                fclose(fout);
                return 1;
            }
            if (fputs(pair, fout) == EOF || fputc('\n', fout) == EOF) {
                perror("fputs in dedupe()");
                free(pair);
                fclose(fin);
                fclose(fout);
                return 1;
            }
        }
    }
    free(pair);
    for (size_t i = 0; i < seen_count; i++) {
        free(seen[i]);
    }
    free(seen);
    if (remove(real_path) == 0) {
        if (rename(temp_path, real_path) == -1) {
            fprintf(stderr, "Rename failed: %s (errno %d)\n", strerror(errno),
                    errno);
        }
    } else {
        perror("remove failed");
        return 1;
    }
    fclose(fin);
    fclose(fout);
    return 0;
}

int keys(char *regex){
    return 0;
}

int help(void) {
    printf("Usage: sdb <command> [args]\n");
    printf("Commands:\n");
    printf("    set key=value    Set key to value\n");
    printf("    del key          delete a pair\n");
    printf("    get key          Get value for key\n");
    printf("    list             List all the pairs\n");
    printf("    keys regex       List all the pairs with that regex pattern\n");
    printf("    -h, --help       Show this help\n");
    return 0;
}

static char *my_strdup(const char *source) {
    size_t len = strlen(source) + 1;
    char *dest = malloc(len);
    if (dest)
        memcpy(dest, source, len);
    return dest;
}

int line_no_of_pair(FILE *fp, char *key) {
    int line_no = 0;
    char *pair;
    char *malloced_pair;
    malloced_pair = malloc(sizeof(char));
    if (!malloced_pair) {
        perror("malloc in line_no_of_pair()");
        return 1;
    }
    while (NULL != (pair = get_pair(fp))) {
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

char *get_pair(FILE *fp) {
    size_t offset = 0;
    int character;
    size_t bufsize = 4;
    char *pair;
    pair = malloc(bufsize);
    if (!pair) {
        perror("malloc in get()");
        return NULL;
    }
    while (character = fgetc(fp), character != '\n' && character != EOF) {
        // while (EOF != (character = fgetc(fp)) && character != '\n') {
        if (offset == bufsize) {
            bufsize *= bufsize;
            pair = realloc(pair, bufsize);
            if (!pair) {
                free(pair);
                return pair;
            }
        }
        pair[offset++] = character;
    }
    if (character == EOF && offset == 0) {
        return NULL;
    }
    pair[offset++] = '\0';
    return realloc(pair, offset);
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
