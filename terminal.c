#include "terminal.h"
#include <string.h>
#include "defs.h"
#include "fs.h"

void show_usage() {
    printf("Disk is simulated in file \"simplefs.data\".\n"
            "Supported operations:\n"
           "\t- ls <directory>\n"
           "\t- cd <directory>\n"
           "\t- touch <filename>\n"
           "\t- mkdir <directory_name>\n"
           "\t- append <filename> \"data\" (quotes are necessery)\n"
           "\t- cat <filename>\n"
           "\t- rm <name> (works with files and empty dirs)\n"
           "\t- exit (save superblock and exit)\n\n");
}

void execute(char* command, int* cur_inode, char* cur_dir) {
    if (!strncmp(command, "cat", 3)) {
        char buffer[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", buffer);
        cat(*cur_inode, buffer);
    }
    else if (!strncmp(command, "append", 6)) {
        char name[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", name);
        char* ptr = strchr(command, '"');
        char* end = strchr(ptr + 1, '"');
        char buffer[1024] = {0};
        memcpy(buffer, ptr + 1, end - ptr - 1);
        fs_append(*cur_inode, name, buffer);
    }
    else if (!strncmp(command, "ls", 2)) {
        char buffer[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", buffer);
        if (buffer[0] == '\0') buffer[0] = '.';
        ls(*cur_inode, buffer);
    }
    else if (!strncmp(command, "cd", 2)) {
        char buffer[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", buffer);
        cd_path(cur_inode, cur_dir, buffer);
    }
    else if (!strncmp(command, "touch", 5)) {
        char buffer[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", buffer);
        touch(*cur_inode, buffer, 0);
    }
    else if (!strncmp(command, "mkdir", 5)) {
        char buffer[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", buffer);
        touch(*cur_inode, buffer, DIR_FLAG);
    }
    else if (!strncmp(command, "rm", 2)) {
        char buffer[MAX_PATH_SIZE] = {0};
        sscanf(command, "%*s %s", buffer);
        rm(*cur_inode, buffer);
    }
    else if (!strncmp(command, "exit", 4)) {
        clean_up();
        exit(0);
    }
    else if (!strcmp(command, "help")) {
        show_usage();
    }
    else {
        ERROR("Command not found");
    }
}

void run_terminal() {
    show_usage();
    const char terminal_name[] = "simplefs> ";
    int cur_inode = 1;
    char cur_dir[4096] = "/";
    for(;;) {
        printf("%s\n", cur_dir);
        printf("%s", terminal_name);
        char buffer[4096] = {0};
        int ptr = 0;
        char x;
        while (x = getchar()) {
            if (x == '\n' || x == '\0') {
                if (ptr == 0)
                    break;
                ptr = 0;
                execute(buffer, &cur_inode, cur_dir);
                memset(buffer, 0, sizeof(buffer));
                printf("\n");
                break;
            } else {
                buffer[ptr++] = x;
            }
        }
    }
}
