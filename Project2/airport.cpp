
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include<iostream>
#include<vector>
#include<cstdlib>
#include<string.h>
#include<queue>
using namespace std;
pthread_t tower;
int plane_counter = 0;
pthread_cond_t runway_permission_landings;
pthread_cond_t runway_permission_taking_offs;
pthread_cond_t completed;
pthread_mutex_t runway_mutex;
pthread_mutex_t landing_queue_mutex;
pthread_mutex_t taking_off_queue_mutex;
pthread_mutex_t counting_mutex;
pthread_mutex_t in_progress_mutex;
bool simulation_finished = false;
bool tower_destroyed = false;
struct plane{
	pthread_t plane_thread;
	int index;
	bool is_landing;
	int plane_id;
	bool completed_action = false;
	struct plane* next;
};
queue<struct plane*> landing_planes;
queue<struct plane*> taking_off_planes;
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
void* notify_tower_landing(void* plane){
	
	struct plane* landing_plane = (struct plane*) plane;
	pthread_mutex_lock(&landing_queue_mutex);
	landing_planes.push(landing_plane);
	pthread_mutex_unlock(&landing_queue_mutex);
	int id = landing_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_landings, &runway_mutex);
	while(landing_planes.front()->plane_id != landing_plane->plane_id){
		pthread_cond_wait(&runway_permission_landings, &runway_mutex);
	}
	landing_plane->completed_action = true;
	cout<<"LANDING "<<id<<endl;
	pthread_mutex_unlock(&runway_mutex);
		cout<<"THIS PLANE COMPLETEd"<<endl;	
}
void* notify_tower_taking_off(void* plane){
	
	struct plane* taking_off_plane = (struct plane*) plane;
	pthread_mutex_lock(&taking_off_queue_mutex);
	taking_off_planes.push(taking_off_plane);
	pthread_mutex_unlock(&taking_off_queue_mutex);
	int id = taking_off_plane->plane_id;
	pthread_mutex_unlock(&taking_off_queue_mutex);
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_taking_offs, &runway_mutex);
	while(taking_off_planes.front()->plane_id != taking_off_plane->plane_id)
		pthread_cond_wait(&runway_permission_taking_offs, &runway_mutex);
	taking_off_plane->completed_action = true;
	cout<<"TAKING OFF "<<id<<endl;
	pthread_mutex_unlock(&runway_mutex);
	cout<<"THIS PLANE COMPLETEd"<<endl;
	return NULL;
} 
void *create_plane(double p){
	struct plane* new_plane = (struct plane*)malloc(sizeof(struct plane*));
	new_plane->plane_thread = (pthread_t)malloc(sizeof(pthread_t));
	double chosen_p = random_generator();
	if(chosen_p > p){ // 1-p
		pthread_t taking_off;
		pthread_mutex_lock(&counting_mutex);
		new_plane->plane_id = plane_counter;
		cout<<"TAKING OF "<<new_plane->plane_id<<" CREATED"<<endl;
		plane_counter++;
		pthread_mutex_unlock(&counting_mutex),
		new_plane->index = 0;
		new_plane->is_landing = false;
		new_plane->plane_thread = taking_off;
		pthread_create(&taking_off, NULL, notify_tower_taking_off,(void*)new_plane);
	}else{// p
		pthread_t landing;
		pthread_mutex_lock(&counting_mutex);
		new_plane->plane_id = plane_counter;
		cout<<"LANDING "<<new_plane->plane_id<<" CREATED"<<endl;
		plane_counter++;
		pthread_mutex_unlock(&counting_mutex);
		new_plane->index = 0;
		new_plane->is_landing = true;
		new_plane->plane_thread = landing;
		pthread_create(&landing, NULL, notify_tower_landing, (void*)new_plane);
	}
	tower_destroyed = true;
	return NULL;
}
void *check_notifications(void *ptr){
	while(!simulation_finished){
		while(!simulation_finished && landing_planes.size() != 0){ //BUNLARI CHECK EDERKEN DE RACE CONDITION OLABILIR LOCK LAZIM
			pthread_mutex_lock(&runway_mutex);
			pthread_cond_broadcast(&runway_permission_landings);
			pthread_mutex_unlock(&runway_mutex);
			pthread_sleep(2);
			pthread_mutex_lock(&landing_queue_mutex);
			if(landing_planes.front()->completed_action)
				landing_planes.pop();
			pthread_mutex_unlock(&landing_queue_mutex);
			
		}
		
		if(taking_off_planes.size() != 0){
			pthread_mutex_lock(&runway_mutex);
			pthread_cond_broadcast(&runway_permission_taking_offs);
			pthread_mutex_unlock(&runway_mutex);
			pthread_sleep(2);
			pthread_mutex_lock(&taking_off_queue_mutex);
			if(taking_off_planes.front()->completed_action)
				taking_off_planes.pop();
			pthread_mutex_unlock(&taking_off_queue_mutex);
		}
		

	}
	return NULL;
}
void run_simulation(double simulation_time, double probability){
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	int seconds = current_time.tv_sec;
	double end_time = current_time.tv_sec + simulation_time;
	while(current_time.tv_sec < end_time){
		gettimeofday(&current_time,NULL);
		if(current_time.tv_sec - seconds == 1){
			seconds++;
			create_plane(probability);
		}
		
	}
	simulation_finished = true;

}
int main(int argc, char* argv[]){
	pthread_create(&tower, NULL, check_notifications, NULL);
	pthread_mutex_init(&runway_mutex, NULL);
	pthread_mutex_init(&counting_mutex, NULL);
	pthread_mutex_init(&in_progress_mutex, NULL);
	pthread_mutex_init(&landing_queue_mutex, NULL);
	pthread_mutex_init(&taking_off_queue_mutex, NULL);
	pthread_cond_init(&runway_permission_landings, NULL);
	pthread_cond_init(&runway_permission_taking_offs, NULL);
	pthread_cond_init(&completed, NULL);
	create_plane(1);//Initial landing plane
	create_plane(0);//Initial taking off plane
	if(strcmp("-s", argv[1]) == 0 && strcmp("-p", argv[3]) == 0)
		run_simulation(atof(argv[2]), atof(argv[4]));
	else if(strcmp("-p", argv[1]) == 0 && strcmp("-s", argv[3]) == 0)
		run_simulation( atof(argv[4]), atof(argv[2]));
	else{
		cout<<"INVALID ARGUMENT"<<endl;
		return NULL;
	}
	
	
	
	return 0;
}