
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include<iostream>
#include<vector>
#include<cstdlib>
using namespace std;
pthread_t tower;
vector<pthread_t> landing_planes;
vector<pthread_t> taking_off_planes;
struct notification_args{
	int arg1;
	int arg2;
};
 /****************************************************************************** 
  pthread_sleep takes an integer number of seconds to pause the current thread 
  original by Yingwu Zhu
  updated by Muhammed Nufail Farooqi
  *****************************************************************************/
int pthread_sleep (int seconds)
{
   pthread_mutex_t mutex;
   pthread_cond_t conditionvar;
   struct timespec timetoexpire;
   if(pthread_mutex_init(&mutex,NULL))
    {
      return -1;
    }
   if(pthread_cond_init(&conditionvar,NULL))
    {
      return -1;
    }
   struct timeval tp;
   //When to expire is an absolute time, so get the current time and add //it to our delay time
   gettimeofday(&tp, NULL);
   timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

   pthread_mutex_lock (&mutex);
   int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
   pthread_mutex_unlock (&mutex);
   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&conditionvar);

   //Upon successful completion, a value of zero shall be returned
   return res;

}
double random_generator(){ //BUNA Bİ GÖZ AT ÇOK DA RANDOM DURMUYIR
	return (double)(rand() % 100)/100;
}
void* notify_tower(void* args){
    struct notification_args* parameters = (struct notification_args *)args;
	int plane_id = parameters->arg1;
	int is_landing = parameters->arg2;
	if(is_landing){
		cout<<"I am plane "<<plane_id<<" me landing"<<endl;
	} else{
		cout<<"I am plane "<<plane_id<<" me taking off dude"<<endl;
	}
}
void *create_plane(double p){
	
	double chosen_p = random_generator();
	struct notification_args args[2]; 
	if(chosen_p > p){ // 1-p
		pthread_t taking_off;
		int thread_id = taking_off_planes.size();
		args->arg1 = thread_id;
		args->arg2 = 0; //0 for taking off
		pthread_create(&taking_off, NULL, notify_tower,(void*)args);
		taking_off_planes.push_back(taking_off);
	}else{// p
		pthread_t landing;
		int thread_id = landing_planes.size();
		args->arg1 = thread_id;
		args->arg2 = 1; // 1 for landing planes
		pthread_create(&landing, NULL, notify_tower, (void*)args);
		landing_planes.push_back(landing);
	}
	return NULL;
}
void *count(void *ptr){
	cout<<"Iam here ya";
	for(int i = 0; i<5; i++)
		cout<<i<<" ";
	return NULL;
}
int main(int argc, char* argv[]){
	pthread_create(&tower, NULL, count, NULL);
	
	for(int i = 0; i<50; i++)
		create_plane(atof(argv[1]));

	pthread_exit(NULL);
	return 0;
}