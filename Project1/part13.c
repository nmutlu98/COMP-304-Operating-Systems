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
int draw_tree(pid_t pid, char *diagraph)
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
		
		printk(KERN_INFO "Printing children");
		int empty = -1;
		struct task_struct *task_child;
		struct list_head *list;
		long long int time = -1;
		pid_t max_child = -1;
		list_for_each(list, &parent->children) {
		    task_child = list_entry(list, struct task_struct, sibling);
		    if(task_child->start_time>time){
		    	max_child = task_child->pid;
		    	time = task_child->start_time;
		    }

	    	empty = 1;
	    	
		}
		list_for_each(list, &parent->children) {
		    task_child = list_entry(list, struct task_struct, sibling);
		    char line[1024];
		    char line2[1024];
		    if(task_child->pid == max_child)
		    	 sprintf(line, "    \"%d\" [ label=\"%d %lld\" , color=red ];\n", task_child->pid,task_child->pid, task_child->start_time);
		    else
		    	sprintf(line, "    \"%d\" [ label=\"%d %lld\" ];\n", task_child->pid,task_child->pid, task_child->start_time);
	    	
	    	sprintf(line2, "    \"%d\" -> \"%d\";\n", pid, task_child->pid);
	    	strcat(diagraph,line);
	    	strcat(diagraph,line2);
	    	
	    	
	    	empty = 1;
	    	
		}
		if(empty==-1){
			printk(KERN_INFO "No children found");
		}
	}
	return EXIT_SUCCESS;
		
}
//prints the starts and ends for the diagraph structure. When the resulting diagraph{...} is given to dot -Tx11 as input with a pipe
//it prints us the graph in a png form and opens it in a graphiz frame
int process_tree(pid_t pid)
{
	char diagraph[10000];
    strcat(diagraph, "digraph {\n");
	
    draw_tree(pid, diagraph);
    strcat(diagraph, "}\n");
  	printk(KERN_INFO "Diagraph: %s",diagraph);
  	
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