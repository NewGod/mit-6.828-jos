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
#include <kern/trap.h>

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
    { "backtrace", "Display a listing of function call frames", mon_backtrace},
    { "showmappings", "Display the memory mapping", mon_showmappings }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
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
    uint32_t* ebp = (uint32_t*) read_ebp();
    cprintf("Stack backtrace;\n");
    for (uint32_t* ebp = (uint32_t*) read_ebp(); ebp; ebp = (uint32_t*) *ebp){
        uint32_t eip = ebp[1];
        cprintf("ebp %x eip %x args", ebp, eip);
        struct Eipdebuginfo info;
        debuginfo_eip(eip, &info);
        for (int i=2; i<=6; i++) 
            cprintf(" %08.x", ebp[i]);
        cprintf("\n");
		cprintf(" %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
    }
	return 0;
}

int 
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    extern pte_t *pgdir_walk(pde_t *pgdir, const void *va, int create);
    extern pde_t *kern_pgdir;

    if (argc != 3) {
        cprintf("Usage: showmappings begin_addr end_addr\n");
        return 0;
    }
    
    long begin = strtol(argv[1], NULL, 0);
    long end = strtol(argv[2], NULL, 0);
    if (begin != ROUNDUP(begin, PGSIZE) || end != ROUNDUP(end, PGSIZE))
    {
        cprintf("Warning: not aligned\n the address will aligned automaticly\n");
        begin = ROUNDUP(begin, PGSIZE);
        end = ROUNDUP(end, PGSIZE);
    }
    if (end <= begin) {
        cprintf("Error: end_addr must larger than begin_addr\n");
        return 0;
    }
    for (; begin < end; begin += PGSIZE){
        cprintf("%08x--%08x: ", begin, begin+PGSIZE);
        pte_t *pte = pgdir_walk(kern_pgdir, (void*)begin, 0);
        if (!pte)
        {
            cprintf("not mapped\n");
            continue;
        }
        cprintf("page %08x ", PTE_ADDR(*pte));
        cprintf("PTE_P: %x, PTE_W: %x, PTE_U: %x\n", *pte&PTE_P, *pte&PTE_W, *pte&PTE_U);
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
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
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

	cprintf("\033[0;31;40mWelcome \033[0;32;40mto \033[0;33;40mthe \033[0;34;40mJOS \033[0;35;40mkernel \033[0;36;40mmonitor!\033[0m\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
