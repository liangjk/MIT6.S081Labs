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


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  // struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  struct
  {
    struct spinlock lock;
    struct buf head;
  } bucket[NBUCKET];
} bcache;

void
binit(void)
{
  // struct buf *b;

  // initlock(&bcache.lock, "bcache");

  // // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  // initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUCKET; ++i)
  {
    initlock(&bcache.bucket[i].lock, "bcache.bucket");
    bcache.bucket[i].head.prev = &bcache.bucket[i].head;
    bcache.bucket[i].head.next = &bcache.bucket[i].head;
  }
  int id = 0;
  for (struct buf *b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.bucket[id].head.next;
    b->prev = &bcache.bucket[id].head;
    b->next->prev = b;
    b->prev->next = b;
    initsleeplock(&b->lock, "buffer");
  }
}

struct buf *findfreebuf(int eid)
{
  int i = eid + 1;
  if (i >= NBUCKET)
    i = 0;
  while (i != eid)
  {
    acquire(&bcache.bucket[i].lock);
    for (struct buf *b = bcache.bucket[i].head.prev; b != &bcache.bucket[i].head; b = b->prev)
    {
      if (b->refcnt == 0)
      {
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.bucket[i].lock);
        return b;
      }
    }
    release(&bcache.bucket[i].lock);
    ++i;
    if (i >= NBUCKET)
      i = 0;
  }
  return 0;
}

// Caller mush hold the lock
struct buf *findinbucket(uint dev, uint blockno)
{
  int hashid = blockno % NBUCKET;
  for (struct buf *b = bcache.bucket[hashid].head.next; b != &bcache.bucket[hashid].head; b = b->next)
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      return b;
    }
  for (struct buf *b = bcache.bucket[hashid].head.prev; b != &bcache.bucket[hashid].head; b = b->prev)
  {
    if (b->refcnt == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      return b;
    }
  }
  return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  // struct buf *b;

  // acquire(&bcache.lock);

  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // panic("bget: no buffers");

  int hashid = blockno % NBUCKET;
  acquire(&bcache.bucket[hashid].lock);
  struct buf *b = findinbucket(dev, blockno);
  release(&bcache.bucket[hashid].lock);
  if (b)
  {
    acquiresleep(&b->lock);
    return b;
  }
  b = findfreebuf(hashid);
  acquire(&bcache.bucket[hashid].lock);
  if (b)
  {
    b->next = &bcache.bucket[hashid].head;
    b->prev = bcache.bucket[hashid].head.prev;
    b->next->prev = b;
    b->prev->next = b;
  }
  b = findinbucket(dev, blockno);
  release(&bcache.bucket[hashid].lock);
  if (b)
  {
    acquiresleep(&b->lock);
    return b;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }

  // release(&bcache.lock);
  int hashid = b->blockno % NBUCKET;
  acquire(&bcache.bucket[hashid].lock);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bucket[hashid].head.next;
    b->prev = &bcache.bucket[hashid].head;
    b->next->prev = b;
    b->prev->next = b;
  }
  release(&bcache.bucket[hashid].lock);
}

void
bpin(struct buf *b) {
  // acquire(&bcache.lock);
  // b->refcnt++;
  // release(&bcache.lock);
  int hashid = b->blockno % NBUCKET;
  acquire(&bcache.bucket[hashid].lock);
  b->refcnt++;
  release(&bcache.bucket[hashid].lock);
}

void
bunpin(struct buf *b) {
  // acquire(&bcache.lock);
  // b->refcnt--;
  // release(&bcache.lock);
  int hashid = b->blockno % NBUCKET;
  acquire(&bcache.bucket[hashid].lock);
  b->refcnt--;
  release(&bcache.bucket[hashid].lock);
}


