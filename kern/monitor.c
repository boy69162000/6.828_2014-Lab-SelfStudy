// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Display information of all function call frames by backtracing", mon_backtrace},
    { "showmappings", "Display certain address and page mappings", mon_showmappings },
    { "changeperm", "Change permission of an address", mon_changeperm },
    { "memdump", "Display memory contents for a range", mon_memdump },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

int heximal_value(char *s, uint32_t *v)
{
    //jyhsu: heximal number string to heximal value
    //       return value: 0 if successful.
    //       -1=string too short
    //       -2=not leading with '0x'
    //       -3=effective digit > 8 (0xffffffff)
    //       -4=non-heximal character is found

    static char zero[] = "0";
    static char hexmrk[] = "0x";
    int i, len = strlen(s);
    uint32_t prev_v= 0;
    *v = 0;

    if (len < 3)
        return -1;
    if (strncmp(s, hexmrk, 2))
        return -2;

    for (i = 2; i < len; i++) {
        prev_v = *v;
        *v *= 0x10;

        if(prev_v > *v)   //overflow
            return -3;

        if (s[i] >= '0' && s[i] <= '9')
            *v += s[i]-'0';
        else if (s[i] >= 'a' && s[i] <= 'f')
            *v += s[i]-'a'+0xa;
        else
            return -4;
    }

    return 0;
}

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
    //jyhsu: my code
    uint32_t *ebp;
    struct Eipdebuginfo info;

    ebp = (uint32_t *)read_ebp();

    for (; ebp != 0; ebp = (uint32_t *)*ebp) {
        debuginfo_eip((uintptr_t) *(ebp+1), &info);
        cprintf("ebp %08x eip %08x args %08x %08x %08x %08x %08x\n", ebp, *(ebp+1), *(ebp+2), *(ebp+3), *(ebp+4), *(ebp+5), *(ebp+6));
        cprintf("%s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, *(ebp+1)-(info.eip_fn_addr));
    }

	return 0;
}

int
mon_showmappings (int argc, char **argv, struct Trapframe *tf)
{
    //jyhsu: Display physical page mappings that apply to a particular
    //       range of virtual/linear addresses in the currently ative
    //       address space. Ex. 'showmappings 0x3000 0x5000' displays
    //       the physical page mappings and corresponding permission
    //       bits that apply to the pages at virtual addresses 0x3000,
    //       0x4000, and 0x5000.
    //       Format: showmappings (address1) (address2)
    //       addresses must be heximal.

    uintptr_t va1, va2, i;
    struct PageInfo *page;
    pte_t *pte;
    char us, rw;

    if (argc != 3)
        cprintf("# of arguments incorrect!\n");
    else {
        if (heximal_value(argv[1], &va1) || heximal_value(argv[2], &va2))
            cprintf("Heximal Format incorrect!\n");
        else {
            for (i = ROUNDDOWN(va1, PGSIZE); i <= va2; i += PGSIZE) {
                page = page_lookup(kern_pgdir, PGADDR(PDX(i), PTX(i), PGOFF(i)), &pte);
                if (!page)
                    cprintf("0x%08x->NULL\n", i);
                else {
                    us = (*pte & PTE_U) ? 'u' : 's';
                    rw = (*pte & PTE_W) ? 'w' : 'r';
                    cprintf("0x%08x->0x%08x %c%c\n", i, PTE_ADDR(*pte), us, rw);
                }
            }
        }
    }

    return 0;
}

int
mon_changeperm (int argc, char **argv, struct Trapframe *tf)
{
    //jyhsu: Change the permissions of any mapping in the current
    //       address space. Format: changeperm [u|s][r|w] (vaddress)
    //       u: User, s: Supervisor, r: RO, w: RW.
    //       address is virtual and must be heximal.

    uint32_t addr, perm = ~0, virtual = 0;
    struct PageInfo *page;
    pte_t *pte;
    int perm_check = 1;

    if (argc != 3)
        cprintf("# of arguments incorrect!\n");
    else {
        switch (argv[1][0]) {
            case 's':   perm &= ~PTE_U;
            case 'u':   break;
            default:    perm_check = 0;
        }

        switch (argv[1][1]) {
            case 'r':   perm &= ~PTE_W;
            case 'w':   break;
            default:    perm_check = 0;
        }

        if (!perm_check)
            cprintf("Permssion Format incorrect!\n");
        else if (heximal_value(argv[2], &addr))
            cprintf("Heximal Format incorrect!\n");
        else {
            page = page_lookup(kern_pgdir, PGADDR(PDX(addr), PTX(addr), PGOFF(addr)), &pte);
            if (!page)
                cprintf("Invalid virtual Address!\n");
            else
                *pte &= perm;
        }
    }

    return 0;
}

int
mon_memdump (int argc, char **argv, struct Trapframe *tf)
{
    //jyhsu: Dump the contents of a range of memory given either a virtual
    //       or physical address range.
    //       Format: memdump [p|v] (address1) (address2)
    //       addresses must be heximal.

    uint32_t i, j, addr1, addr2, virtual = 0, base, *pa;
    struct PageInfo *page, *page2;
    pte_t *pte;
    int virtual_check = 1, print_count = 0;

    if (argc != 4)
        cprintf("# of arguments incorrect!\n");
    else {
        switch (argv[1][0]) {
            case 'v':   virtual = 1;
            case 'p':   break;
            default:    virtual_check = 0;
        }

        if (!virtual_check)
            cprintf("Neither virtual nor physical is specified!\n");
        else if (heximal_value(argv[2], &addr1) || heximal_value(argv[3], &addr2))
            cprintf("Heximal Format incorrect!\n");
        else {
            if (virtual) {
                for (i = ROUNDDOWN(addr1, PGSIZE); i <= ROUNDDOWN(addr2, PGSIZE); i += PGSIZE) {
                    cprintf("0x%08x ~ 0x%08x:\n", MAX(i, addr1), MIN(i+PGSIZE, addr2));

                    print_count = 0;
                    page = page_lookup(kern_pgdir, PGADDR(PDX(i), PTX(i), PGOFF(i)), &pte);
                    if (!page)
                        cprintf("NULL\n");
                    else {
                        base = PTE_ADDR(*pte);
                        for (j = base+PGOFF(MAX(i ,addr1)); j <= (i+PGSIZE <= addr2 ? base+PGSIZE-0x4 : base+PGOFF(addr2)); j += 0x4) {
                            pa = (uint32_t *) KADDR(j);
                            cprintf("0x%08x ", *pa);
                            if((print_count = (print_count+1)%4) == 0)
                                cprintf("\n");
                        }
                        if (print_count > 0)
                            cprintf("\n");
                    }
                }
            }
            else {
                for (i = ROUNDDOWN(addr1, PGSIZE); i <= ROUNDDOWN(addr2, PGSIZE); i += PGSIZE) {
                    cprintf("0x%08x ~ 0x%08x:\n", MAX(i, addr1), MIN(i+PGSIZE, addr2));

                    print_count = 0;
                    for (j = MAX(i ,addr1); j <= MIN(i+PGSIZE, addr2); j += 4) {
                        pa = (uint32_t *) KADDR(j);
                        cprintf("0x%08x ", *pa);

                        if ((print_count = (print_count+1)%4) == 0)
                            cprintf("\n");
                    }
                    if (print_count > 0)
                        cprintf("\n");
                }
            }
        }

    }

    return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
