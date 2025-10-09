// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

/**
 * @brief 初始化锁
 *
 * @param lk 待初始化的锁
 * @param name  锁名称
 */
void initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
/**
 * 获取自旋锁
 *
 * 该函数用于获取指定的自旋锁。如果锁已被占用，函数会自旋等待直到锁被释放。
 * 在获取锁的过程中，会禁用中断以避免死锁，并记录锁的持有信息用于调试。
 *
 * @param lk 指向要获取的自旋锁结构体的指针
 */
void acquire(struct spinlock *lk)
{
  pushcli(); // disable interrupts to avoid deadlock.
  if (holding(lk))
    panic("acquire");

  // 原子地尝试获取锁，如果锁已被占用则自旋等待
  while (xchg(&lk->locked, 1) != 0)
  {
    continue;
  }

  // 告诉C编译器和处理器不要将加载或存储操作移动到此点之后，
  // 以确保临界区的内存引用在获取锁之后发生
  __sync_synchronize();

  // 记录锁获取的调试信息
  lk->cpu = mycpu();
  getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
void release(struct spinlock *lk)
{
  if (!holding(lk))
    panic("release");

  lk->pcs[0] = 0;
  lk->cpu = 0;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m"(lk->locked) :);

  popcli();
}

// Record the current call stack in pcs[] by following the %ebp chain.
void getcallerpcs(void *v, uint pcs[])
{
  uint *ebp;
  int i;

  ebp = (uint *)v - 2;
  for (i = 0; i < 10; i++)
  {
    if (ebp == 0 || ebp < (uint *)KERNBASE || ebp == (uint *)0xffffffff)
      break;
    pcs[i] = ebp[1];      // saved %eip
    ebp = (uint *)ebp[0]; // saved %ebp
  }
  for (; i < 10; i++)
    pcs[i] = 0;
}

// Check whether this cpu is holding the lock.
int holding(struct spinlock *lock)
{
  int r;
  pushcli();
  r = lock->locked && lock->cpu == mycpu();
  popcli();
  return r;
}

// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void pushcli(void)
{
  int eflags;

  eflags = readeflags();
  cli();
  if (mycpu()->ncli == 0)
    mycpu()->intena = eflags & FL_IF; // 关中断前的中断状态，用于恢复最外层中断状态。
  mycpu()->ncli += 1;
}

void popcli(void)
{
  if (readeflags() & FL_IF)
    panic("popcli - interruptible");
  if (--mycpu()->ncli < 0)
    panic("popcli");
  // 最外层中断恢复，如果处于中断中，则恢复中断状态。
  if (mycpu()->ncli == 0 && mycpu()->intena)
    sti();
}
