#include <stdio.h>
#include <string.h>

int set(int argc, char **argv);
int get(char *key);
int del(char *key);
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
            printf("    sdb get key=value key2=value2\n");
        }
        set(argc, argv);
    }
    return 0;
}

int set(int argc, char **argv) {
    return 0;
}
