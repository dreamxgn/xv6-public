// On-disk file system format.
// Both the kernel and user programs use this header file.

#define ROOTINO 1 // root i-number 根目录inode编号
#define BSIZE 512 // block size 数据块的大小

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock
{
  uint size;       // Size of file system image (blocks) 文件系统总的块数量
  uint nblocks;    // Number of data blocks 数据块的数量
  uint ninodes;    // Number of inodes. inodes数量
  uint nlog;       // Number of log blocks 日志块数量
  uint logstart;   // Block number of first log block 日志块的起始块号
  uint inodestart; // Block number of first inode block 第一个inode的起始块号
  uint bmapstart;  // Block number of first free map block 用于记录块空闲情况(已使用/未使用)的起始块号
};

#define NDIRECT 12 //直接块的数量
#define NINDIRECT (BSIZE / sizeof(uint)) // 间接块的数量
#define MAXFILE (NDIRECT + NINDIRECT) // 文件最多可以包含的数据块数量

// On-disk inode structure
struct dinode
{
  short type;              // File type 0表示空闲inode
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

//目录中每一项的结构(目录项)
struct dirent
{
  ushort inum; //目录项关联的inode编号
  char name[DIRSIZ]; //目录项的名称
};
