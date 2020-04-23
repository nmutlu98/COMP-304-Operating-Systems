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
pthread_cond_t runway_permission_emergencies;
pthread_cond_t completed;
pthread_mutex_t runway_mutex;
pthread_mutex_t landing_queue_mutex;
pthread_mutex_t taking_off_queue_mutex;
pthread_mutex_t counting_mutex;
pthread_mutex_t in_progress_mutex;
pthread_mutex_t emergency_mutex;
bool simulation_finished = false;
bool tower_destroyed = false;
struct plane{
	pthread_t plane_thread;
	int index;
	bool is_landing;
	bool is_emergency = false;
	int plane_id;
	bool completed_action = false;
	struct plane* next;
};
queue<struct plane*> landing_planes;
queue<struct plane*> taking_off_planes;
queue<struct plane*> emergency;
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
void* notify_emergency(void* plane){
	struct plane* urgent_plane = (struct plane*) plane;
	pthread_mutex_lock(&emergency_mutex);
	emergency.push(urgent_plane);
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	cout<<"******************"<<endl;
	cout<<"YARATILDIM EMERGENCY"<<urgent_plane->plane_id<<endl;
	cout<<current_time.tv_sec<<endl;
	cout<<"********************"<<endl;
	pthread_mutex_unlock(&emergency_mutex);
	int id = urgent_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_emergencies, &runway_mutex);
	while(emergency.front()->plane_id != urgent_plane->plane_id){
		pthread_cond_wait(&runway_permission_emergencies, &runway_mutex);
	}
	urgent_plane->completed_action = true;
	cout<<"****************"<<endl;
	cout<<"EMERGENCY "<<id<<endl;
	gettimeofday(&current_time, NULL);
	cout<<current_time.tv_sec<<endl;
	cout<<"*********************"<<endl;
	pthread_mutex_unlock(&runway_mutex);
		cout<<"THIS PLANE COMPLETEd"<<endl;		
}
void* notify_tower_landing(void* plane){
	
	struct plane* landing_plane = (struct plane*) plane;
	pthread_mutex_lock(&landing_queue_mutex);
	landing_planes.push(landing_plane);
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	cout<<"******************"<<endl;
	cout<<"YARATILDIM LANDING"<<landing_plane->plane_id<<endl;
	cout<<current_time.tv_sec<<endl;
	cout<<"********************"<<endl;
	pthread_mutex_unlock(&landing_queue_mutex);
	int id = landing_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_landings, &runway_mutex);
	while(landing_planes.front()->plane_id != landing_plane->plane_id){
		pthread_cond_wait(&runway_permission_landings, &runway_mutex);
	}
	landing_plane->completed_action = true;
	cout<<"****************"<<endl;
	cout<<"LANDING "<<id<<endl;
	gettimeofday(&current_time, NULL);
	cout<<current_time.tv_sec<<endl;
	cout<<"*********************"<<endl;
	pthread_mutex_unlock(&runway_mutex);
		cout<<"THIS PLANE COMPLETEd"<<endl;	
}
void* notify_tower_taking_off(void* plane){
	
	struct plane* taking_off_plane = (struct plane*) plane;
	pthread_mutex_lock(&taking_off_queue_mutex);
	taking_off_planes.push(taking_off_plane);
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	cout<<"*************************"<<endl;
	cout<<"YARATILDIM OFF"<<taking_off_plane->plane_id<<endl;
	cout<<current_time.tv_sec<<endl;
	cout<<"***********************"<<endl;
	pthread_mutex_unlock(&taking_off_queue_mutex);
	int id = taking_off_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_taking_offs, &runway_mutex);
	while(taking_off_planes.front()->plane_id != taking_off_plane->plane_id)
		pthread_cond_wait(&runway_permission_taking_offs, &runway_mutex);
	taking_off_plane->completed_action = true;
	cout<<"**************************************"<<endl;
	cout<<"TAKING OFF "<<id<<endl;
	cout<<"********************************"<<endl;
	gettimeofday(&current_time, NULL);
	cout<<current_time.tv_sec<<endl;
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
void *create_emergency_plane(){
	pthread_t urgent_thread;
	struct plane* new_plane = (struct plane*)malloc(sizeof(struct plane*));
	new_plane->plane_thread = (pthread_t)malloc(sizeof(pthread_t));
	new_plane->is_emergency = true;
	pthread_mutex_lock(&counting_mutex);
	new_plane->plane_id = plane_counter;
	plane_counter++;
	pthread_mutex_unlock(&counting_mutex);
	new_plane->plane_thread = urgent_thread;
	pthread_create(&urgent_thread, NULL, notify_emergency, (void*)new_plane); 
}
void *let_emergencies(){
	pthread_mutex_lock(&emergency_mutex);
	while(emergency.size() != 0){
		pthread_mutex_unlock(&emergency_mutex);
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_emergencies);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&emergency_mutex);
		if(emergency.front()->completed_action)
			emergency.pop();
		pthread_mutex_unlock(&emergency_mutex);
	}
	pthread_mutex_unlock(&emergency_mutex);
}
void *let_one_to_take_off(){
	pthread_mutex_lock(&taking_off_queue_mutex);
	if(taking_off_planes.size() != 0 && taking_off_planes.size() < 5){
		pthread_mutex_unlock(&taking_off_queue_mutex);
		let_emergencies();
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_taking_offs);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&taking_off_queue_mutex);
		if(taking_off_planes.front()->completed_action)
			taking_off_planes.pop();
		pthread_mutex_unlock(&taking_off_queue_mutex);
	}
	pthread_mutex_unlock(&taking_off_queue_mutex);
}
void *let_landings(){
	pthread_mutex_lock(&landing_queue_mutex);
	pthread_mutex_lock(&taking_off_queue_mutex);
	while(taking_off_planes.size()<5 && landing_planes.size() != 0){ //BUNLARI CHECK EDERKEN DE RACE CONDITION OLABILIR LOCK LAZIM
		pthread_mutex_unlock(&taking_off_queue_mutex);
		pthread_mutex_unlock(&landing_queue_mutex);
		let_emergencies();
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_landings);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&landing_queue_mutex);
		if(landing_planes.front()->completed_action)
			landing_planes.pop();
			pthread_mutex_lock(&taking_off_queue_mutex);
		}
		pthread_mutex_unlock(&taking_off_queue_mutex);
		pthread_mutex_unlock(&landing_queue_mutex);	
		let_one_to_take_off();
}
void *let_one_to_land(){
	pthread_mutex_lock(&landing_queue_mutex);
	if(landing_planes.size() != 0){
		pthread_mutex_unlock(&landing_queue_mutex);
		let_emergencies();
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_landings);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&landing_queue_mutex);
		if(landing_planes.front()->completed_action)
			landing_planes.pop();
		pthread_mutex_unlock(&landing_queue_mutex);
	}
	pthread_mutex_unlock(&landing_queue_mutex);

}
void *let_taking_offs(){
	pthread_mutex_lock(&taking_off_queue_mutex);
	while(taking_off_planes.size() >= 5){
		pthread_mutex_unlock(&taking_off_queue_mutex);
		let_emergencies();
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_taking_offs);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&taking_off_queue_mutex);
		if(taking_off_planes.front()->completed_action)
			taking_off_planes.pop();
		pthread_mutex_unlock(&taking_off_queue_mutex);
		let_one_to_land();
		pthread_mutex_lock(&taking_off_queue_mutex);
	}
	pthread_mutex_unlock(&taking_off_queue_mutex);
}
void *check_notifications(void *ptr){
	while(!simulation_finished){
		let_emergencies();
		let_landings();
		let_taking_offs();

	}
	return NULL;
}
void run_simulation(double simulation_time, double probability){
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	struct timeval emergency_counter = current_time;
	int seconds = current_time.tv_sec;
	cout<<current_time.tv_sec<<endl;
	double end_time = current_time.tv_sec + simulation_time;
	while(current_time.tv_sec < end_time){
		gettimeofday(&current_time,NULL);
		if(current_time.tv_sec-emergency_counter.tv_sec == 5){
			gettimeofday(&emergency_counter, NULL);
			create_emergency_plane();
		}
		else if(current_time.tv_sec - seconds == 1){
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
	pthread_mutex_init(&emergency_mutex, NULL);
	pthread_mutex_init(&taking_off_queue_mutex, NULL);
	pthread_cond_init(&runway_permission_landings, NULL);
	pthread_cond_init(&runway_permission_emergencies, NULL);
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