#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include "./header/cashier.h"

#include "./header/director.h"
#include "./header/util.h"
#include "./header/structures.h"
#include "./header/exit_management.h"
#include "./header/customers_manager.h"

#define ITERATIONS_LIMIT 30
#define USLEEP_TIME 250000

extern pthread_cond_t c_exit_request;
extern pthread_cond_t c_exit_customer;
extern checkout_line** checkout_lines;
extern configuration* cfg;
extern volatile sig_atomic_t sigquit, sighup;

void open_line(unsigned int seed, pthread_t * tid_cashiers){
	int index = 0;
	checkout_line* line;
	int limit = 0;
	//loop to find a line to open
	do{
		if(limit > 0){
			ec_non0(pthread_mutex_unlock(&line->m_line), "open_line: trying to release lock on m_line");
		}
		index = rand_r(&seed) % (cfg->K);
		line = checkout_lines[index];
		limit++;
		ec_non0(pthread_mutex_lock(&line->m_line), "open_line: trying to acquire lock on m_line");
	}while(line->closed == 0 && limit <= ITERATIONS_LIMIT);

	if(limit <= ITERATIONS_LIMIT){
		line->closed = 0;
		ec_non0(pthread_create(&tid_cashiers[index], NULL, &cashier, line), "open_line: Unable to create cassiere");
	}
	ec_non0(pthread_mutex_unlock(&line->m_line), "open_line: trying to release lock on m_line");
}

void close_line(unsigned int seed, pthread_t * tid_cashiers){
	int index;
	int first_time = 1;
	checkout_line* line;
	//loop to find a line to close
	do{
		if(!first_time){
			ec_non0(pthread_mutex_unlock(&line->m_line), "close_line: trying to release lock on m_line");
		}
		first_time = 0;
		index = rand_r(&seed) % (cfg->K);
		line = checkout_lines[index];
		ec_non0(pthread_mutex_lock(&line->m_line), "close_line: trying to acquire lock on m_line");
	}while(line->closed != 0);
	line->closed = 1;
	ec_non0(pthread_mutex_unlock(&line->m_line), "close_line: trying to release lock on m_line");
	ec_non0(pthread_cond_signal(&line->c_cashier), "close_line: trying to broadcast on c_cashier");
	pthread_join(tid_cashiers[index], NULL);
}

void *director() {
	pthread_t tid_customers_manager;
	pthread_t tid_exit_management;
	pthread_t * tid_cashiers;
	int open_lines = 0;
	unsigned int seed = time(NULL) ^ getpid() ^ pthread_self();
	int q_length = 0;
	int customers_in_queue = 0;
	int line_opened = 0;

	tid_cashiers = calloc(sizeof(pthread_t*), cfg->K);

	//thread that allow the customers without products to exit
	ec_non0(pthread_create(&tid_exit_management, NULL, &exit_management, NULL), "director: Unable to create exit_management");

	open_line(seed, tid_cashiers);
	open_lines++;

	sleep(1);

	//thread witch creates new customers
	ec_non0(pthread_create(&tid_customers_manager, NULL, &customers_manager, NULL), "director: Unable to create customers_manager");
	printf("Customers entering into supermarket\n");

	//loop to open and close checkout lines
	while(sighup == 0 && sigquit == 0){
		usleep(USLEEP_TIME);
		line_opened = 0;
		customers_in_queue = 0;

		//check if a checkout line must be opened
		for(int i = 0; i < cfg->K; i++){
			ec_non0(pthread_mutex_lock(&checkout_lines[i]->m_line), "director: trying to acquire lock on m_line");
			q_length = length(checkout_lines[i]->queue);
			customers_in_queue += q_length;
			if((q_length > cfg->S1) && (open_lines < cfg->K) && (line_opened == 0)){
				ec_non0(pthread_mutex_unlock(&checkout_lines[i]->m_line), "director: trying to release lock on m_line");
				open_line(seed, tid_cashiers);
				open_lines++;
				line_opened = 1;
			}
			ec_non0(pthread_mutex_unlock(&checkout_lines[i]->m_line), "director: trying to release lock on m_line");
		}

		usleep(USLEEP_TIME);

		//check if a checkout line must be closed
		if((customers_in_queue < cfg->S2) && (open_lines > 1)){
			close_line(seed, tid_cashiers);
			open_lines--;
		}
	}

	//signal other threads in case of sigquit
	if(sigquit){
		ec_non0(pthread_cond_signal(&c_exit_customer), "director: trying to signal on c_exit_customer");
		ec_non0(pthread_cond_signal(&c_exit_request), "director: trying to signal on c_exit_request");
		for(int i = 0; i < cfg->K; i++){
			if(checkout_lines[i]->closed == 0){
				ec_non0(pthread_cond_signal(&checkout_lines[i]->c_cashier), "director: trying to signal on c_cashier");
			}
		}
	}

	//signal other threads in case of sighup
	if(sighup){
		ec_non0(pthread_cond_signal(&c_exit_request), "director: trying to signal on c_exit_request");
		ec_non0(pthread_cond_signal(&c_exit_customer), "director: trying to signal on c_exit_customer");
	}

	//reset open lines
	open_lines = 0;
	for(int i = 0; i < cfg->K; i++){
		ec_non0(pthread_mutex_lock(&checkout_lines[i]->m_line), "director: trying to acquire lock on m_line");
		if(!checkout_lines[i]->closed){
			open_lines++;
		}
		ec_non0(pthread_mutex_unlock(&checkout_lines[i]->m_line), "director: trying to release lock on m_line");
	}

	//loop in case of sighup to close all checkout lines still open
	while(sighup && ((customers_in_queue > 0) || (open_lines > 0))){
		usleep(USLEEP_TIME);
		customers_in_queue = 0;

		for(int i = 0; i < cfg->K; i++){
			ec_non0(pthread_mutex_lock(&checkout_lines[i]->m_line), "director: trying to acquire lock on m_line");
			q_length = length(checkout_lines[i]->queue);
			//check if a checkout line has no customers in queue
			if(q_length == 0 && (checkout_lines[i]->closed == 0)){
				checkout_lines[i]->closed = 1;
				open_lines--;
				ec_non0(pthread_cond_signal(&checkout_lines[i]->c_cashier), "close_line: trying to signal on c_cashier");
			}
			ec_non0(pthread_mutex_unlock(&checkout_lines[i]->m_line), "director: trying to release lock on m_line");
			customers_in_queue += q_length;
		}
	}

	//joins
	pthread_join(tid_customers_manager, NULL);
	printf("No more customers can enter into supermarket\n");
	for(int i = 0; i < cfg->K; i++){
		if(checkout_lines[i]->closed == 0){
			pthread_join(tid_cashiers[i], NULL);
		}
	}
	printf("Checkout lines are closed\n");
	pthread_join(tid_exit_management, NULL);

	free(tid_cashiers);
	pthread_exit(NULL);
}
