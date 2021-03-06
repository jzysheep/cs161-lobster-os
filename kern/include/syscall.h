/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <cdefs.h> /* for __DEAD */
#include <array.h>
struct trapframe; /* from <machine/trapframe.h> */

/*
 * The system call dispatcher.
 */

void syscall(struct trapframe *tf);

/*
 * Support functions.
 */

/* Helper for fork(). You write this. */
void enter_forked_process(struct trapframe *tf);

/* Enter user mode. Does not return. */
__DEAD void enter_new_process(int argc, userptr_t argv, userptr_t env,
		       vaddr_t stackptr, vaddr_t entrypoint);

/* Gets called when the kernel first starts up */
int runprogram(char *progname, char **args, int argc);
/* Should not be called directly by the user; gets called by runprogram */
int _launch_program(char *progname, vaddr_t *stack_ptr, vaddr_t *entry_point);

// Helper functions for both runprogram and execv
int extract_args(userptr_t args, char *buf, struct array *argv, struct array *argv_lens, bool copy_args);
void copy_args_to_stack(vaddr_t *stack_ptr, struct array *argv, struct array *argv_lens);


/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);
int sys_execv(userptr_t prog, userptr_t args);
void sys__exit(int exitcode);
int sys_fork(struct trapframe *parent_tf, pid_t *retval);
int sys_getpid(pid_t* retval);
int sys_waitpid(pid_t pid, int *status, int options, pid_t *retval);

/*
 * Prototypes for file system calls
 */

int sys_open(userptr_t filename, int flags, int *fd);
int sys_close(int fd);
int sys_read(int fd, userptr_t buf, size_t len, size_t *read);
int sys_write(int fd, userptr_t buf, size_t len, size_t *wrote);
int sys_lseek(int fd, off_t pos, int whence, off_t *new_pos);
int sys_dup2(int old_fd, int new_fd);
int sys_chdir(userptr_t path);
int sys___getcwd(userptr_t buf, size_t len, size_t *copied);

int sys_sync(void);
int sys_mkdir(userptr_t path, mode_t mode);
int sys_rmdir(userptr_t path);
int sys_remove(userptr_t path);
int sys_link(userptr_t oldpath, userptr_t newpath);
int sys_rename(userptr_t oldpath, userptr_t newpath);
int sys_getdirentry(int fd, userptr_t buf, size_t buflen, int *retval);
int sys_fstat(int fd, userptr_t statptr);
int sys_fsync(int fd);
int sys_ftruncate(int fd, off_t len);

/*
 * Prototypes for memory management system calls
 */

int sys_sbrk(int32_t amount, int32_t *retval);

#endif /* _SYSCALL_H_ */
