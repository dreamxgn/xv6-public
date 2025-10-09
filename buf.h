struct buf
{
  int flags;
  uint dev;              // 磁盘号
  uint blockno;          // 块号
  struct sleeplock lock; // 互斥锁
  uint refcnt;           // 被引用数量
  struct buf *prev;      // LRU cache list 最近最少使用
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[BSIZE]; // 缓存大小 512 bytes
};
#define B_VALID 0x2 // buffer has been read from disk
#define B_DIRTY 0x4 // buffer needs to be written to disk
