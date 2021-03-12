#ifndef SIMPLEFS_FS_H
#define SIMPLEFS_FS_H

void move_ptr(int block_num, int offset);

void prev_dir(char* path);

char* next_dir(char* path);

void separate_dir_file(char* path, char* dir, char* file);

void init_dir(int inode_ind, int parent_ind);

int name_to_inode(int from_ind, char* name);

void init();

void ls(int from, char* name);

int cd(int* cur_inode, char* cur_name, char* name);

void cd_path(int* cur_inode, char* cur_name, char* name);

int get_free_inode();

int is_inode_used(int inode_ind);

void free_block(int ind);

void free_inode(int ind);

int get_free_block();

void fs_append(int from, char * name, char* buffer);

int fs_read(int inode_ind, char* buffer, uint32_t n, uint32_t offset);

int touch(int where, char* name, char FLAGS);

void cat(int from, char* name);

void rm_inode(int where, int target);

void rm(int where, char* name);

void clean_up();

#endif //SIMPLEFS_FS_H
