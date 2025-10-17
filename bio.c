// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses two state flags internally:
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

struct
{
  struct spinlock lock; // 保护分配块缓存数据的锁
  struct buf buf[NBUF]; // 缓存块

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head;
} bcache;

void binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // PAGEBREAK!
  //  初始化链表
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
/**
 * 从缓冲区缓存中获取指定设备和块号的缓冲区
 * @param dev 设备号
 * @param blockno 块号
 * @return 指向缓冲区结构体的指针
 */
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // 查找请求的块是否已经在缓存中
  for (b = bcache.head.next; b != &bcache.head; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 块未被缓存，尝试回收一个未使用的缓冲区
  // 即使refcnt为0，如果B_DIRTY标志被设置，表示该缓冲区正在被日志系统使用，
  // 因为log.c已经修改了它但还没有提交
  for (b = bcache.head.prev; b != &bcache.head; b = b->prev)
  {
    if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
/*
 * 返回块的缓冲区，尝试从块缓存中获取该块的缓存项，如果该缓存项不存在，则创建一个并从磁盘读取块内容。
 * @param dev: 设备号
 * @param blockno: 块号
 * @return: 指向包含数据的缓冲区结构体指针
 */
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  // 获取指定设备和块号的缓冲区
  b = bget(dev, blockno);

  // 如果缓冲区数据无效，则从磁盘读取数据
  if ((b->flags & B_VALID) == 0)
  {
    iderw(b);
  }

  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  b->flags |= B_DIRTY;
  iderw(b);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }

  release(&bcache.lock);
}
// PAGEBREAK!
//  Blank page.
