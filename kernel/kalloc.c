// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern uint64 cas(volatile void *addr, int expected, int newval); //TODO:new change

#define NUM_PYS_PAGES ((PHYSTOP-KERNBASE) / PGSIZE) //TODO:new change
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct {
    struct spinlock lock;
    struct run *freelist;
    uint64 ref_count[NUM_PYS_PAGES]; //TODO:new change
} kmem;


struct run {
    struct run *next;
};


void
kinit()
{
    initlock(&kmem.lock, "kmem");
    memset(kmem.ref_count, 0, sizeof(kmem.ref_count)); //TODO:new change
    freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
        kmem.ref_count[(((uint64)p-KERNBASE) / PGSIZE)] = 1; //TODO:new change
        kfree(p);
    }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  int b=-1;
    struct run *r;

    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    changeCount((uint64)pa,b); //TODO:new change
    if (get_reference_count((uint64)pa) > 0) //TODO:new change
        return; //TODO:new change

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  int a =1;
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r) 
        kmem.freelist = r->next;
    release(&kmem.lock);

    if(r)
        memset((char*)r, 5, PGSIZE); // fill with junk

    if(r)
        changeCount((uint64)r,a); //TODO:new change

    return (void*)r;
}
/*
In order to avoid memory leaks, you should maintain a reference count per each pyhsical
page (which was used in COW), and when the refrence drop to 0, make sure you free the
page.

*/

void changeCount(uint64 pa,int numer){ //TODO:new change
    uint64 entry = (pa-KERNBASE) / PGSIZE;
    while(cas(kmem.ref_count + entry, kmem.ref_count[entry], kmem.ref_count[entry] + numer));
}

int get_reference_count(uint64 pa){ //TODO:new change
    return kmem.ref_count[(pa-KERNBASE) / PGSIZE];
}