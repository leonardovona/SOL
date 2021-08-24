#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "./header/structures.h"
#include "./header/util.h"
#include "./header/customer.h"

#define MIN_CUSTOMER 10
#define ITERATIONS_LIMIT 50

extern volatile sig_atomic_t sigquit, sighup;
extern checkout_line** checkout_lines;
extern configuration* cfg;

extern pthread_mutex_t m_customers_in_supermarket;
extern int customers_in_supermarket;

extern pthread_cond_t c_exit_customer;
extern pthread_cond_t c_exit_request;
extern pthread_cond_t c_exit_granted;

extern Queue_t* customers_to_exit;
extern pthread_mutex_t m_exit_queue;

extern pthread_mutex_t m_log;

//finds a checkout line
checkout_line* find_line(unsigned int seed){
	checkout_line* line;
	int index;
	int limit = 0;
	do{
		if(limit != 0){
			ec_non0(pthread_mutex_unlock(&line->m_line), "find_line: trying to release lock on m_line");
		}
		limit++;
		index = rand_r(&seed) % (cfg->K);
		line = checkout_lines[index];
		ec_non0(pthread_mutex_lock(&line->m_line), "find_line: trying to acquire lock on m_line");
	}while(line->closed != 0 && limit < ITERATIONS_LIMIT);
	if(limit > ITERATIONS_LIMIT){
		fprintf(stderr, "find_line: Unable to find line");
		pthread_exit(NULL);
	}
	ec_non0(pthread_mutex_unlock(&line->m_line), "find_line: trying to release lock on m_line");
	return line;
}

//writes customers stats into log file
void write_data(customer_data* cust_data){
	ec_non0(pthread_mutex_lock(&m_log), "write_data: trying to acquire lock on m_log");
	FILE *log_fp = fopen(cfg->log_file, "a");
	ec_null(log_fp, "write_data: opening logfile");
	fprintf(log_fp, "customer;%d;%d;%0.3f;%0.3f;%d\n", cust_data->id,
													cust_data->products,
													cust_data->time_in_supermarket,
													cust_data->time_in_queue,
													cust_data->n_lines);
	fclose(log_fp);
	ec_non0(pthread_mutex_unlock(&m_log), "write_data: trying to release lock on m_log");
}

//checks if the checkout line is closed
checkout_line* check_change_line(checkout_line* line, customer_data* cust_data, unsigned int seed){
	ec_non0(pthread_mutex_lock(&line->m_line), "check_change_line: trying to acquire lock on m_line");
	if(line->closed == 1){
		ec_non0(pthread_mutex_unlock(&line->m_line), "check_change_line: trying to release lock on m_line");
		line = find_line(seed);
		ec_non0(pthread_mutex_lock(&line->m_line), "check_change_line: trying to acquire lock on m_line");
		if (push(line->queue, cust_data) == -1) {
			fprintf(stderr, "Errore: push\n");
			pthread_exit(NULL);
		}
		cust_data->n_lines++;
	}
	ec_non0(pthread_mutex_unlock(&line->m_line), "check_change_line: trying to release lock on m_line");
	return line;
}

//waits the director to exit without products
void no_products_exit(customer_data* cust_data){
	ec_non0(pthread_mutex_lock(&m_exit_queue), "no_products_exit: trying to acquire lock on m_exit_queue");
	if (push(customers_to_exit, cust_data) == -1) {
		fprintf(stderr, "Errore: push\n");
		pthread_exit(NULL);
	}
	ec_non0(pthread_mutex_unlock(&m_exit_queue), "no_products_exit: trying to release lock on m_exit_queue");
	ec_non0(pthread_mutex_lock(&cust_data->m_cust), "no_products_exit: trying to acquire lock on m_cust");
	pthread_cond_signal(&c_exit_request);
	while((cust_data->has_paid == 0) && (sigquit == 0)){
		ec_non0(pthread_cond_wait(&c_exit_granted, &cust_data->m_cust), "customer: wait on c_exit_granted");
	}
	ec_non0(pthread_mutex_unlock(&cust_data->m_cust), "no_products_exit: trying to release lock on m_cust");
}

//finds a checkout line and pays
void checkout(clock_t* time_in_queue, unsigned int seed, customer_data* cust_data){
	checkout_line* line;
	*time_in_queue = clock();
	line = find_line(seed);
	ec_non0(pthread_mutex_lock(&line->m_line), "checkout: trying to acquire lock on m_line");

	if (push(line->queue, cust_data) == -1) {
		fprintf(stderr, "Errore: push\n");
		pthread_exit(NULL);
	}
	ec_non0(pthread_mutex_unlock(&line->m_line), "checkout: trying to release lock on m_line");
	ec_non0(pthread_mutex_lock(&cust_data->m_cust), "checkout: trying to acquire lock on m_cust");
	cust_data->n_lines++;

	ec_non0(pthread_cond_signal(&line->c_cashier), "checkout: trying to signal on c_cashier");

	while((cust_data->has_paid == 0) && (sigquit == 0)){
		line = check_change_line(line, cust_data, seed);
		pthread_cond_wait(&line->c_customer, &cust_data->m_cust);
	}
	ec_non0(pthread_mutex_unlock(&cust_data->m_cust), "checkout: trying to release lock on m_cust");
}

void* customer(void *arg) {
	unsigned int seed = time(NULL) ^ getpid() ^ pthread_self();
	int products = rand_r(&seed)%(cfg->P);

	//time spent on picking products
	struct timespec picking_time;
	int t = rand_r(&seed)%(cfg->T - MIN_CUSTOMER) + MIN_CUSTOMER;
	picking_time.tv_sec = t / 1000;
	picking_time.tv_nsec = (t - picking_time.tv_sec*1000) * 1000000;

	clock_t time_in_supermarket, time_in_queue = 0;

	//initializes customer statistics
	customer_data* cust_data;
	cust_data = malloc(sizeof(customer_data));
	cust_data->id = (int) (intptr_t) arg;
	cust_data->products = products;
	cust_data->n_lines = 0;
	cust_data->has_paid = 0;
	cust_data->time_in_queue = 0;
	pthread_mutex_init(&cust_data->m_cust, NULL);

	//picking products
	time_in_supermarket = clock();
	if(nanosleep(&picking_time, &picking_time) == 1){
		if(sighup == 1){
			nanosleep(&picking_time, NULL);
		}
		if(sigquit == 1){
			time_in_supermarket = clock() - time_in_supermarket;
			cust_data->time_in_supermarket = ((double)time_in_supermarket)/CLOCKS_PER_SEC;
			cust_data->time_in_queue = 0;
			write_data(cust_data);
			pthread_exit(NULL);
		}
	}

	if(products == 0){
		no_products_exit(cust_data);
	} else{
		checkout(&time_in_queue, seed, cust_data);
	}

	//signals the exit of the customer from the supermarket
	ec_non0(pthread_mutex_lock(&m_customers_in_supermarket), "customer: trying to acquire lock on m_customers_in_supermarket");
	customers_in_supermarket--;
	ec_non0(pthread_mutex_unlock(&m_customers_in_supermarket), "customer: trying to release lock on m_customers_in_supermarket");
	ec_non0(pthread_cond_signal(&c_exit_customer), "customer: trying to signal on c_exit_customer");

	time_in_supermarket = clock() - time_in_supermarket;
	cust_data->time_in_supermarket = ((double)time_in_supermarket)/CLOCKS_PER_SEC;
	if(!sigquit){
		time_in_queue = cust_data->time_in_queue - time_in_queue;
		cust_data->time_in_queue = ((double)time_in_queue)/CLOCKS_PER_SEC;
	}

	write_data(cust_data);
	free(cust_data);
	pthread_exit(NULL);
}
