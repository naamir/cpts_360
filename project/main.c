/****************************************************************************
*                   Main Program                                      *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

char *disk = "mydisk";

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;

char gpath[256]; // holder of component strings in pathname
char *name[64];	// assume at most 64 components in pathnames
char *myargs[30];                 // arguments
char argtemp[256];
int  myargc;                       // number of arguments

int fd, dev, n;
int nblocks, ninodes, bmap, imap, iblk;
char line[256], cmd[32], pathname[256];
char *cmds[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm","save", "reload", "menu", "quit", NULL};

#include "util.c"
#include "cd_ls_pwd.c"
#include "ialloc_balloc.c"
#include "mkdir.c"
#include "rmdir_nofal.c"
#include "creat.c"
#include "link_unlink_symlink.c"

int tokArguments(char *mystr)
{
	char *s;
	int i = 0;

	strcpy(argtemp, mystr);

	s = strtok(argtemp, " ");
	while(s) {
		myargs[i] = s;
		s = strtok(0, " ");
		i++;
	}
	return i;
}

int init()
{
	int i;
	MINODE *mip;
	PROC *p;

	printf("init()\n");

	for (i = 0; i < NMINODE; i++)
	{
		mip = &minode[i];
		// set all entries to 0;
		mip->refCount = 0;
		mip->dirty = 0;
		mip->mounted = 0;
		mip->mptr = 0;
	}
	for (i = 0; i < NPROC; i++)
	{
		p = &proc[i];
		p->pid = i;
		p->uid = i;
		p->cwd = 0;
	}
}

// load root INODE and set root pointer to it
int mount_root()
{
	char buf[BLKSIZE];
	//SUPER *sp;
	//GD *gp;
	//MINODE mip;

	printf("mount_root()\n");
	/*
	 (1). read super block in block #1;
	 verify it is an EXT2 FS;
	 record nblocks, ninodes as globals;
       */
	/* get super block of the rootdev */
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	/* check magic number */
	if (sp->s_magic != SUPER_MAGIC)
	{
		printf("super magic=%x : %s is not an EXT2 file system\n",
					 sp->s_magic, disk);
		exit(0);
	}
	nblocks = sp->s_blocks_count;
	ninodes = sp->s_inodes_count;
	/*
	 (2). get GD0 in Block #2:
	 record bmap, imap, inodes_start as globals
       */
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	bmap = gp->bg_block_bitmap;
	imap = gp->bg_inode_bitmap;
	iblk = gp->bg_inode_table;
	/*
	 (3). root = iget(dev, 2);       // get #2 INODE into minode[ ]
	 printf("mounted root OK\n");
	*/

	root = iget(dev, 2);
	printf("root=%x dev=%d, ino=%d\n", root, root->dev, root->ino);
	printf("root refCount = %d\n", root->refCount);

	printf("creating P0 as running process\n");
	
	proc[0].status = READY;
	proc[0].cwd = iget(dev, 2);
	running = &proc[0];
	// set proc[1]'s cwd to root also
	proc[1].cwd = iget(dev, 2);

	printf("mounted root OK\n");
}

void reset()
{
	int i;
	for (i=0; i<256; i++)
		pathname[i] = 0;

	for (i=0; i<64; i++)
		name[i] = 0;
	
	for (i=0; i<32; i++)
		cmd[i] = 0;
}

int findcmd(char *command)
{
  for(int i=0; cmd[i]!=NULL; i++)
    {
      if (strcmp(cmd[i], command) == 0)
	{
	  printf("%s\n", cmd[i]);
	  return i;
	}     
    }
  return -1;
}

int main(int argc, char *argv[])
{
	int ino;
	char buf[BLKSIZE];
	if (argc > 1)
		disk = argv[1];

	fd = open(disk, O_RDWR);
	if (fd < 0)
	{
		printf("open %s failed\n", disk);
		exit(1);
	}
	dev = fd;

	init();
	mount_root();
	printf("root refCount = %d\n", root->refCount);
	pimap();
	pbmap();

	while (1)
	{
		printf("input command : [ ls | cd | pwd | mkdir | rmdir | creat | link | unlink | quit] ");
		fgets(line, 128, stdin);
		line[strlen(line) - 1] = 0;

		myargc = tokArguments(line);
		if (myargc < 3){
			sscanf(line, "%s %s", cmd, pathname);
			printf("cmd=%s pathname=%s\n", cmd, pathname);
		}
		else {
			strcpy(cmd, myargs[0]);
		}

		if (strcmp(cmd, "ls") == 0)
			ls(pathname);

		if (strcmp(cmd, "cd") == 0)
			chdir(pathname);

		if (strcmp(cmd, "pwd") == 0)
			pwd(running->cwd);

		if (strcmp(cmd, "quit") == 0)
			quit();
		if (cmd[0] == 'q')
			quit();

		if (strcmp(cmd, "mkdir") == 0)
			make_dir(pathname);
		
		if (strcmp(cmd, "creat") == 0)
			creat_file(pathname);
		
		if (strcmp(cmd, "rmdir") == 0)
			remove_dir(pathname);
		
		if (strcmp(cmd, "link") == 0)
			mylink(myargs[1], myargs[2]);

		if (strcmp(cmd, "unlink") == 0)
			my_unlink(pathname);

		reset();
	}
}
