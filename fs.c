#include "defs.h"
#include "fs.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int FD;
SuperBlock sb;


void move_ptr(int block_num, int offset) {
    lseek(FD, SUPERBLOCK_OFFSET + BLOCK_SIZE*block_num + offset, SEEK_SET);
}

void prev_dir(char* path) {
    int n = strlen(path);
    char * pos = strrchr(path, '/');
    if (pos == NULL) {
        return;
    }
    if (path == pos) {
        memset(pos + 1, 0, MAX_PATH_SIZE - 1);
        return;
    }
    memset(pos, 0, MAX_PATH_SIZE - (pos - path));
}

char* next_dir(char* path) {
    if (!path) return NULL;
    if (path[0] == '\0') return NULL;
    char* res = strchr(path, '/');
    if (res == NULL)
        res = strchr(path, '\0');
    return res;
}

void separate_dir_file(char* path, char* dir, char* file) {
    int n = strlen(path);
    char * pos = strrchr(path, '/');
    if (pos == NULL) {
        memcpy(file, path, MAX_NAME_SIZE);
        memset(dir, 0, MAX_PATH_SIZE);
        dir[0] = '.';
        return;
    }
    memcpy(dir, path, MAX_PATH_SIZE);
    memcpy(file, pos + 1, MAX_NAME_SIZE);
    prev_dir(dir);
}

void init_dir(int inode_ind, int parent_ind) {
    File dot = {.inode_index = inode_ind,
            .name = "."};
    File dotdot = {.inode_index = parent_ind,
            .name =".."};
    sb.iNodes[inode_ind].size = sizeof(File) * 2;
    move_ptr(sb.iNodes[inode_ind].block_ptr[0], 0);
    write(FD, &dot, sizeof(dot));
    write(FD, &dotdot, sizeof(dotdot));
}

int name_to_inode(int from_ind, char* name) {
    if (!strcmp(name, "/")) {
        return 1;
    }
    INode * inode = &sb.iNodes[from_ind];
    if (strchr(name, '/') == NULL) {
        for (int i = 0; i < BLOCKS_IN_INODE; ++i) {
            File buffer[BLOCK_SIZE / sizeof(File)];
            move_ptr(inode->block_ptr[i], 0);
            read(FD, buffer, BLOCK_SIZE/sizeof(File)*sizeof(File));
            for (int j = 0; j < BLOCK_SIZE / sizeof(File); ++j) {
                if (!strcmp(buffer[j].name, name)) {
                    return buffer[j].inode_index;
                }
            }
        }
        return -1;
    }
    if (name[0] == '/') {
        return name_to_inode(1, name + 1);
    }
    char * pos = strchr(name, '/');
    char buffer[MAX_PATH_SIZE] = {0};
    memcpy(buffer, name, pos - name);
    int next_inode = name_to_inode(from_ind, buffer);
    return name_to_inode(next_inode, pos + 1);
}

void init() {
    if (access(FS_PATH, F_OK) == 0) {
        FD = open(FS_PATH, O_RDWR);
        lseek(FD, 0, SEEK_SET);
        read(FD, &sb, sizeof(SuperBlock));
        return;
    }
    FD = open(FS_PATH, O_RDWR|O_CREAT);
    ftruncate(FD, 0);
    ftruncate(FD, DISK_SIZE);
    sb.total_blocks = BLOCK_NUM;
    sb.total_inodes = INODE_NUM;
    sb.free_blocks_num = BLOCK_NUM;
    sb.free_inodes_num = INODE_NUM;

    memset(sb.free_block, 0xff, sizeof(sb.free_block));
    memset(sb.free_inode, 0xff, sizeof(sb.free_inode));
    // Root dir initialization
    set0(sb.free_inode[0], 1);
    set0(sb.free_block[0], 1);
    sb.free_blocks_num--;
    sb.free_inodes_num--;
    sb.iNodes[1].attr |= DIR_FLAG;
    sb.iNodes[1].block_ptr[0] = 1;
    write(FD, &sb, sizeof(SuperBlock));
    init_dir(1, 1);
}

void ls(int from, char* name) {
    INode * inode = &sb.iNodes[name_to_inode(from, name)];
    if (!(inode->attr&DIR_FLAG)) {
        ERROR("This is not a directory.");
        return;
    }
    printf("Contents of the directory:\n");
    int file_count = inode->size / sizeof(File);
    for (int i = 0; i < BLOCKS_IN_INODE; ++i) {
        File buffer[BLOCK_SIZE / sizeof(File)];
        move_ptr(inode->block_ptr[i], 0);
        read(FD, buffer, BLOCK_SIZE/sizeof(File)*sizeof(File));
        for (int j = 0; j < BLOCK_SIZE / sizeof(File); ++j) {
            if (file_count <= 0) return;
            if (buffer[j].inode_index) {
                printf("\t%s\n", buffer[j].name);
                file_count--;
            }
        }
    }
}

int cd(int* cur_inode, char* cur_name, char* name) {
    INode* inode = &sb.iNodes[*cur_inode];
    for (int i = 0; i < BLOCKS_IN_INODE; ++i) {
        File buffer[BLOCK_SIZE / sizeof(File)];
        move_ptr(inode->block_ptr[i], 0);
        read(FD, buffer, BLOCK_SIZE/sizeof(File)*sizeof(File));
        for (int j = 0; j < BLOCK_SIZE / sizeof(File); ++j) {
            if (buffer[j].inode_index != 0 && !strcmp(buffer[j].name, name)) {
                *cur_inode = buffer[j].inode_index;
                if (!strcmp(buffer[j].name, ".")) {
                }
                else if (!strcmp(buffer[j].name, "..")) {
                    prev_dir(cur_name);
                }
                else {
                    if (strlen(cur_name) > 1) {
                        strcat(cur_name, "/");
                    }
                    strcat(cur_name, buffer[j].name);
                }
                return 1;
            }
        }
    }
    ERROR("No such directory");
    return -1;
}
void cd_path(int* cur_inode, char* cur_name, char* name) {
    int origin = *cur_inode;
    char original_name[MAX_PATH_SIZE] = {0};
    memcpy(original_name, cur_name, MAX_PATH_SIZE);
    int offset = 0;
    if (name[0] == '/') {
        *cur_inode = 1;
        offset++;
        memset(cur_name, 0, MAX_PATH_SIZE);
        cur_name[0] = '/';
    }
    char* pos = NULL;
    while (pos = next_dir(name + offset)) {
        char buffer[MAX_NAME_SIZE] = {0};
        memcpy(buffer, name + offset, pos - (name + offset));
        if (cd(cur_inode, cur_name, buffer) == -1) {
            *cur_inode = origin;
            memcpy(cur_name, original_name, MAX_PATH_SIZE);
            return;
        }
        offset = pos - name + 1;
    }
}


int get_free_inode() {
    for (int i = 0; i < sizeof(sb.free_inode); ++i) {
        if (sb.free_inode[i] > 0) {
            for (int j = 0; j < 8; ++j) {
                if (i == 0 && j == 0) continue; // always unused
                if (get(sb.free_inode[i], j)) {
                    set0(sb.free_inode[i], j);
                    sb.free_inodes_num--;
                    return 8*i+j;
                }
            }
        }
    }
    return -1;
}


int is_inode_used(int inode_ind) {
    return get(sb.free_inode[inode_ind / 8], inode_ind % 8);
}

void free_block(int ind) {
    set1(sb.free_block[ind / 8], ind % 8);
    sb.free_blocks_num++;
}

void free_inode(int ind) {
    set1(sb.free_inode[ind / 8], ind % 8);
    sb.free_inodes_num++;
}


int get_free_block() {
    for (int i = 0; i < sizeof(sb.free_block); ++i) {
        if (sb.free_block[i] > 0) {
            for (int j = 0; j < 8; ++j) {
                if (i == 0 && j == 0) continue;
                if (get(sb.free_block[i], j)) {
                    set0(sb.free_block[i], j);
                    sb.free_blocks_num--;
                    return 8*i+j;
                }
            }
        }
    }
    return -1;
}

void fs_append(int from, char * name, char* buffer) {
    int inode_ind = name_to_inode(from, name);
    if (inode_ind == -1) {
        ERROR("Such name does not exist.");
        return;
    }
    INode* inode = &sb.iNodes[inode_ind];
    if (inode->attr & DIR_FLAG) {
        ERROR("Cannot append to directory");
        return;
    }
    int n = strlen(buffer);
    int block_ind = inode->size / BLOCK_SIZE;
    int to_append = min(BLOCK_SIZE - inode->size % BLOCK_SIZE, n);
    lseek(FD, SUPERBLOCK_OFFSET + BLOCK_SIZE * inode->block_ptr[block_ind] + inode->size % BLOCK_SIZE, SEEK_SET);
    write(FD, buffer, to_append);
    int written = 0;
    written += to_append;
    while (written < n) {
        block_ind++;
        if (inode->block_ptr[block_ind] == 0) {
            inode->block_ptr[block_ind] = get_free_block();
            if (inode->block_ptr[block_ind] == -1) {
                return;
            }
        }
        lseek(FD, SUPERBLOCK_OFFSET + BLOCK_SIZE * inode->block_ptr[block_ind],SEEK_SET);
        to_append = min(n - written, BLOCK_SIZE);
        write(FD, buffer + written, to_append);
        written += to_append;
    }
    inode->size += n;
}

int fs_read(int inode_ind, char* buffer, uint32_t n, uint32_t offset) {
    INode* inode = &sb.iNodes[inode_ind];
    if (offset > inode->size) {
        ERROR("Offset is too big.");
        return 0;
    }
    n = min(n, inode->size - offset);
    int i = 0;
    int block_ind = offset / BLOCK_SIZE;
    int inside_block_ptr = offset % BLOCK_SIZE;
    lseek(FD, SUPERBLOCK_OFFSET + BLOCK_SIZE*inode->block_ptr[block_ind],SEEK_SET);
    char block[BLOCK_SIZE];
    read(FD, block, sizeof(block));
    for (; inside_block_ptr < BLOCK_SIZE && i < n; ++inside_block_ptr) {
        buffer[i++] = block[inside_block_ptr];
    }
    return i;
}


int touch(int where, char* name, char FLAGS) {
    if (sb.free_inodes_num == 0) {
        ERROR("Too many files. Not enough iNodes");
        return -1;
    }
    if (sb.free_blocks_num == 0) {
        ERROR("Not enough space.");
        return -1;
    }
    int inode_num = get_free_inode();
    if (inode_num == -1) {
        ERROR("Unexpected error.");
        return -1;
    }
    int initial_block = get_free_block();
    if (initial_block == -1) {
        ERROR("Unexpected error.");
        return -1;
    }
    char dir[MAX_PATH_SIZE];
    char file[MAX_NAME_SIZE];
    separate_dir_file(name, dir, file);
    where = name_to_inode(where, dir);
    if (name_to_inode(where, file) != -1) {
        ERROR("This name already exist.");
        return -1;
    }
    sb.iNodes[where].size += sizeof(File);
    sb.iNodes[inode_num].block_ptr[0] = initial_block;

    sb.iNodes[inode_num].size = 0;
    INode * inode = &sb.iNodes[where];
    for (int i = 0; i < BLOCKS_IN_INODE; ++i) {
        File buffer[BLOCK_SIZE / sizeof(File)] ={0};
        move_ptr(inode->block_ptr[i], 0);
        read(FD, buffer, sizeof(buffer));
        char found = 0;
        for (int j = 0; j < BLOCK_SIZE / sizeof(File); ++j) {
            if (!buffer[j].inode_index) {
                buffer[j].inode_index = inode_num;
                memcpy(buffer[j].name, file, MAX_NAME_SIZE);
                found = 1;
                break;
            }
        }
        if (found) {
            move_ptr(inode->block_ptr[i], 0);
            write(FD, buffer, sizeof(buffer));
            break;
        }
    }
    sb.iNodes[inode_num].attr |= FLAGS;
    if (sb.iNodes[inode_num].attr & DIR_FLAG) {
        init_dir(inode_num, where);
    }
    return inode_num;
}

void cat(int from, char* name) {
    int inode_ind = name_to_inode(from, name);
    if (inode_ind == -1){
        ERROR("Such name does not exist.");
        return;
    }
    INode* inode =&sb.iNodes[inode_ind];
    if (inode->attr & DIR_FLAG) {
        ERROR("Cannot cat a directory");
        return;
    }
    char buffer[BLOCK_SIZE];
    int already_read = 0;
    while (already_read < inode->size) {
        int cur = fs_read(inode_ind, buffer, BLOCK_SIZE, already_read);
        printf("%s", buffer);
        memset(buffer, 0, cur);
        already_read += cur;
    }
}

void rm_inode(int where, int target) {

    INode * inode = &sb.iNodes[target];
    INode * dir_inode = &sb.iNodes[where];
    if (inode->attr & DIR_FLAG) {
        if (inode->size > 2 * sizeof(File)) {
            ERROR("Directory is not empty.");
            return;
        }
    }
    for (int i = 0; i < BLOCKS_IN_INODE; ++i) {
        free_block(inode->block_ptr[i]);
        inode->block_ptr[i] = 0;
    }
    free_inode(target);
    for (int i = 0; i < BLOCKS_IN_INODE; ++i) {
        File buffer[BLOCK_SIZE / sizeof(File)] ={0};
        move_ptr(dir_inode->block_ptr[i], 0);
        read(FD, buffer, sizeof(buffer));
        for (int j = 0; j < BLOCK_SIZE / sizeof(File); ++j) {
            if (buffer[j].inode_index == target) {
                dir_inode->size -=sizeof(File);
                memset(&buffer[j], 0, sizeof(buffer[j]));
                move_ptr(dir_inode->block_ptr[i], 0);
                write(FD, buffer, sizeof(buffer));
                return;
            }
        }
    }
}

void rm(int where, char* name) {
    if (!strcmp(name, ".") || !strcmp(name, "..") || !strcmp(name, "/")) {
        ERROR("Cannot delete special directory.");
        return;
    }
    char dir[MAX_PATH_SIZE];
    char file[MAX_NAME_SIZE];
    separate_dir_file(name, dir, file);
    int dir_inode= name_to_inode(where, dir);
    where = dir_inode;
    if (dir_inode == -1) {
        ERROR("Such name does not exist.");
        return;
    }
    int file_inode = name_to_inode(where, file);
    rm_inode(dir_inode, file_inode);
}


void clean_up() {
    lseek(FD, 0, SEEK_SET);
    write(FD, &sb, sizeof(sb));
    close(FD);
}
