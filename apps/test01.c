#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "file.h"

int8_t MountLLFS();

#define preper_write_to_disk()              \
do{                                         \
    vdisk = fopen("vdisk", "w+b");          \
    if(!vdisk){                             \
        fatalsys("fopen failed");           \
    }                                       \
}while(0)

#define end_write_to_disk()                 \
do{                                         \
    fwrite(system_addr, SYSTEM_SIZE*sizeof(char), 1, vdisk);    \
    fflush(vdisk);                          \
    fsync(fileno(vdisk));                   \
    fclose(vdisk);                          \
}while(0)

int main()
{
    FILE* vdisk = NULL;

    /* mount a existed vdisk */
    printf("try to mount the new vdisk...\n");
    if(MountLLFS() == -1){
        printf("mount failed\n");
        exit(EXIT_FAILURE);
    }
    printf("mount success\n");
    /* file operation */

    printf("[1] create a empty file\n");
    preper_write_to_disk();
    create_file("Micheal", 1);
    end_write_to_disk();

    printf("[2] write a string to the file\n");
    preper_write_to_disk();
    write_file("Micheal", "hello,world!");
    end_write_to_disk();

    printf("[3] read the file\n");
    preper_write_to_disk();
    read_file("Micheal", 12);
    end_write_to_disk();
    
    printf("[4] delete the file\n");
    preper_write_to_disk();
    delete_file("Micheal");
    end_write_to_disk();

    printf("[5] create a sub-directory: sub_dir1\n");
    preper_write_to_disk();
    create_subdir("sub_dir1");
    end_write_to_disk();

    printf("[6] create & write a file in the sub-directory\n");
    preper_write_to_disk();
    change_workdir("sub_dir1");
    create_file("text", 1);
    write_file("text", "i am a file in a sub-dir!");
    end_write_to_disk();

    printf("[7] read & delete in the file sub-directory\n");
    preper_write_to_disk();
    read_file("text", 26);
    delete_file("text");
    end_write_to_disk();

    printf("[8] delete the sub-directory: sub_dir1\n");
    preper_write_to_disk();
    delete_subdir("sub_dir1");
    end_write_to_disk();


    free(system_addr);
    return 0;
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

    return ret;
}
