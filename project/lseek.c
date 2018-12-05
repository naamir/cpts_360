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
int my_lseek (int fd, int position)
{
    OFT *oftp;
	oftp = malloc(sizeof(OFT));
    //From fd, find the OFT entry. 
    for (int j=0 ; j< NOFT; j++)
     {
        oftp = &ofTable[j];
        if (oftp->mptr->dev ==fd)
        {
           if (oftp->mode > 0)
           return -1;
         }
         
     }
  //change OFT entry's offset to position but make sure NOT to over run either end of the file.

 // return originalPosition
}