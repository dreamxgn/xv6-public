// On-disk file system format.
// Both the kernel and user programs use this header file.

#define ROOTINO 1 // root i-number
#define BSIZE 512 // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock
{
  uint size;       // Size of file system image (blocks) 文件系统总的blocks数量
  uint nblocks;    // Number of data blocks 数据块的数量
  uint ninodes;    // Number of inodes. inodes数量
  uint nlog;       // Number of log blocks 存储事务日志块的数量
  uint logstart;   // Block number of first log block 日志块的起始块号
  uint inodestart; // Block number of first inode block inode块的起始块号
  uint bmapstart;  // Block number of first free map block 存储空闲块的起始块号
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode
{
  short type;              // File type
  short major;             // Major device number (T_DEV only)
  short minor;             // Minor device number (T_DEV only)
  short nlink;             // Number of links to inode in file system
  uint size;               // Size of file (bytes)
  uint addrs[NDIRECT + 1]; // Data block addresses
};

// 每个块有可以有多少个 inode Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB (BSIZE * 8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b / BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent
{
  ushort inum;
  char name[DIRSIZ];
};
