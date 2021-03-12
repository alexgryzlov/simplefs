#ifndef SIMPLEFS_TERMINAL_H
#define SIMPLEFS_TERMINAL_H
#include <stdio.h>

void show_usage();
void execute(char* command, int* cur_inode, char* cur_dir);
void run_terminal();

#endif //SIMPLEFS_TERMINAL_H
