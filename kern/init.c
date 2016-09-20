/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/monitor.h>
#include <kern/console.h>
#include <kern/pmap.h>
#include <kern/kclock.h>

// Test the stack backtrace function (lab 1 only)
//jyhsu: func located at 0xf0100040
void
test_backtrace(int x)
{
    /*jyhsu:
        (in parent)
        push {last_arg}
        ...
        push {first_arg}
        call {blah} #ret addr pushing done here
        (in func)
        mov %esp, %ebp
        push %ebp
        (normal func code)
        
        stack:
            ---------high addr, esp before preparing func call
            func arg
            ret addr
            pushed ebp
            ---------low addr, esp before normal func code
    */

    cprintf("entering test_backtrace %d\n", x);
	if (x > 0)
		test_backtrace(x-1);
	else
		mon_backtrace(0, 0, 0);
	cprintf("leaving test_backtrace %d\n", x);
}

void
i386_init(void)
{
	extern char edata[], end[];

	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();

    //jyhsu: Lab1 test code part 1
    /*
    unsigned i = 0x00646c72;
    cprintf("H%x, Wo%s", 57616, &i); //57616=0xe110, 72='r', 6c='l', 64='d'
    */

    //jyhsu: Lab1 test code part 2
    /*
    cprintf("x=%d y=%d", 3); //va_arg does not check if ap reach eo list
    */

	cprintf("6828 decimal is %o octal!\n", 6828);

	// Lab 2 memory management initialization functions
	mem_init();

	// Drop into the kernel monitor.
	while (1)
		monitor(NULL);
}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	// Be extra sure that the machine is in as reasonable state
	__asm __volatile("cli; cld");

	va_start(ap, fmt);
	cprintf("kernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}
