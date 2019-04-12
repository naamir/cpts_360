/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern OFT oftp[NFD];
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char pathname[256];

int mywrite(int fd, char *buf, int nbytes) 
{
    char *cp, *cq, zbuf[BLKSIZE];
    int i12, i13, *i_dbl, *di_db1, *di_db2, di_nb1, di_nb2;
    char indbuf[BLKSIZE], dindbuf1[BLKSIZE], dindbuf2[BLKSIZE], wbuf[BLKSIZE];
    int pblk, lblk, start, remain, id, count = 0; //avail
    OFT *oftp;
    MINODE *fmip;

    memset(wbuf, 0,BLKSIZE);
    cq = buf;

    // assign the openned file in running Proc to the local ptr oftp
    oftp = running->fd[fd];
    //avail = oftp->mptr->INODE.i_size - oftp->offset; // filesize - offset
    
    // get the file's MINODE ptr
    fmip = oftp->mptr;
    if (oftp->mode != W && oftp->mode != RW) {
        printf("do not have write permissions\n");
        return 0;
    }

    while (nbytes) {
        lblk = oftp->offset / BLKSIZE; // note: offset is 0 when new file
        start = oftp->offset % BLKSIZE;

        if (lblk < 12){                     // lbk is a direct block
            if (fmip->INODE.i_block[lblk] == 0)  // if no data block yet
                fmip->INODE.i_block[lblk] = balloc(fmip->dev);// MUST ALLOCATE a block
            //printf("dir_lblk:%i\n", lblk);
            pblk = fmip->INODE.i_block[lblk];      // blk should be a disk block now
        }
        else if (lblk >= 12 && lblk < 256 + 12) {
            // indirect blocks
            //printf("indir_lblk:%i\n", lblk);
            i12 = fmip->INODE.i_block[12];
            if (i12 == 0) {
                i12 = fmip->INODE.i_block[12] = balloc(fmip->dev);
                memset(zbuf, 0, BLKSIZE);       // zero out buffer
                put_block(fmip->dev, i12, zbuf);  // zero out block on disk
            }
            get_block(dev, i12, indbuf);
            i_dbl = (int *)indbuf;
            //pblk = i_dbl[lblk];
            
            pblk = i_dbl[lblk - 12];
            if (pblk == 0) { // if no data block yet
                //printf("i_dbl[%i]\n", lblk - 12); -->WORKING
                i_dbl[lblk - 12] = balloc(fmip->dev);
                pblk = i_dbl[lblk - 12];
                put_block(fmip->dev, i12, indbuf);
            }
        }
        else {
            // double indirect blocks
            //printf("doubleindir_lblk:%i\n", lblk);
            i13 = fmip->INODE.i_block[13];
            //printf("outsidei13[%i]\n", i13);
            if (i13 == 0) {
                fmip->INODE.i_block[13] = balloc(fmip->dev);
                i13 = fmip->INODE.i_block[13];
                //printf("i13val:%i\n", i13);
                memset(zbuf, 0, BLKSIZE);       // zero out buffer
                put_block(fmip->dev, i13, zbuf);  // zero out block on disk
            }
            get_block(dev, i13, dindbuf1);            
            di_db1 = (int *)dindbuf1;
            lblk -= (256 + 12);
            //di_nb1 = di_db1[lblk / 256];
            //printf("di_db1[lblk / 256][%i]=%i\n", lblk / 256, di_db1[lblk / 256]);
            if (di_db1[lblk / 256] == 0) {
                di_db1[lblk / 256] = balloc(fmip->dev);
                //printf("di_db1[%i], bvalue:%i\n", lblk / 256, di_db1[lblk / 256]);
                put_block(fmip->dev, i13, dindbuf1);  // record changes in block i13
            }

            get_block(dev, di_db1[lblk / 256], dindbuf2);
            di_db2 = (int *)dindbuf2;
            //pblk = di_db2[lblk % 256];
            if (di_db2[lblk % 256] == 0) {
                di_db2[lblk % 256] = balloc(fmip->dev);
                pblk = di_db2[lblk % 256];
                //printf("di_db2[%i], pblk:%i\n", lblk % 256, pblk);
                put_block(fmip->dev, di_db1[lblk / 256], dindbuf2);  // record changes in block di_db2
            }
            //getchar();
        }

        // get the data block into wbuf[BLKSIZE]
        get_block(fmip->dev, pblk, wbuf);
        cp = wbuf + start;  // start address to write in disk
        remain = BLKSIZE - start;   // number of bytes that remain in readbuf[]
        //printf("nbytes:%i, remain:%i\n", nbytes, remain);
        while (remain > 0 && nbytes > 0) {
            if (remain <= nbytes) {
                memcpy(cp, cq, remain);
                oftp->offset += remain;
                count += remain;
                nbytes -= remain;
                if (oftp->offset > fmip->INODE.i_size) 
                    fmip->INODE.i_size += remain;
                remain -= remain;
            }
            else { // if nbytes are greater than available bytes then read only as much are in the current block
                memcpy(cp, cq, nbytes);
                oftp->offset += nbytes;
                count += nbytes;
                remain -= nbytes;
                if (oftp->offset > fmip->INODE.i_size)
                    fmip->INODE.i_size += nbytes;
                nbytes -= nbytes;
            }
            
            //printf("nbytes:%i\n", nbytes);
            if (nbytes <= 0) // this condition will also break the outter loop
                break;
        }
        //printf("wbuf:%s\n", wbuf);
        put_block(fmip->dev, pblk, wbuf);
        //getchar();
    }
    fmip->dirty = 1;
    printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    return nbytes;
}

int testBlockval(char *f, char *blk)
{
    int fd, block, *di_db1, *di_db2, pblk;
    char dindbuf1[BLKSIZE], dindbuf2[BLKSIZE];
    OFT *oftp;
    MINODE *fmip;
    
    block = atoi(blk);
    fd = my_open(f, R);
    // assign the openned file in running Proc to the local ptr oftp
    oftp = running->fd[fd];
    // get the file's MINODE ptr
    fmip = oftp->mptr;
    printf("block %i: %i\n", block, fmip->INODE.i_block[block]);

    if (block == 13) {
        printf("double indirect blocks check\n");
        printf("got block 13:%i\n", fmip->INODE.i_block[block]);
        get_block(fmip->dev, fmip->INODE.i_block[block], dindbuf1);
        di_db1 = (int *)dindbuf1; // this is the array
        printf("got block di_db1[0]:%i\n", di_db1[0]);
        get_block(fmip->dev, di_db1[0], dindbuf2);
        di_db2 = (int *)dindbuf2;
        pblk = di_db2[0];
        printf("our 1st pblk should be:%i\n", pblk);
    }
    pfd();
    my_close(fd);
}

int my_cp(char *oldf, char *newf)
{
    int fd, gd, n;
    char buf[BLKSIZE];

    fd = my_open(oldf, R);
    gd = my_open(newf, RW);

    while( n=myread(fd, buf, BLKSIZE) ) {
       mywrite(gd, buf, n);  // notice the n in write()
    }
    pfd();
    my_close(fd); 
    my_close(gd);
}