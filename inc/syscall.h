#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	NSYSCALLS
};
#define MSR_IA32_SYSTEM_CS (0x174)
#define MSR_IA32_SYSTEM_ESP (0x175)
#define MSR_IA32_SYSTEM_EIP (0x176)

#endif /* !JOS_INC_SYSCALL_H */
