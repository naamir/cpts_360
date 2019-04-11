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
            printf("dir_lblk:%i\n", lblk);
            pblk = fmip->INODE.i_block[lblk];      // blk should be a disk block now
        }
        else if (lblk >= 12 && lblk < 256 + 12) {
            // indirect blocks
            printf("indir_lblk:%i\n", lblk);
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
            printf("doubleindir_lblk:%i\n", lblk);
            i13 = fmip->INODE.i_block[13];
            printf("outsidei13[%i]\n", i13);
            if (i13 == 0) {
                fmip->INODE.i_block[13] = balloc(fmip->dev);
                i13 = fmip->INODE.i_block[13];
                printf("i13[%i]\n", i13);
                memset(zbuf, 0, BLKSIZE);       // zero out buffer
                put_block(fmip->dev, i13, zbuf);  // zero out block on disk
            }
            get_block(dev, i13, dindbuf1);            
            di_db1 = (int *)dindbuf1;
            lblk -= (256 + 12);
            di_nb1 = di_db1[lblk / 256];
            //printf("outsidedi_db1[%i]\n", lblk / 256);
            if (di_nb1 == 0) {
                //di_nb1 = balloc(fmip->dev);
                di_db1[lblk / 256] = balloc(fmip->dev);
                printf("di_db1[%i]\n", lblk / 256);
                //memset(zbuf, 0, BLKSIZE);       // zero out buffer
                //put_block(fmip->dev, di_nb1, zbuf);  // zero out block on disk
                //pblk = di_db1[lblk / 256];
                put_block(fmip->dev, i13, dindbuf1);
            }

            //di_db2 = (int *)di_db1[lblk/256];
            get_block(dev, di_db1[lblk / 256], dindbuf2);
            di_db2 = (int *)dindbuf2;
            pblk = di_db2[lblk % 256];
            if (pblk == 0) {
                pblk = balloc(fmip->dev);
                //memset(zbuf, 0, BLKSIZE);       // zero out buffer
                put_block(fmip->dev, di_db1[lblk / 256], dindbuf2);  // zero out block on disk
            }
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