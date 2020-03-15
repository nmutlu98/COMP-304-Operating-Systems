#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/sched/signal.h> 
#define EXIT_SUCCESS 0
static int pid = -1;

module_param(pid, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(pid, "Pid of the process");
//traverses the subprocess tree whose root is the given pid
int draw_tree(pid_t pid)
{	struct task_struct *task;
	int flag_found = -1;
	for_each_process(task){
		if(task->pid == pid){
			flag_found = 1;
			break;
		}
	}
	if(flag_found == -1){
 		printk(KERN_ERR "Invalid pid number");
		return 1;
	} else{
		struct task_struct *parent = task;
		
		printk(KERN_INFO "Printing children of %d",pid);
		int empty = -1;
		struct task_struct *task_child;
		struct list_head *list;
		list_for_each(list, &parent->children) {
		    task_child = list_entry(list, struct task_struct, sibling);
		    printk(KERN_INFO "Child pid: %d Child Start Time: %lld",task_child->pid,task_child->start_time);
		    draw_tree(task_child->pid);
	    	empty = 1;
	    	
		}
		if(empty==-1){
			printk(KERN_INFO "No children found");
		}
	}
	return EXIT_SUCCESS;
		
}

int process_tree(pid_t pid)
{
    draw_tree(pid);
    return EXIT_SUCCESS;
}

static int __init part3_init(void)
{
	printk(KERN_INFO "Loading module\n");
	if(pid == -1){
		printk(KERN_ERR "Empty pid number.");
		return 1;
	}
	process_tree(pid);
	
	return 0;
}

static void __exit part3_exit(void)
{
	printk(KERN_INFO "Removing the module\n");
}
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("SGG");
module_init(part3_init);
module_exit(part3_exit);