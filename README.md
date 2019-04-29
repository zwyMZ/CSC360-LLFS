# CSC360-LLFS
In the design of The Little Log File System(LLFS), file read and write, subdirectory creation and deletion, and addition and
deletion of files in subdirectories are implemented. At the same time, in order to improve the robustness of the file system and
prevent the data structure from being destroyed, use fflush & fsync to perform the atomic operation. The operation of the file
and write changes to the data results to disk.Finally, complete the test01, test02 test program, test the creation and mounting
of the vdisk, and various read and write operations on the file.

--------------------------------------------------------------
1. data structure
	In this program, the block usage is as follows
block[0-9] reserved blocks, no permit to store data
block 0 : super_block
block 1 : vector_block
block 2-5 : directory_block
block 6-7 : dentry_block, mapped relationship between inode and directory
block 8-9 : inode_block

block[10-4095] : data blocks
 
-------------------------------------------------------------
2. ensure that disk data is always up to date
	After each file system is mounted, the memory is always manipulated without changing the contents of the file system. However, 
after changing the file system, you need to update the disk in time. This design treats the operation of a file, such as creating,
reading, writing, deleting, etc., as an atomic operation.

	Either all is done, then the results of the changes are uniformly updated to disk, or the entire result is discarded. In this way,
it can be well avoided. When the computer is executing on an operation, it suddenly crashes, but some disk data has been updated.
This makes it easy to crash the file system as well.
Two macros named preper_write_to_disk() and end_write_to_disk() are implemented in test01.c to accomplish this function. It uses
fflush() & fsync() to force changed data to be written into the vdisk.

--------------------------------------------------------------
3. test program
/apps/test01.c
	Use create_file(), write_file(), read_file(), delete_file(), create_subdir(), delete_subdir() and Change_workdir() functions tests
the file system separately.

/apps/test02.c
	The function InitLLFS() completes the creation of the new vdisk, and the function MountLLFS() completes the mount of the vdisk.
InitLLFS() mainly performs the necessary initialization of the spuer_block, vector_block and creates a root inode. When MountLLFS()
is mounted, the file system's magic_num is checked to determine whether vdisk is supported. In addition, the root root directory 
will be set.
