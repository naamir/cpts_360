/*************** type.h file ************************/
#ifndef TYPE_HEADER
#define TYPE_HEADER

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE    64
#define NFD        16
#define NMOUNT      4
#define NPROC       2

#define EXT2_NAME_LEN  255

#define DIR_MODE    0x41ED
#define FILE_MODE   0x81AE
#define SUPER_MAGIC 0xEF53

typedef struct minode{
  INODE          INODE;
  int            dev, ino;
  int            refCount;
  int            dirty;
  int            mounted;
  struct mntable *mptr;
}MINODE;

typedef struct oft{
  int          mode;
  int          refCount;
  MINODE       *mptr;
  int          offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          ppid;
  int          status;
  int          uid, gid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

typedef struct mtable{
  int       dev;            // device number; 0 for FREE
  int       ninodes;        // from superblock
  int       nblocks;
  int       free_blocks;    // from superblock and GD
  int       free_inodes;
  int       bmap;           // from group descriptor
  int       imap;
  int       iblock;         // inodes start block
  MINODE    *mntDirPtr;     // mount point DIR pointer
  char      devName[64];    // device name
  char      mntName[64];    // mount point DIR name
}MTABLE;

//extern print(MINODE *mip);

#endif