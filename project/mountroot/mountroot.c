/****************************************************************************
*                   mount_root Program                                      *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[256]; // holder of component strings in pathname
char *name[64];  // assume at most 64 components in pathnames
int  n;

int  fd, dev;
int  nblocks, ninodes, bmap, imap, iblk, inode_start;
char line[256], cmd[32], pathname[256];

MINODE *iget();

/******* WRITE YOUR OWN util.c and others ***********
#include "util.c"
#include "cd_ls_pwd.c"
***************************************************/
int init()
{
  int i;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
      mip = &minode[i];
      // set all entries to 0;
  }
  for (i=0; i<NPROC; i++){
       p = &proc[i];
      // set pid = i; uid = i; cwd = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  char buf[BLKSIZE];
  SUPER *sp;
  GD    *gp;

  printf("mount_root()\n");
/*
  (1). read super block in block #1;
       verify it is an EXT2 FS;
       record nblocks, ninodes as globals;

  (2). get GD0 in Block #2:
       record bmap, imap, inodes_start as globals

  (3). root = iget(dev, 2);       // get #2 INODE into minoe[ ]
       printf("mounted root OK\n");
       */
}

char *disk = "mydisk";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  if (argc > 1)
     disk = argv[1];

  fd = open(disk, O_RDWR);
  if (fd < 0){
     printf("open %s failed\n", disk);  
     exit(1);
  }
  dev = fd;

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  // set proc[1]'s cwd to root also
  printf("root refCount = %d\n", root->refCount);

  while(1){
    printf("input command : [ls|cd|pwd|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s", cmd, pathname);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if (strcmp(cmd, "ls")==0)
       ls(pathname);

    if (strcmp(cmd, "cd")==0)
       chdir(pathname);

    if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);

    if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int print(MINODE *mip)
{
  int blk;
  char buf[1024], temp[256];
  int i;
  DIR *dp;
  char *cp;

  INODE *ip = &mip->INODE;
  for (i=0; i < 12; i++){
    if (ip->i_block[i]==0)
      return 0;
    get_block(dev, ip->i_block[i], buf);

    dp = (DIR *)buf; 
    cp = buf;

    while(cp < buf+1024){

       // make dp->name a string in temp[ ]
       printf("%d %d %d %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

       cp += dp->rec_len;
       dp = (DIR *)cp;
    }
  }
}
 
int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}