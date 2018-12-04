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
extern char pathname[256];

int myread(int fd, char *buf, int nbytes)
{
    char *cq;
    unsigned int i12, i13, *i_dbl, *di_db1, *di_db2;
    char indbuf[BLKSIZE/4], dindbuf1[BLKSIZE/4], dindbuf2[BLKSIZE/4];
    int pblk, lblk, start, remain, avail;
    OFT *oftp;
    MINODE *fmip;

    cq = buf;
    avail = oftp->mptr->INODE.i_size - oftp->offset; // filesize - offset

    // assign the openned file in running Proc to the local variable oftp
    oftp = running->fd[fd];
    // get the file's MINODE ptr
    fmip = oftp->mptr;
    if (oftp->mode != R || oftp->mode != RW) {
        printf("file is not open\n");
        return 0;
    }

    // we want to read nbytes and we have avail remining
    // NOTE: nybytes should be greater than
    while (nbytes && avail) {
        lblk = oftp->offset / BLKSIZE; // note: offset is 0 when new file
        start = oftp->offset % BLKSIZE;
        remain = BLKSIZE - start;

        if (lblk < 12){                     // lbk is a direct block
           pblk = fmip->INODE.i_block[lblk]; // map LOGICAL lbk to PHYSICAL blk
        }
        else if (lblk >= 12 && lblk < 256 + 12) { 
                //  indirect blocks 
        }
        else{ 
                //  double indirect blocks
        } 
    }

}