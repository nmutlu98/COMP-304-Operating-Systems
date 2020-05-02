#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include<iostream>
#include<vector>
#include<cstdlib>
#include<string.h>
#include<queue>
#include<fstream>
#include<ctime>
#define filename "planes.log"
#define EMERGENCY_INTERVAL 40
using namespace std;
pthread_t tower;
int landing_plane_counter = 0;
int departing_plane_counter = 1;
pthread_cond_t runway_permission_landings; //a new landing plane waits on this cond until it is signalled by Traffic Controller(TC) 
pthread_cond_t runway_permission_taking_offs;//a new departing plane waits on this cond until it is signalled by TC
pthread_cond_t runway_permission_emergencies;//a new emergency plane waits on this cond until it is signalled by TC
pthread_mutex_t runway_mutex; //the plane that has the permission will hold this mutex to use runway
pthread_mutex_t landing_queue_mutex;//controls the access to landing planes queue
pthread_mutex_t taking_off_queue_mutex;//controls the access to departing planes queue
pthread_mutex_t landing_counting_mutex;//controls the access to landing_plane_counter which is used as id for landings
pthread_mutex_t departing_counting_mutex;//controls the access to landing_plane_counter which is used as id for departings
pthread_mutex_t emergency_mutex;//controls the access to the emergency planes queue
pthread_mutex_t all_planes_mutex;
pthread_t reporting_thread;//reports the planes on air and ground
struct timeval start_time;//start time of simulation
bool simulation_finished = false;
bool time_came = false;//false if the program didnt reach the  nth second, true otherwise
int n;//commandline argument for reporting time
ofstream logfile;//logfile stream
struct plane{
	pthread_t plane_thread;
	bool is_landing;
	bool is_emergency = false;
	int plane_id;
	bool completed_action = false;
	struct timeval* request_time;
	struct timeval* runway_time;
};
queue<struct plane*> landing_planes;
queue<struct plane*> taking_off_planes;
queue<struct plane*> emergency;
queue<struct plane*> all_planes;
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
/************************************************
 * generates a random probability 
 *************************************************/
double random_generator(){ 
	return (double)(rand() % 100)/100;
}
/**************************************************
 * When a new emergency plane arrives, it runs this function to add itself to emergency queue
 * and wait for completing its action until signalled by the traffic controller
 **************************************************/
void* notify_emergency(void* plane){
	struct plane* urgent_plane = (struct plane*) plane;
	pthread_mutex_lock(&emergency_mutex);
	emergency.push(urgent_plane);
	gettimeofday(urgent_plane->request_time, NULL);
	pthread_mutex_unlock(&emergency_mutex);
	int id = urgent_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_emergencies, &runway_mutex);
	while(emergency.front()->plane_id != urgent_plane->plane_id){ //if it is not the first plane in the queue it continues to wait
		pthread_cond_wait(&runway_permission_emergencies, &runway_mutex);
	}
	urgent_plane->completed_action = true;
	pthread_mutex_unlock(&runway_mutex);
}
/****************************************************
 * When a new landing plane comes, it runs this function to add itself to landing queue and 
 * waits until signalled by the traffic controller to complete its action
 ****************************************************/
void* notify_tower_landing(void* plane){
	
	struct plane* landing_plane = (struct plane*) plane;
	pthread_mutex_lock(&landing_queue_mutex);
	landing_planes.push(landing_plane);
	gettimeofday(landing_plane->request_time, NULL);
	pthread_mutex_unlock(&landing_queue_mutex);
	int id = landing_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_landings, &runway_mutex);
	while(landing_planes.front()->plane_id != landing_plane->plane_id){//if it is not the first plane in the queue, it waits
		pthread_cond_wait(&runway_permission_landings, &runway_mutex);
	}
	landing_plane->completed_action = true;
	pthread_mutex_unlock(&runway_mutex);
}
/****************************************************
 * When a new departing plane comes, it runs this function to add itself to taking offs queue and 
 * waits until signalled by the traffic controller to complete its action
 ****************************************************/
void* notify_tower_taking_off(void* plane){
	
	struct plane* taking_off_plane = (struct plane*) plane;
	pthread_mutex_lock(&taking_off_queue_mutex);
	taking_off_planes.push(taking_off_plane);
	gettimeofday(taking_off_plane->request_time, NULL);
	pthread_mutex_unlock(&taking_off_queue_mutex);
	int id = taking_off_plane->plane_id;
	pthread_mutex_lock(&runway_mutex);
	pthread_cond_wait(&runway_permission_taking_offs, &runway_mutex);
	while(taking_off_planes.front()->plane_id != taking_off_plane->plane_id)//if it is not the first plane in the queue, it waits
		pthread_cond_wait(&runway_permission_taking_offs, &runway_mutex);
	taking_off_plane->completed_action = true;
	pthread_mutex_unlock(&runway_mutex);
	return NULL;
} 
/****************************************
 * Given a probability p, it produces a departing plane with the probability 1-p
 * or a landing plane with probability p
 ****************************************/
void *create_plane(double p){
	struct plane* new_plane = (struct plane*)malloc(sizeof(struct plane*));
	new_plane->plane_thread = (pthread_t)malloc(sizeof(pthread_t));
	new_plane->request_time = (struct timeval*)malloc(sizeof(struct timeval*));
	new_plane->runway_time = (struct timeval*)malloc(sizeof(struct timeval*));
	pthread_mutex_lock(&all_planes_mutex);
	all_planes.push(new_plane);
	pthread_mutex_unlock(&all_planes_mutex);
	double chosen_p = random_generator();
	if(chosen_p > p){ 
		pthread_t taking_off;
		pthread_mutex_lock(&departing_counting_mutex);
		new_plane->plane_id = departing_plane_counter;
		departing_plane_counter += 2;
		pthread_mutex_unlock(&departing_counting_mutex),
		new_plane->is_landing = false;
		new_plane->plane_thread = taking_off;
		pthread_create(&taking_off, NULL, notify_tower_taking_off,(void*)new_plane);
	}else{// p
		pthread_t landing;
		pthread_mutex_lock(&landing_counting_mutex);
		new_plane->plane_id = landing_plane_counter;
		landing_plane_counter += 2;
		pthread_mutex_unlock(&landing_counting_mutex);
		new_plane->is_landing = true;
		new_plane->plane_thread = landing;
		pthread_create(&landing, NULL, notify_tower_landing, (void*)new_plane);
	}
	return NULL;
}
/*************************************
 * Creates an emergency plane when called
 *************************************/
void *create_emergency_plane(){
	pthread_t urgent_thread;
	struct plane* new_plane = (struct plane*)malloc(sizeof(struct plane*));
	new_plane->plane_thread = (pthread_t)malloc(sizeof(pthread_t));
	new_plane->request_time = (struct timeval*)malloc(sizeof(struct timeval*));
	new_plane->runway_time = (struct timeval*)malloc(sizeof(struct timeval*));
	new_plane->is_emergency = true;
	pthread_mutex_lock(&all_planes_mutex);
	all_planes.push(new_plane);
	pthread_mutex_unlock(&all_planes_mutex);
	pthread_mutex_lock(&landing_counting_mutex);
	new_plane->plane_id = landing_plane_counter;
	landing_plane_counter += 2;
	pthread_mutex_unlock(&landing_counting_mutex);
	new_plane->plane_thread = urgent_thread;
	pthread_create(&urgent_thread, NULL, notify_emergency, (void*)new_plane); 
}
/*******************************************
 gives runway permission to emergency planes until the emergency queue becomes empty
 *******************************************/
void *let_emergencies(){
	pthread_mutex_lock(&emergency_mutex);
	while(emergency.size() != 0){
		pthread_mutex_unlock(&emergency_mutex);
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_emergencies);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&emergency_mutex);
		if(emergency.front()->completed_action){
			gettimeofday(emergency.front()->runway_time, NULL);
			emergency.pop();
		}
		pthread_mutex_unlock(&emergency_mutex);
	}
	pthread_mutex_unlock(&emergency_mutex);
}
/******************************************************
 Gives permission to one of the planes in taking off queue to use the runway
 when the number of planes waiting in the taking off queue is less than 5
 ******************************************************/
void *let_one_to_take_off(){
	pthread_mutex_lock(&taking_off_queue_mutex);
	if(taking_off_planes.size() != 0 && taking_off_planes.size() < 5){
		pthread_mutex_unlock(&taking_off_queue_mutex);
		let_emergencies();//gives permissions to emergencies if there are some in the queue
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_taking_offs);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&taking_off_queue_mutex);
		if(taking_off_planes.front()->completed_action){
			gettimeofday(taking_off_planes.front()->runway_time, NULL);
			taking_off_planes.pop();
			}
		pthread_mutex_unlock(&taking_off_queue_mutex);
	}
	pthread_mutex_unlock(&taking_off_queue_mutex);
}
/************************************************************
 * When the number of waiting departing planes is less than 5, this function gives permission to all
 * landings one by one by one and when there is no planes in the landings queueu, it lets one departing plane
 * to use the runway
 ************************************************************/
void *let_landings(){
	pthread_mutex_lock(&landing_queue_mutex);
	pthread_mutex_lock(&taking_off_queue_mutex);
	while(taking_off_planes.size()<5 && landing_planes.size() != 0){ 
		pthread_mutex_unlock(&taking_off_queue_mutex);
		pthread_mutex_unlock(&landing_queue_mutex);
		let_emergencies(); //also it lets emergencies if there are some in the emergency queueu
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_landings);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&landing_queue_mutex);
		if(landing_planes.front()->completed_action){
			gettimeofday(landing_planes.front()->runway_time, NULL);
			landing_planes.pop();
		}
		pthread_mutex_lock(&taking_off_queue_mutex);
		}
		pthread_mutex_unlock(&taking_off_queue_mutex);
		pthread_mutex_unlock(&landing_queue_mutex);	
		let_one_to_take_off();
}
/**************************************************
 * Gives permission to one of the landing planes in the queue to use the runway 
 **************************************************/
void *let_one_to_land(){
	pthread_mutex_lock(&landing_queue_mutex);
	if(landing_planes.size() != 0){
		pthread_mutex_unlock(&landing_queue_mutex);
		let_emergencies();//lets emergencies if there are some in the waiting queueu
		pthread_mutex_lock(&runway_mutex);
		pthread_cond_broadcast(&runway_permission_landings);
		pthread_mutex_unlock(&runway_mutex);
		pthread_sleep(2);
		pthread_mutex_lock(&landing_queue_mutex);
		if(landing_planes.front()->completed_action){
			gettimeofday(landing_planes.front()->runway_time, NULL);
			landing_planes.pop();
			}
		pthread_mutex_unlock(&landing_queue_mutex);
	}
	pthread_mutex_unlock(&landing_queue_mutex);

}
/**********************************************
 * Gives permissions to departing planes if there are 5 or more waiting in the taking off queueu
 * To avoid starvation in landing planes after letting one taking off plane, it lets a landing plane
 * to use the runway. Everytime it gives permission to emergency planes to use the runway first.
 **********************************************/
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
		if(taking_off_planes.front()->completed_action){
			gettimeofday(taking_off_planes.front()->runway_time, NULL);
			taking_off_planes.pop();
			}
		pthread_mutex_unlock(&taking_off_queue_mutex);
		let_one_to_land();
		pthread_mutex_lock(&taking_off_queue_mutex);
	}
	pthread_mutex_unlock(&taking_off_queue_mutex);
}
/*********************************************************************
 * Traffic controller thread runs this function during the simulation to schedule the planes
 *********************************************************************/
void *check_notifications(void *ptr){
	while(!simulation_finished){
		let_emergencies();
		let_landings();
		let_taking_offs();

	}
	return NULL;
}
/*************************************************************
 * Starting from the nth second, it prints the planes waiting in the landing queueu 
 * and in the taking off queueu for each second
 ************************************************************/
void* report_planes(void* ptr){
	struct timeval last_update;
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	while(current_time.tv_sec-start_time.tv_sec < n){
		gettimeofday(&current_time, NULL);
	}
	last_update = current_time;
	while(!simulation_finished){
		queue<struct plane*> landings;
		queue<struct plane*>emergencies;
		queue<struct plane*> departings;
		if(current_time.tv_sec-last_update.tv_sec == 1){
			last_update = current_time;
			pthread_mutex_lock(&landing_queue_mutex);
			landings = landing_planes;
			pthread_mutex_unlock(&landing_queue_mutex);
			pthread_mutex_lock(&emergency_mutex);
			emergencies = emergency;
			pthread_mutex_unlock(&emergency_mutex);
			pthread_mutex_lock(&taking_off_queue_mutex);
			departings = taking_off_planes;
			pthread_mutex_unlock(&taking_off_queue_mutex);
			cout<<"---At "<<current_time.tv_sec-start_time.tv_sec<<" sec---"<<endl;
			cout<<"Air: ";
			while(!landings.empty()){
				cout<<landings.front()->plane_id<<" ";
				landings.pop();
			}
			while(!emergencies.empty()){
				cout<<emergencies.front()->plane_id<<" ";
				emergencies.pop();
			}
			cout<<endl;
			cout<<"Ground: ";
			while(!departings.empty()){
				cout<<departings.front()->plane_id<<" ";
				departings.pop();
			}
			cout<<endl;
		
		}
		gettimeofday(&current_time, NULL);
	}
}
/************************************************
 * Creates the planes and reporting thread given the simulation time
 * and landing plane probability
 ************************************************/
void run_simulation(double simulation_time, double probability){
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	start_time = current_time;
	pthread_create(&reporting_thread, NULL, report_planes, NULL);
	struct timeval emergency_counter = current_time;
	int seconds = current_time.tv_sec;
	double end_time = current_time.tv_sec + simulation_time;
	while(current_time.tv_sec < end_time){
		gettimeofday(&current_time,NULL);
		if(current_time.tv_sec-emergency_counter.tv_sec == EMERGENCY_INTERVAL){
			gettimeofday(&emergency_counter, NULL);
			create_emergency_plane();
		}
		else if(current_time.tv_sec - seconds == 1){
			seconds++;
			create_plane(probability);
		}
		
	}
	simulation_finished = true;
	while(!all_planes.empty()){ //prints log file when the simulation finishes
		struct plane* target = all_planes.front();
		char status;
		if(target->is_emergency)
			status = 'E';
		else if(target->is_landing)
			status = 'L';
		else
			status = 'D';
		if(target->completed_action)
			logfile<<"Plane id: "<<target->plane_id<<endl
					<<"Status: "<<status<<endl
					<<"Request time: "<<(target->request_time->tv_sec-start_time.tv_sec)<<endl
					<<"Runway Time: "<<target->runway_time->tv_sec-start_time.tv_sec<<endl
					<<"Turnaround Time: "<<target->runway_time->tv_sec-target->request_time->tv_sec<<endl<<endl;
		else
			logfile<<"Plane id: "<<target->plane_id<<endl
					<<"Status: "<<status<<endl
					<<"Request time: "<<(target->request_time->tv_sec-start_time.tv_sec)<<endl<<endl;
		all_planes.pop();
	}
}

int main(int argc, char* argv[]){
	srand(time(0));
	logfile.open(filename);
	pthread_create(&tower, NULL, check_notifications, NULL);
	pthread_mutex_init(&runway_mutex, NULL);
	pthread_mutex_init(&landing_counting_mutex, NULL);
	pthread_mutex_init(&departing_counting_mutex, NULL);
	pthread_mutex_init(&landing_queue_mutex, NULL);
	pthread_mutex_init(&emergency_mutex, NULL);
	pthread_mutex_init(&taking_off_queue_mutex, NULL);
	pthread_cond_init(&runway_permission_landings, NULL);
	pthread_cond_init(&runway_permission_emergencies, NULL);
	pthread_cond_init(&runway_permission_taking_offs, NULL);
	create_plane(1);//Initial landing plane
	create_plane(0);//Initial taking off planem
	cout<<"Running simulation..."<<endl;
	if(strcmp("-s", argv[1]) == 0 && strcmp("-p", argv[3]) == 0 && strcmp("-n", argv[5]) == 0){
		n = atof(argv[6]);
		run_simulation(atof(argv[2]), atof(argv[4]));
	}else if(strcmp("-p", argv[1]) == 0 && strcmp("-s", argv[3]) == 0 && strcmp("-n", argv[5]) == 0){
		n = atof(argv[6]);
		run_simulation( atof(argv[4]), atof(argv[2]));
	}else{
		cout<<"INVALID ARGUMENT"<<endl;
		return NULL;
	}
/*	if(simulation_finished){
		pthread_t printer;
		pthread_create(&printer, NULL, print_remaining_planes, NULL);
	}*/
	return 0;
}