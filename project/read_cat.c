/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern OFT oftp[NFD];
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
    char *cp, *cq;
    unsigned int i12, i13, *i_dbl, *di_db1, *di_db2;
    char indbuf[BLKSIZE/4], dindbuf1[BLKSIZE/4], dindbuf2[BLKSIZE/4], readbuf[BLKSIZE];
    int pblk, lblk, start, remain, avail, id;
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
        // logical block will tell us which block our updated offset resides in
        // NOTE: offset is just which exact byte no. we want to read or start reading
        lblk = oftp->offset / BLKSIZE; // note: offset is 0 when new file
        start = oftp->offset % BLKSIZE;
        remain = BLKSIZE - start;

        if (lblk < 12){                     // lbk is a direct block
            pblk = fmip->INODE.i_block[lblk]; // map LOGICAL lbk to PHYSICAL blk
        }
        else if (lblk >= 12 && lblk < 256 + 12) { 
            // indirect blocks
            i12 = fmip->INODE.i_block[12];
            get_block(dev, i12, indbuf);
            i_dbl = (unsigned int *)indbuf;

            pblk = i_dbl[lblk];
        }
        else{ 
            // double indirect blocks
            i13 = fmip->INODE.i_block[13];
            get_block(dev, i13, dindbuf1);
            di_db1 = (unsigned int *)dindbuf1;

            for (id = 0; id < 256; id++) {
                get_block(dev, di_db1[id], dindbuf2);
                di_db2 = (unsigned int *)dindbuf2;

                pblk = di_db2[lblk];
            }
        }

        // get the data block into readbuf[BLKSIZE]
        get_block(fmip->dev, pblk, readbuf);

        cp = readbuf + start;  // start address to read in disk
        remain = BLKSIZE - start;   // number of bytes that     remain in readbuf[]

        while (remain > 0) {
            
        }
    }

}

int my_cat(char *pathname)
{
    char buf[BLKSIZE];
    int fd;

    fd = my_open(pathname, R);
    pfd();
    close_file(fd);
}