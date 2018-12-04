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

int fdalloc(PROC *proc)   // allocate a free inode
{
    int i;
    for (i = 0; i<NFD; i++){
        OFT *of = proc->fd[i];
        // refCount for all FDs are set to 0 in init()
        // they're set to 1 when opened
        if (of->refCount == 0){
            return i; // return the lowest available index
        }
    }
    printf("FS panic: out of FDs\n");
    return 0;
}
/*
int oftalloc(OFT *of)   // release a used open file table
{
    of->refCount = 0;  // just resetting the refCount is enough
                        // to reset as that memory location is not
                        // locked anymore
}
*/
int my_open(char *pathname, int mode)
{
    int ino, i;
    MINODE *mip;
    INODE *ip;
    OFT *oftp;

    if (pathname[0] == '/')  dev = root->dev;
    else                     dev = running->cwd->dev;

    ino = getino(pathname);
    if (ino == 0)  // if no file present then create it!
        {
            creat_file(pathname);
            ino = getino(pathname);
        }
    mip = iget(dev, ino); // get the file's MINODE

    ip = &mip->INODE;
    if (S_ISREG(ip->i_mode) && //check if it's a REG file
        ((ip->i_mode & S_IRWXU) == S_IRUSR) &&  // if File Owner has READ permissions
        ((ip->i_mode & S_IRWXG) == S_IRUSR))   { // if Group has READ permissions

        for (i = 0; i < NFD; i++) {
            // checking if the pathname minode can be found in the current list
            // of open fds in the running PROC - so we can check if file is already open
            if (mip == running->fd[i]->mptr) {
                printf("file already open\n");
                return 0;
            }
        }

        //oftp = oftalloc();
        oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
        oftp->refCount = 1;
        oftp->mptr = mip;  // point at the file's minode[]

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
        int ifd = fdalloc(running);
        running->fd[ifd] = oftp;  // Let running->fd[i] point at the OFT entry

        switch(mode){
            case 0: ip->i_atime = time(0L);
                    break;
            case 1: ip->i_atime = time(0L);
                    ip->i_mtime = time(0L);
                    break;
            case 2: ip->i_atime = time(0L);
                    ip->i_mtime = time(0L);
                    break;
            case 3: ip->i_atime = time(0L);
                    ip->i_mtime = time(0L);
                    break;
            default: printf("invalid mode\n");
                    return(-1);
        }
        mip->dirty = 1;
        iput(mip);

        return ifd; // return file descriptor
    }
}