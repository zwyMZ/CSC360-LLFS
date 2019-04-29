#include <stdio.h>
#include <stdlib.h>
#include "file.h"

void InitLLFS();
int8_t MountLLFS();
void modify_vdisk();

int main()
{
    /* initialize a new vdisk */
    printf("create a new vdisk...\n");
    InitLLFS();

    /* mount a existed vdisk */
    printf("try to mount the new vdisk...\n");
    if(MountLLFS() == -1){
        printf("mount failed\n");
        exit(EXIT_FAILURE);
    }
    printf("mount success\n");

    return 0;
}

// block 0: super block
// block 1: block vector
// first ten blocks reserved
void InitLLFS()
{
    /* allocate space */
    system_addr = (uint8_t *)calloc(SYSTEM_SIZE, sizeof(char));  //2M

    /* set up the super block */
    struct block *blk = (struct block *)system_addr;
    struct super_block *sb = (struct super_block *)blk;
    sb->magic_num = 0x12345678;
    sb->total_blocks = NUM_BLOCKS;
    sb->total_inodes = 0;

    /* initialize the block vector */
    blk += 1;
    struct vector_block *vb = (struct vector_block *)blk;
    for(int ibit=0; ibit<10; ++ibit){
        uint32_t i = ibit / 8;
        uint32_t j = ibit % 8;
        vb->vec[i] |= (uint8_t)(1 << j);
    }

    init_rootdir();

    /* write into the vdisk file */
    FILE* vdisk = fopen("vdisk", "w+b");
    if(!vdisk){
        fatalsys("fopen failed");
    }
    fwrite(system_addr, SYSTEM_SIZE*sizeof(char), 1, vdisk);
    fclose(vdisk);

    free(system_addr);
}

int8_t MountLLFS()
{
    uint8_t ret=0;
    /* open the vdisk file */
    FILE* vdisk = fopen("vdisk", "r+b");
    if(!vdisk){
        fatalsys("fopen failed");
    }

    /* read the file including super-block and block vector */
    system_addr = (uint8_t *)calloc(SYSTEM_SIZE, sizeof(char));   //2M
    fread(system_addr, sizeof(struct block), 4096, vdisk);
    struct super_block *sb = (struct super_block *)system_addr;
    printf("check magic_num of the vdisk\n");
    if(sb->magic_num != 0x12345678){
        perror("unknown file system type");
        ret = -1;
    }

    /* initialize directory */
    printf("try to set up root directory\n");
    root = (struct directory *)get_blockaddr(2);

    curdir = root;
    path[0] = '\\';
    path[1] = '\0';

    fclose(vdisk);
    free(system_addr);

    return ret;
}


void modify_vdisk()
{
    /* open the vdisk file */
    FILE* vdisk = fopen("vdisk", "r+b");
    if(!vdisk){
        fatalsys("fopen failed");
    }

    /* read the file including super-block and block vector */
    system_addr = (uint8_t *)calloc(SYSTEM_SIZE, sizeof(char));   //2M
    fread(system_addr, sizeof(struct block), 4096, vdisk);
    struct super_block *sb = (struct super_block *)system_addr;
    sb->magic_num = 0x11223344;
    fclose(vdisk);

    vdisk = fopen("vdisk", "w+b");
    if(!vdisk){
        fatalsys("fopen failed");
    }
    fwrite(system_addr, SYSTEM_SIZE*sizeof(char), 1, vdisk);

    fclose(vdisk);
    free(system_addr);
}
