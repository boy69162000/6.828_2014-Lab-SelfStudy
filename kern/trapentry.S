/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

    //jyhsu: my code
    TRAPHANDLER_NOEC(DIVIDE, T_DIVIDE);
    TRAPHANDLER_NOEC(DEBUG, T_DEBUG);
    TRAPHANDLER_NOEC(NMI, T_NMI);
    TRAPHANDLER_NOEC(BRKPT, T_BRKPT);
    TRAPHANDLER_NOEC(OFLOW, T_OFLOW);
    TRAPHANDLER_NOEC(BOUND, T_BOUND);
    TRAPHANDLER_NOEC(ILLOP, T_ILLOP);
    TRAPHANDLER_NOEC(DEVICE, T_DEVICE);
    TRAPHANDLER(DBLFLT, T_DBLFLT);
    //9 is reserved
    TRAPHANDLER(TSS, T_TSS);
    TRAPHANDLER(SEGNP, T_SEGNP);
    TRAPHANDLER(STACK, T_STACK);
    TRAPHANDLER(GPFLT, T_GPFLT);
    TRAPHANDLER(PGFLT, T_PGFLT);
    //15 is reserved
    TRAPHANDLER_NOEC(FPERR, T_FPERR);
    TRAPHANDLER(ALIGN, T_ALIGN);
    TRAPHANDLER_NOEC(MCHK, T_MCHK);
    TRAPHANDLER_NOEC(SIMDERR, T_SIMDERR);
    //20-31 are reserved
    //32-47 are IRQ
    TRAPHANDLER_NOEC(TIMER, IRQ_OFFSET+IRQ_TIMER);  //32+0
    TRAPHANDLER_NOEC(KBD, IRQ_OFFSET+IRQ_KBD);  //32+1
    TRAPHANDLER_NOEC(IRQ_2, IRQ_OFFSET+2);
    TRAPHANDLER_NOEC(IRQ_3, IRQ_OFFSET+3);
    TRAPHANDLER_NOEC(SERIAL, IRQ_OFFSET+IRQ_SERIAL);    //32+4
    TRAPHANDLER_NOEC(IRQ_5, IRQ_OFFSET+5);
    TRAPHANDLER_NOEC(IRQ_6, IRQ_OFFSET+6);
    TRAPHANDLER_NOEC(SPURIOUS, IRQ_OFFSET+IRQ_SPURIOUS);    //32+7
    TRAPHANDLER_NOEC(IRQ_8, IRQ_OFFSET+8);
    TRAPHANDLER_NOEC(IRQ_9, IRQ_OFFSET+9);
    TRAPHANDLER_NOEC(IRQ_10, IRQ_OFFSET+10);
    TRAPHANDLER_NOEC(IRQ_11, IRQ_OFFSET+11);
    TRAPHANDLER_NOEC(IRQ_12, IRQ_OFFSET+12);
    TRAPHANDLER_NOEC(IRQ_13, IRQ_OFFSET+13);
    TRAPHANDLER_NOEC(IDE, IRQ_OFFSET+IRQ_IDE);  //32+14
    TRAPHANDLER_NOEC(IRQ_15, IRQ_OFFSET+15);
    TRAPHANDLER_NOEC(ERROR, IRQ_OFFSET+IRQ_ERROR);  //32+19
    //48 is syscall
    TRAPHANDLER_NOEC(SYSCALL, T_SYSCALL); //48
    TRAPHANDLER_NOEC(DEFAULT, T_DEFAULT); //500, none of the traps above

/*
 * Lab 3: Your code here for _alltraps
 */

    //jyhsu: my code
_alltraps:
    pushl %eax
    movl $GD_UD, %eax
    orl $3, %eax
    movw %ax, %ds
    movw %ax, %es
    popl %eax
    pushl %ds //ds
    pushl %es //es
    pushal     //normal reg

    movw $GD_KD, %ax
    movw %ax, %ds
    movw %ax, %es

    pushl %esp
    call trap

trapfail:
    jmp trapfail
