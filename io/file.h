#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <stdint.h>

#define SYSTEM_SIZE 2*1024*1024
#define INODE_SIZE  32
#define BLOCK_SIZE  512
#define NUM_BLOCKS  4096

extern uint8_t *system_addr;
extern struct directory * root;
extern struct directory * curdir;
extern char path[256];

typedef struct super_block{
    uint32_t magic_num;
    uint32_t total_blocks;
    uint32_t total_inodes;
}super_block;

/* use a block to indicate which blocks is available */
typedef struct vector_block{
    uint8_t vec[BLOCK_SIZE];  //4096 bits
}vector_block;

typedef struct block{
    uint8_t space[BLOCK_SIZE];
}block;

/* store inodes info into block 2-9 */
typedef struct inode{
    uint32_t file_size;
    uint32_t flags;
    uint16_t blocks[10];
    uint16_t single_block;
    uint16_t double_block;
}inode;

typedef struct dentry{
    uint8_t inode_idx;
    uint8_t filename[31];
}dentry;

typedef struct directory{
    struct dentry entry[16];
}directory;

void fatalsys(const char *errmsg);

void readBlock(FILE* disk, int blockNum, char* buffer);
void writeBlock(FILE* disk, int blockNum, char* data);

int32_t allocate_block(uint16_t blockSize);
void release_block(uint16_t block_num, uint16_t sz);

uint8_t *get_blockaddr(uint16_t block_num);
uint16_t get_blocknum(uint8_t *addr);

struct inode *allocate_inode();
void release_inode(uint16_t inode_idx);

struct inode *get_inodeaddr(uint8_t inode_idx);
uint8_t get_inodenum(struct inode *addr);

int32_t create_inode(struct inode *node, uint16_t block_num, uint16_t sz, uint32_t flags);
int8_t find_entry(struct directory *dir, const char* filename, uint32_t flags);
int8_t find_free_entry(struct directory *dir);
int32_t add_todir(const char *filename, struct directory *dir, uint8_t inode_num);

int32_t release_file(struct inode *node);
int32_t release_entry(struct directory *dir, uint8_t entry_idx);

int32_t create_file(char *filename, uint16_t sz);
int32_t delete_file(const char* filename);

int32_t read_file(const char* filename, int len);
int32_t write_file(const char* filename, const char* content);

int32_t create_subdir(const char* dirname);
int32_t delete_subdir(const char* dirname);

void init_rootdir();
int32_t change_workdir(const char* dirname);

#endif // __FILE_H__
