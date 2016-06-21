**********************************SHELL PROGRAM**************************************
Created a custom shell that has 4 builtin commands (cd, pwd, path, and exit).
Path command allows a user to set a specific path for additional commands, the default being /bin. User runs ./whoosh, then is able to enter those commands. 
Program parses the commands, then does the necessary function. Any error that occurs
prints a simple error message. Additionally, a user can chose to direct the output of a command using “> {filename}”.

**********************************LINUX SCHEDULER************************************

Added two new system calls and changed the scheduler to priority based. Every process is by default, given a priority of 1. The priority of a program can be changed calling setpri(int), information about a process can displayed by calling getpinfo(struct stat *).
Steps:
	1)include/syscall.h: 	#define SYS_setpri 22
				#define SYS_getpinfo 23
	2)kernel/sysfunc.h:	int sys_setpri(void)
				int getpinfo(void)
	3)kernel/sysproc.c	implementation of functions (actually calls setpri()
				and getpinfo()). Parses arguments using argptr(int, 					ptr) and argint(int, int)
	4)kernel/syscall.c	[SYS_setpri] sys_setpri,
				[SYS_getpinfo] sys_getpinfo,
	5)kernel/defs.h		int setpri(int)		
				int getpinfo(struct stat *)
	6)kernel/proc.c		actual logic for functions
	7)user/usys.S		SYSCALL(setpri)
				SYSCALL (getpinfo)
	8)user/user.h		int setpri(int)
				int getpinfo(struct stat *)
	9)makefile.mk		setpri\
				getpinfo\
	10)update scheduler	change scheduler to sort processes based on their 					priority, always run higher priority over lower 					priority (maintain round robin for same priority level 				processes. Clear counts and queues at the end.
				
	
