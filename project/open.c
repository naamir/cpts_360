/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[256], cmd[32], pathname[256];

/*************************FUNCTIONS*******************************/

/*
   
int open_file()
{
  1. ask for a pathname and mode to open:
         You may use mode = 0|1|2|3 for R|W|RW|APPEND

  2. get pathname's inumber:
         if (pathname[0]=='/') dev = root->dev;          // root INODE's dev
         else                  dev = running->cwd->dev;  
         ino = getino(pathname); 

  3. get its Minode pointer
         mip = iget(dev, ino);  

  4. check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
      
     Check whether the file is ALREADY opened with INCOMPATIBLE mode:
           If it's already opened for W, RW, APPEND : reject.
           (that is, only multiple R are OK)

  5. allocate a FREE OpenFileTable (OFT) and fill in values:
 
         oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
         oftp->refCount = 1;
         oftp->minodePtr = mip;  // point at the file's minode[]

  6. Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:

      switch(mode){
         case 0 : oftp->offset = 0;     // R: offset = 0
                  break;
         case 1 : truncate(mip);        // W: truncate file to 0 size
                  oftp->offset = 0;
                  break;
         case 2 : oftp->offset = 0;     // RW: do NOT truncate file
                  break;
         case 3 : oftp->offset =  mip->INODE.i_size;  // APPEND mode
                  break;
         default: printf("invalid mode\n");
                  return(-1);
      }

   7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
      Let running->fd[i] point at the OFT entry

   8. update INODE's time field
         for R: touch atime. 
         for W|RW|APPEND mode : touch atime and mtime
      mark Minode[ ] dirty

   9. return i as the file descriptor
}


int truncate(MINODE *mip)
{
  1. release mip->INODE's data blocks;
     a file may have 12 direct blocks, 256 indirect blocks and 256*256
     double indirect data blocks. release them all.
  2. update INODE's time field

  3. set INODE's size to 0 and mark Minode[ ] dirty
}
 */
int open_file(char path[124], char *third)
{
	int i, ino, fd = 0, perm = 0, mask = 0, mode = -1, offset = 0;
	MINODE *mip;
	INODE* ip;
	OFT* of = NULL;

	char buf[1024];

	printf("Opening file!\n");
	//Checks for the open mode and sets it to the correct int
	if (!strcmp(third, ""))
	{
		printf("No open mode specified!\n");
		return;
	}
	if (!strcmp(third, "0")) //R
		mode = 0;
	else if (!strcmp(third, "1")) //W
		mode = 1;
	else if (!strcmp(third, "2")) //RW
		mode = 2;
	else if (!strcmp(third, "3")) //APPEND
		mode = 3;
	else
	{
		printf("Invalid mode!\n");
		return;
	}

	//Checks the path and gets the ino for the pathname of what we want to open
	if (path[0] == '/')
		ino = getino(path);
	else  //get the ino
		ino = getino(path);

	//Ensures the file exists
	printf("%d\n", ino);
	if (ino == 0)
	{
		printf("ERROR: No such file!\n");
		return;
	}

	//iget on the ino and ensure it is a file
	mip = iget(dev, ino);
	ip = &mip->INODE;

	//ip points at the direct inode that is the file, we can now check its mode against a macro
	if (!S_ISREG(ip->i_mode))
	{
		printf("ERROR: Not a file!\n");
		iput(mip);
		return;
	}


	//checking permissions, checking the proc permissions against inodes perms
	if (running->uid == ip->i_uid)
	{
		//Correct user ownership
		perm = 6;
	}
	else if (running->gid == ip->i_gid)
	{
		//Correct group ownership
		perm = 3;
	}

	//6 = 110
	//calculates the mask based on the granted permissons for analyzing the mode
	//permission, 7 is shifted by 6 bits to left to yield '111000000' then anded with the mode
	//mask result of this operation is 0
	mask = mode & (7 << perm);
	iput(mip);

	//Checks the created mask to the permissions of the file
	//This ensures that the running process has the permissions to read, write, or execute the file
	//Returns if the permissions are not met, and therefore will not open the file
	//Mode is the mode that the user would like to open the file in
	switch(mode)
	{
		//4 = 100
		//when your permissions are not high enough to shift the value of the mask,
		case 1: //if the mask value = zero on the file and user is not super then fail
			if (((4 << perm) & mask) == 0 && running->uid != SUPER_USER)
			{
				printf("PERMISSION DENY!\n");
				iput(mip);
				return;
			}
			break;
		case 2:
		//2 = 010, if your permissions shift off and are flipped by your mask to equal 0 then no permission
			if (((2 << perm) & mask) == 0 && running->uid != SUPER_USER)
			{
				printf("PERMISSION DENY!\n");
				iput(mip);
				return;
			}
			break;
		case 3:
		//6 = 110
			if (((6 << perm) & mask) == 0 && running->uid != SUPER_USER)
			{
				printf("PERMISSION DENY!\n");
				iput(mip);
				return;
			}
			break;
		case 4:
		//2 = 010
		if (((2 << perm) & mask) == 0 && running->uid != SUPER_USER)
		{
			printf("PERMISSION DENY!\n");
			iput(mip);
			return;
		}
		offset = ip->i_size;
		break;
	}

	//finds an open file descriptor to use to open the file
	//Check the fd's in runnning to ensure we have an open fd for the file
	//If we do not have any useable fd's returns, and does not open the file
	for (fd = 0; fd < NFD; fd++)
	{
		if (running->fd[fd] == NULL)
			break;

		if (fd == NFD -1)
		{
			printf("Out of processes!\n");
			iput(mip);
			return;
		}
	}

	//Runs trough the OpenFileTable
	for (i = 0; i < NOFT; i++)
	{
		//this sets the open file table pointer to the selected open file table
		//we are now looking to see if its already open, ref count > 1 and if its busy
		of = &OpenFileTable[i];

		//Checking to make sure it is not open already
		//If it is open, making sure it is open in read mode
		//Multiple reads is OK
		if(mode != 0 && of->refCount > 0 && of->mptr == mip && of->mode != 0)
		{
			printf("Cannot open, file is being used!\n");
			iput(mip);
			return;
		}

		//Checks to make sure the fd is not busy
		//Set some varibales and brake cause we have our fd
		if (of->refCount == 0)
		{
			of->mode = mode;
			of->offset = offset;
			of->mptr = mip;
			of->refCount++;
			//the selected running fd now is equal to the of we have selected and modified
			running->fd[fd] = of;

			break;
		}

		//Breaks and returns if we do not have any room
		//Does not open the file
		if (i == NOFT - 1)
		{
			printf("No more room!\n");
			iput(mip);
			return;
		}
	}

	//Set the time and dirty
	ip->i_atime = time(0L);
	mip->dirty = 1;

	return fd;
}

//NOFT is number of open file table structs
//NFD is number of File Descriptors