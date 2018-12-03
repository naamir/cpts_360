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

int my_chmod(char *mode, char *pathname)
{
	int ino;
	MINODE *mip; //mip will point to the child Inode
	short perm = 0;

	ino = getino(pathname);
	mip = iget(dev, ino);
	
	printf("\nprevious permissions: %o\n", mip->INODE.i_mode);
	
		sscanf(mode, "%o", &perm);

	
  /*  long int new_mode = 0;
	new_mode = perm[0]-48 << 6;
	new_mode |= perm[1]-48 << 3;
	new_mode |= perm[2]-48;
*/
    // Change the file's mode to new_mode 
    // The leading 4 bits of type indicate type
    // First clear its current mode by ANDing with 0xF000
    // Then set its mode by ORing with new_mode 
	mip->INODE.i_mode = (mip->INODE.i_mode & 0xF000) | perm;
	
	printf("new permissions: %o\n", mip->INODE.i_mode);

return;

}