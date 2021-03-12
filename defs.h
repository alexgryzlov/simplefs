#ifndef SIMPLEFS_DEFS_H
#define SIMPLEFS_DEFS_H
#include <inttypes.h>

#define ERROR(msg) printf("[ERROR][%d:%s] %s\n", __LINE__, __FUNCTION__, msg)
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define BLOCKS_IN_INODE 15
typedef struct {
    int32_t block_ptr[BLOCKS_IN_INODE];
    int32_t next;
    char attr;
    uint32_t size;
} INode;

#define MAX_NAME_SIZE 28
#define MAX_PATH_SIZE 128
#define BLOCK_SIZE 1024
#define BLOCK_NUM 8192
#define INODE_NUM 1024
#define FS_PATH "simplefs.data"

typedef struct {
    uint32_t inode_index;
    char name[MAX_NAME_SIZE];
} File;
extern int FD;
typedef struct {
    uint16_t total_blocks;
    uint16_t total_inodes;
    uint16_t free_blocks_num;
    uint16_t free_inodes_num;
    unsigned char free_block[BLOCK_NUM / 8];
    unsigned char free_inode[INODE_NUM / 8];
    INode iNodes[INODE_NUM];
} SuperBlock;

extern SuperBlock sb;

#define SUPERBLOCK_OFFSET sizeof(SuperBlock)
#define DISK_SIZE BLOCK_SIZE*BLOCK_NUM+SUPERBLOCK_OFFSET

#define set1(x, pos) (x)|=(1<<pos)
#define set0(x, pos) (x)&=(~(1<<pos))
#define get(x, pos) (x)&(1<<pos)
#define DIR_FLAG 1
#endif //SIMPLEFS_DEFS_H
