#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
//goes over the children recursively and prints nodes with their labels and edges. 
int draw_tree(char *pid, FILE *dot)
{
	FILE *child_list;
	FILE *child_list2;
	char cmd[1024];
	strcpy(cmd,"ps --format pid,time --ppid ");
	strcat(cmd,pid);
	char buffer[1024];
	int i = 0;
	if((child_list2 = popen(cmd,"r"))== NULL){
		printf("%s\n","Error openning pipe" );
		exit(-1);
		}
	char max[1024];
	int k = 0;
	//to determine the oldest child I read all of the child from the file and check their times
	while (fgets(buffer, 1024, child_list2) != NULL) {
			if(k == 0){
				strcpy(max,buffer);
			}else{
				char copy[1024];
	       		strcpy(copy,buffer);
	       		char *token = strtok(copy," ");
	       		char pid_child[1024];
	       		strcpy(pid_child,token);
	       		token = strtok(NULL," ");
	       		if(strcmp(token,max)>0){
	       			strcpy(max,token);
	       		}
			}
				

	}
	//to build the diagraph structure goes over the childs of the process with given pid recursively and for
	//each child it prints a node and prints the necessary edge between the given pid and child pid
	if((child_list = popen(cmd,"r"))== NULL){
		printf("%s\n","Error openning pipe" );
		exit(-1);
		}
	while (fgets(buffer, 1024, child_list) != NULL) {
	       		char copy[1024];
	       		strcpy(copy,buffer);
	       		char *token = strtok(copy," ");
	       		char pid_child[1024];
	       		strcpy(pid_child,token);
	       		if(i>0){
		       		//printf("\n %s%s",indent,token);
		       		token = strtok(NULL," ");
		       		//printf("\n %s%s",indent,token);
		       		if(dot != NULL){
				        /* Output a node for this child, */
				        if(strcmp(max,buffer) == 0){
				        	 printf("%s\n",pid_child );
					        fprintf(dot, "    \"%s\" [ label=\"%s %s\", color=red ] \n", pid_child,pid_child, token);
				        }else{
					        printf("%s\n",pid_child );
					        fprintf(dot, "    \"%s\" [ label=\"%s %s\" ];\n", pid_child,pid_child, token);
				        }
				        /* and if not at the top level (0), an edge from our parent. */
				       
				        fprintf(dot, "    \"%s\" -> \"%s\";\n", pid, pid_child);

				        fflush(dot);
				    

		       		draw_tree(pid_child,dot);
	   		}	
	   		
	   	}	
			i+=1;
   
   
}
		return EXIT_SUCCESS;
}
//prints the starts and ends for the diagraph structure. When the resulting diagraph{...} is given to dot -Tx11 as input with a pipe
//it prints us the graph in a png form and opens it in a graphiz frame
int process_tree(FILE *out,char *pid)
{

    if (out) {
        fprintf(out, "digraph {\n");
         
        fflush(out);
    }


    draw_tree(pid, out);
    if (out) {

        fprintf(out, "}\n");
        fflush(out);
    }
	
    return EXIT_SUCCESS;
}
int main(int argc, char *argv[])
{
    return process_tree(stdout, argv[1]);
}
