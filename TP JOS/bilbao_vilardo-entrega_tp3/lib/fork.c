// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	int perms = PGOFF(uvpt[PGNUM(addr)]);
	if ((err & FEC_PR) != FEC_PR || (err & FEC_WR) != FEC_WR ||
	    (perms & PTE_COW) != PTE_COW)
		panic("Page fault!");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if (sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W) < 0)
		panic("Error al reservar una pagina");

	memmove(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
	if (sys_page_map(0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U | PTE_W))
		panic("Error al mapear memoria");

	if (sys_page_unmap(0, PFTEMP) < 0)
		panic("Error al liberar la memoria");
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
	// LAB 4: Your code here.
	int perms = PGOFF(uvpt[pn]) & (PTE_SYSCALL | PTE_COW);
	int cow_flag = (perms & PTE_W || perms & PTE_COW) ? PTE_COW : 0;
	perms &= ~PTE_W;

	if (sys_page_map(0,
	                 (void *) (pn * PGSIZE),
	                 envid,
	                 (void *) (pn * PGSIZE),
	                 perms | cow_flag) < 0)
		panic("Error al mapear memoria");

	if (sys_page_map(0,
	                 (void *) (pn * PGSIZE),
	                 0,
	                 (void *) (pn * PGSIZE),
	                 perms | cow_flag) < 0)
		panic("Error al mapear memoria propia");

	return 0;
}

static void
dup_or_share(envid_t dstenv, void *va, int perm)
{
	if (!(perm & PTE_W)) {
		if (sys_page_map(0, va, dstenv, va, perm) < 0)
			panic("Error compartir la memoria");
		return;
	}

	if (sys_page_alloc(dstenv, va, perm) < 0)
		panic("Error al copiar la direccion de memoria");
	if (sys_page_map(dstenv, va, 0, UTEMP, perm) < 0)
		panic("Error al mapear");
	memmove(UTEMP, va, PGSIZE);
	if (sys_page_unmap(0, UTEMP) < 0)
		panic("Error al desmapear");
}

envid_t
fork_v0(void)
{
	envid_t envid = sys_exofork();

	if (envid < 0)
		panic("Error en el fork");

	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (uint8_t *addr = 0; addr < (uint8_t *) UTOP; addr += PGSIZE) {
		if (!(uvpd[PDX(addr)] & PTE_P))
			continue;
		if (!(uvpt[PGNUM(addr)] & PTE_P))
			continue;

		dup_or_share(envid, addr, PGOFF(uvpt[PGNUM(addr)]) & PTE_SYSCALL);
	}

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
		panic("Error al marcar el proceso como RUNNABLE");

	return envid;
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
	set_pgfault_handler(pgfault);

	envid_t envid = sys_exofork();
	if (envid < 0)
		panic("Error al hacer el fork");

	if (envid == 0) {  // Hijo
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (size_t i = 0; i < USTACKTOP; i += PGSIZE) {
		if (!(PGOFF(uvpd[PDX(i)]) & PTE_P))
			continue;

		if (!(uvpt[PGNUM(i)] & PTE_P))
			continue;

		if (duppage(envid, PGNUM(i)) < 0)
			panic("Error al copiar la pagina");
	}

	// Crear el UXStack para el hijo.
	if (sys_page_alloc(envid, (void *) (UTOP - PGSIZE), PTE_W | PTE_U | PTE_P) <
	    0)
		panic("Error al reservar una pagina para el UXSTACK");

	if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall) < 0)
		panic("Error al establecer el upcall");

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
		panic("Error al cambiar el estado del hijo");

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
