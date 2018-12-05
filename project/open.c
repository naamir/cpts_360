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
extern OFT ofTable[NOFT];
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[256], cmd[32], pathname[256];

/*************************FUNCTIONS*******************************/


   
int pfd()
{
//   This function displays the currently opened files as follows:

//         fd     mode    offset    INODE
//        ----    ----    ------   --------
//          0     READ    1234   [dev, ino]  
//          1     WRITE      0   [dev, ino]
//       --------------------------------------
//   to help the user know what files has been opened.
}

int truncate(MINODE *mip)
{
    /* 1. release mip->INODE's data blocks;
     a file may have 12 direct blocks, 256 indirect blocks and 256*256
     double indirect data blocks. release them all. */

   //assume 15 data blocks
	//release 12 direct blocks
	for (int i = 0; i < 15; i++)
	{
		if (mip->INODE.i_block[i] == 0)
		{
		continue;
		}
		bdalloc(mip->dev, mip->INODE.i_block[i]);
	}
	//indirect release
	//there are 256 indirect blocks
	if(mip->INODE.i_block[12] != 0)
	{
		int ind[256];
		get_block(mip->dev, mip->INODE.i_block[12], ind);
		for(int i = 0; i < 256; i++)
		{
			if(ind[i] == 0)
				continue;
			bdalloc(mip->dev, ind[i]);
		}
	}
	//double indirect release
	//there are 256*256 double indirect blocks
	if(mip->INODE.i_block[13] != 0)
	{
		int ind[256];
		int doubleInd[256];
		get_block(mip->dev, mip->INODE.i_block[13], ind);
		for(int i = 0; i < 256; i++)
		{
			if(ind[i] != 0)
			{
				get_block(mip->dev, ind[i], doubleInd);
				for(int j = 0; j < 256; j++)
				{
					if(doubleInd[j] == 0)
						continue;
					bdalloc(mip->dev, doubleInd[j]);
				}
				bdalloc(mip->dev, ind[i]);
			}
		}
	}
	INODE *ip = &mip->INODE;
   //  2. update INODE's time field
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);   // set to current time
	mip->INODE.i_size = 0;
   //  3. set INODE's size to 0 and mark Minode[ ] dirty

	mip->dirty = 1;
   return 1;


}

int open_file(char path[124], char *mode_str
)
{
	int ino, mode, file;
	MINODE *mip;
   INODE *ip;
	OFT *oftp;
	oftp = malloc(sizeof(OFT));
	int input[256];
	ino = 0;
	mode = 0;
   file = 0;

	printf("Opening file!\n");
	
   /*1. ask for a pathname and mode to open:
         You may use mode = 0|1|2|3 for R|W|RW|APPEND*/
   //Checks for the open mode and sets it to the correct int
	if (!strcmp(mode_str , ""))
	{
		printf("No open mode specified!\n");
		return;
	}
	if (!strcmp(mode_str, "0")) //R
		mode = 0;
	else if (!strcmp(mode_str, "1")) //W
		mode = 1;
	else if (!strcmp(mode_str, "2")) //RW
		mode = 2;
	else if (!strcmp(mode_str, "3")) //APPEND
		mode = 3;
	else
	{
		printf("Invalid mode!\n");
		return;
	}

   /*
     2. get pathname's inumber:
         if (pathname[0]=='/') dev = root->dev;          // root INODE's dev
         else                  dev = running->cwd->dev;  
         ino = getino(pathname); 

 */  

	if (path[0] == '/')
      {
		//user is choosing to open from the root
		dev = root->dev; //set root
	}
	else
	{
		//user is opening using the current working directory
		dev = running->cwd->dev; //set cwd
   }
   ino = getino(path);


	if (ino == 0)
	{
		printf("Creating a new file!\n");
    
      creat_file(path);
      ino = getino(path);

	}

	//iget on the ino and ensure it is a file
   /*
    3. get its Minode pointer
         mip = iget(dev, ino);*/
   printf("%d\n", ino);
	mip = iget(dev, ino);
	ip = &mip->INODE;

   /*
    4. check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
      
     Check whether the file is ALREADY opened with INCOMPATIBLE mode:
           If it's already opened for W, RW, APPEND : reject.
           (that is, only multiple R are OK)*/
	//ip points at the direct inode that is the file, we can now check its mode against a macro
	if (ip->i_mode != FILE_MODE)
	{
		printf("ERROR: Not a file!\n");
		iput(mip);
		return;
	}

   for (int j=0 ; j< NOFT; j++)
     {
        oftp = &ofTable[j];
        if (oftp->mptr == mip)
        {
           if (oftp->mode > 0)
           return -1;
         }
         
     }
   /*
   5. allocate a FREE OpenFileTable (OFT) and fill in values:
 
         oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
         oftp->refCount = 1;
         oftp->minodePtr = mip;  // point at the file's minode[] */
   
	      oftp->mode = mode;      
         oftp->refCount = 1;
         oftp->mptr = mip;  
   /*
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
      }*/
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
      
  /* 7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
      Let running->fd[i] point at the OFT entry
      */

     for (fd =0; fd <NFD; fd++)
     {
        if (running->fd[fd] == NULL)
        break;
       
     }
     running->fd[fd]= oftp;
      /*
   8. update INODE's time field
         for R: touch atime. 
         for W|RW|APPEND mode : touch atime and mtime
      mark Minode[ ] dirty */
      if (mode == 0)
         ip->i_atime = time(0L);
      else
      {
         ip->i_atime = time(0L);
         ip->i_mtime = time(0L);
      }
   
   mip->dirty = 1;
   /*9. return i as the file descriptor
   */
  return fd;
}

//NOFT is number of open file table structs
//NFD is number of File Descriptors