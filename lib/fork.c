// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
    pte_t *pte;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

    //jyhsu: my code
    if ((err & (FEC_PR | FEC_WR)) != (FEC_PR | FEC_WR)) {
        cprintf("[%08x] user pg fault va %08x ip %08x\n", thisenv->env_id, addr, utf->utf_eip);
        panic("actual pg fault while handling COW!");
    }
    if ((uvpt[PGNUM(addr)] & PTE_COW) != PTE_COW)
        panic("write access RO pg while handling COW!");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

    //jyhsu: my code
	//panic("pgfault not implemented");
    addr = (void *)PTE_ADDR(addr);
    if (sys_page_alloc(thisenv->env_id, (void *)PFTEMP, PTE_W | PTE_U) < 0)
        panic("new page alloc fail while handling COW!");
    memcpy((void *)PFTEMP, addr, PGSIZE);
    if (sys_page_unmap(thisenv->env_id, addr) < 0)
        panic("unmap old page failed while handling COW!");
    if (sys_page_map(thisenv->env_id, (void *)PFTEMP, thisenv->env_id, addr, PTE_W | PTE_U) < 0)
        panic("map new page failed while handling COW!");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.

    //jyhsu: my code
	//panic("duppage not implemented");
    int perm = PTE_U;

    if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW))
        perm |= PTE_COW;

    if (sys_page_map(thisenv->env_id, (void *)(pn*PGSIZE), envid, (void *)(pn*PGSIZE), perm) < 0)
        panic("dupe page failed while handling COW!");
    if (sys_page_map(thisenv->env_id, (void *)(pn*PGSIZE), thisenv->env_id, (void *)(pn*PGSIZE), perm) < 0)
        panic("remap page with perm COW failed while handling COW!");

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.

    //jyhsu: my code
	//panic("fork not implemented");
    envid_t parent, child;
    uintptr_t pg_thisenv = (uintptr_t) PTE_ADDR(&thisenv);
    unsigned i;
    int err = 0;

    set_pgfault_handler(pgfault);
    if ((child = sys_exofork()) == 0)
        return 0;
    parent = thisenv->env_id;

    if (sys_env_set_pgfault_upcall(child, envs[ENVX(parent)].env_pgfault_upcall) < 0)
        panic("set child pg fault upcall failed while forking!");

    for (i = 0; i < UTOP; i += PGSIZE) {
        if (i == UXSTACKTOP-PGSIZE) {
            if (sys_page_alloc(child, (void *)i, PTE_W | PTE_U) < 0)
                panic("child Xstack alloc failed while forking!");
        }
        else if (i == pg_thisenv) {
            if (sys_page_alloc(parent, (void *)PFTEMP, PTE_W | PTE_U) < 0)
                panic("page alloc failed while modifying child's thisenv!");

            thisenv = &envs[ENVX(child)];
            memcpy((void *)PFTEMP, (void *)pg_thisenv, PGSIZE);
            thisenv = &envs[ENVX(parent)];

            if ((err = sys_page_map(parent, (void *)PFTEMP, child, (void *)pg_thisenv, PTE_W | PTE_U)) < 0)
                panic("page map failed while modifying child's thisenv!");
        }
        else {
            if ((uvpd[PDX(i)] & PTE_P) == PTE_P) {
                if ((uvpt[PGNUM(i)] & PTE_P) == PTE_P) {
                    duppage(child, PGNUM(i));
                }
            }
        }
    }

    sys_env_set_status(child, ENV_RUNNABLE);
    return child;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
