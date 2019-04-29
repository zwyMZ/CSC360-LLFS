#include <stdlib.h>
#include <string.h>
#include "file.h"

struct directory * root;
struct directory * curdir;
char path[256]={};

uint8_t *system_addr;

/* print error message and die */
void fatalsys(const char *errmsg)
{
	perror(errmsg);
	exit(EXIT_FAILURE);
}

void readBlock(FILE* disk, int blockNum, char* buffer)
{
    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, disk);
}

void writeBlock(FILE* disk, int blockNum, char* data)
{
    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
    fwrite(data, BLOCK_SIZE, 1, disk); // Note: this will overwrite existing data in the block
}

/* find consequent blocks in freelist blocks */
int32_t allocate_block(uint16_t blockSize)
{
    uint16_t cnt=0;
    uint16_t start_blk=0;
    struct vector_block *vb = (struct vector_block *)get_blockaddr(1);
    for(int ibit=0; ibit<NUM_BLOCKS; ++ibit){
        if(!(vb->vec[ibit/8] & (uint8_t)(1<<(ibit%8)))){
            if(cnt == 0)
                start_blk = ibit;
            cnt++;
            if(cnt == blockSize){
                /* find free blocks and set up the vector block */
                for(int k=start_blk; k<start_blk+blockSize; ++k){
                    vb->vec[k/8] |= (uint8_t)(1<<(k%8));
                }
                return start_blk;
            }
        } else {
            cnt = 0;
        }
    }
    printf("not found consequent free blocks\n");
    return -1;
}

void release_block(uint16_t block_num, uint16_t sz)
{
    struct vector_block *vb = (struct vector_block *)get_blockaddr(1);
    for(int i=block_num; i<block_num+sz; ++i){
        vb->vec[i/8] &= ~(1<<(i%8));
    }
}

uint8_t *get_blockaddr(uint16_t block_num)
{
    return system_addr + block_num * BLOCK_SIZE;
}

uint16_t get_blocknum(uint8_t *addr)
{
    return (addr - system_addr) / BLOCK_SIZE;
}

struct inode *allocate_inode()
{
    struct super_block *sb = (struct super_block *)get_blockaddr(0);
    struct block *inode_blk = (struct block *)get_blockaddr(8);
    struct inode *free_inode;

    if(sb->total_inodes == 4095){
        perror("no free inode");
        return NULL;
    }
    sb->total_inodes++;
    free_inode = get_inodeaddr(sb->total_inodes);

    return free_inode;
}

void release_inode(uint16_t inode_idx)
{
    struct super_block *sb = (struct super_block *)get_blockaddr(0);
    struct block *inode_blk = (struct block *)get_blockaddr(8);
    for(int i=inode_idx; i<sb->total_inodes; ++i){
        struct inode *id = get_inodeaddr(i);
        *id = *(id+1);
    }

    sb->total_inodes--;
}

struct inode *get_inodeaddr(uint8_t inode_idx)
{
    struct block *inode_blk = (struct block *)get_blockaddr(8);
    struct inode *addr = (struct inode *)(&inode_blk[inode_idx/16])
                                + (inode_idx%16);
    return addr;
}

uint8_t get_inodenum(struct inode *addr)
{
    struct block *inode_blk = (struct block *)get_blockaddr(8);
    uint16_t inode_idx = addr - (struct inode *)inode_blk;

    return inode_idx;
}

int32_t create_inode(struct inode *node, uint16_t block_num, uint16_t sz, uint32_t flags)
{
    for(int i=0; i<sz; ++i){
        node->blocks[i] = block_num+i;
    }
    node->file_size = sz;
    node->flags = flags;        //0 stand for a file
    node->double_block = 0;
    node->single_block = 0;

    return 0;
}

int8_t find_free_entry(struct directory *dir)
{
    for(int i=0; i<16; ++i){
        if(dir->entry[i].inode_idx == 0)
            return i;
    }

    return -1;
}

int8_t find_entry(struct directory *dir, const char* filename, uint32_t flags)
{
    for(int i=0; i<16; ++i){
        if(!strncmp(dir->entry[i].filename,
                        filename, strlen(filename))){
            struct inode *node = get_inodeaddr(dir->entry[i].inode_idx);
            if(node->flags == flags)
                return i;
        }
    }

    return -1;
}

int32_t add_todir(const char *filename, struct directory *dir, uint8_t inode_num)
{
    int8_t free_entry = find_free_entry(dir);
    if(free_entry == -1){
        perror("entry is full");
        return -1;
    }
    dir->entry[free_entry].inode_idx = inode_num;
    strncpy(dir->entry[free_entry].filename, filename, 32);

    return 0;
}

int32_t create_file(char *filename, uint16_t sz)
{
    /* check valid filename */
    if(strlen(filename) > 30){
        perror("file name too long");
        return -1;
    }
    /* allocate inode */
    struct inode *node = allocate_inode();
    if(node == NULL){
        return -1;
    }
    /* allocate data block */
    int32_t file_block = allocate_block(sz);
    if(file_block == -1){
        return -1;
    }
    /* fill inode-info */
    if(create_inode(node, file_block, sz, 0) == -1){
        return -1;
    }
    /* add the file into directory */
    uint8_t inode_num = get_inodenum(node);
    if(add_todir(filename, curdir, inode_num) == -1){
        return -1;
    }
    return 0;
}

int32_t release_file(struct inode *node)
{
    release_block(node->blocks[0], node->file_size);

    return 0;
}

int32_t release_entry(struct directory *dir, uint8_t entry_idx)
{
    for(int i=entry_idx; i<16; ++i){
        dir->entry[i] = dir->entry[i+1];
    }
    int8_t entry = find_free_entry(dir);
    if(entry == -1){
        memset(&dir->entry[15], 0, 32);
    }

    return 0;
}

int32_t delete_file(const char* filename)
{
    /* find the entry */
    int8_t entry_idx = find_entry(curdir, filename, 0);
    if(entry_idx == -1){
        perror("filename not found");
        return -1;
    }
    struct dentry *dty = &curdir->entry[entry_idx];
    struct inode *node = get_inodeaddr(dty->inode_idx);
    /* judge the type */

    /* release file block */
    release_file(node);

    /* release the inode */
    uint16_t inode_num = get_inodenum(node);
    release_inode(inode_num);

    /* release the entry */
    release_entry(curdir, entry_idx);

    return 0;
}

/* change work directory */
int32_t change_workdir(const char* dirname)
{
    int8_t dir_entry = find_entry(curdir, dirname, 1);
    if(dir_entry == -1){
        perror("not found the dir");
        return -1;
    }
    uint8_t dir_inodenum = curdir->entry[dir_entry].inode_idx;
    struct inode *node = get_inodeaddr(dir_inodenum);
    struct directory *dir = (struct directory *)get_blockaddr(node->blocks[0]);
    curdir = dir;

    return 0;
}

/* read file */
int32_t read_file(const char* filename, int len)
{
    int8_t file_entry = find_entry(curdir, filename, 0);
    if(file_entry == -1){
        perror("file not found");
        return -1;
    }
    uint8_t node_num = curdir->entry[file_entry].inode_idx;
    struct inode *node = get_inodeaddr(node_num);

    if(len > node->file_size*512){
        perror("length too long");
        return -1;
    }

    uint16_t file_block = node->blocks[0];
    struct block *file = (struct block *)get_blockaddr(file_block);
    for(int i=0; i<len; ++i){
        printf("%c", *((char *)file+i));
    }
    printf("\n");

    return 0;
}

/* write file */
int32_t write_file(const char* filename, const char* content)
{
    int8_t file_entry = find_entry(curdir, filename, 0);
    if(file_entry == -1){
        perror("file not found");
        return -1;
    }
    uint8_t node_num = curdir->entry[file_entry].inode_idx;
    struct inode *node = get_inodeaddr(node_num);

    if(strlen(content) > node->file_size*512){
        perror("length too long");
        return -1;
    }

    uint8_t nblock = strlen(content) / 512;
    uint8_t nbyte = strlen(content) % 512;
    uint16_t file_block = node->blocks[0];
    struct block *file = (struct block *)get_blockaddr(file_block);

    uint8_t i;
    for(i=0; i<nblock; ++i){
        memcpy((void *)(file+i), (void *)(content+i*512), 512);
    }
    memcpy((void *)(file+i), (void *)(content+i*512), nbyte);

    return 0;
}

uint8_t find_free_dir(const char* dirname)
{

}

/* create a sub-directory */
int32_t create_subdir(const char* dirname)
{
    static uint8_t subdir_cnt = 0;
    subdir_cnt++;

    /* check valid filename */
    if(strlen(dirname) > 30){
        perror("dirname too long");
        return -1;
    }
    /* allocate inode */
    struct inode *node = allocate_inode();
    if(node == NULL){
        return -1;
    }
    /* fill dir inode-info */
    if(create_inode(node, subdir_cnt, 1, 1) == -1){
        return -1;
    }
    /* add the file into directory */
    uint8_t inode_num = get_inodenum(node);
    if(add_todir(dirname, curdir, inode_num) == -1){
        return -1;
    }

    return 0;
}

int32_t delete_subdir(const char* dirname)
{
    /* find the entry */
    int8_t entry_idx = find_entry(curdir, dirname, 1);
    if(entry_idx == -1){
        perror("dirname not found");
        return -1;
    }
    struct dentry *dty = &curdir->entry[entry_idx];
    struct inode *node = get_inodeaddr(dty->inode_idx);

    /* release the inode */
    uint16_t inode_num = get_inodenum(node);
    release_inode(inode_num);

    /* release the entry */
    release_entry(curdir, entry_idx);

    return 0;
}

void init_rootdir()
{
    /* allocate inode */
    struct inode *root_node = allocate_inode();
    if(root_node == NULL){
        perror("initial root failed");
        return ;
    }

    root = (struct directory *)get_blockaddr(2);

    curdir = root;
    path[0] = '\\';
    path[1] = '\0';
}

