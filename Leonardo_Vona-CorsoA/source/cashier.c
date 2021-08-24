#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "./header/structures.h"
#include "./header/util.h"
#include "./header/cashier.h"

#define CASHIER_MIN 20
#define CASHIER_MAX 80
#define PRODUCT_TIME 5

extern volatile sig_atomic_t sigquit;

void* cashier(void* arg){

	checkout_line *line = arg; //assigned checkout line
	unsigned int seed = time(NULL) ^ getpid() ^ pthread_self();
	int cashier_time = rand_r(&seed)%(CASHIER_MAX - CASHIER_MIN) + CASHIER_MIN;
	struct timespec products_time;
	customer_data* cust;
	clock_t t;
	clock_t open_time;
	ec_non0(pthread_mutex_lock(&line->m_line), "cashier: Trying to acquire lock on m_line");
	line->closed = 0;
	ec_meno1(open_time = clock(), "cashier: Unable to retrieve clock");

	while(line->closed != 1 && sigquit == 0){
		while((length(line->queue) == 0) && (sigquit == 0) && (line->closed == 0)){
			ec_non0(pthread_cond_wait(&line->c_cashier, &line->m_line), "cashier: wait on c_cashier");
		}

		if(sigquit == 1){
			break;
		}

		if(length(line->queue) > 0){
			cust = pop(line->queue);

			ec_non0(pthread_mutex_unlock(&line->m_line), "cashier: trying to release lock on m_line");

			ec_non0(pthread_mutex_lock(&cust->m_cust), "cashier: trying to acquire lock on m_cust");
			ec_meno1(cust->time_in_queue = clock(), "cashier: Unable to retrieve clock");
			products_time.tv_sec = (cashier_time + cust->products*PRODUCT_TIME) / 1000;
			products_time.tv_nsec = ((cashier_time + cust->products*PRODUCT_TIME) - products_time.tv_sec*1000) * 1000000;

			nanosleep(&products_time, NULL);

			if(sigquit == 1){
				ec_meno1(cust->time_in_queue = clock(), "cashier: Unable to retrieve clock");
				ec_non0(pthread_mutex_unlock(&cust->m_cust), "cashier: trying to release lock on m_cust");
				break;
			}

			ec_non0(pthread_mutex_lock(&line->m_line), "cashier: trying to acquire lock on m_line");

			cust->has_paid = 1;
			line->tot_products += cust->products;
			ec_non0(pthread_mutex_unlock(&cust->m_cust), "cashier: trying to release lock on m_cust");

			line->served_customers++;

			ec_non0(pthread_cond_broadcast(&line->c_customer), "cashier: trying to broadcast on c_customer");
		}
	}



	//reinit queue
	deleteQueue(line->queue);
	line->queue = initQueue();

	ec_meno1(t = clock(), "cashier: Unable to retrieve clock");
	open_time = t - open_time;
	line->open_time += ((double)open_time)/CLOCKS_PER_SEC;
	line->closures++;
	ec_non0(pthread_mutex_unlock(&line->m_line), "cashier: trying to release lock on m_line");
	ec_non0(pthread_cond_broadcast(&line->c_customer), "cashier: trying to broadcast on c_customer");
	pthread_exit(NULL);
}
