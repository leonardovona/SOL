#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "./header/structures.h"
#include "./header/util.h"
//#include "./header/customer.h"
#include "./header/exit_management.h"

extern volatile sig_atomic_t sigquit;

extern pthread_mutex_t m_no_more_customers;
extern int no_more_customers;

extern pthread_cond_t c_exit_request;
extern pthread_cond_t c_exit_granted;

extern pthread_mutex_t m_exit_queue;
extern Queue_t* customers_to_exit;

void *exit_management(){
	customer_data* cust;
	ec_non0(pthread_mutex_lock(&m_exit_queue), "exit_management: trying to acquire lock on m_exit_queue");
	ec_non0(pthread_mutex_lock(&m_no_more_customers), "exit_management: trying to acquire lock on m_no_more_customers");

	while(sigquit == 0 && no_more_customers == 0){
		//waits until a customer without products wants to exit
		while((length(customers_to_exit) == 0) && (sigquit == 0) && (no_more_customers == 0)){
			ec_non0(pthread_mutex_unlock(&m_no_more_customers), "exit_management: trying to release lock on m_no_more_customers");
			ec_non0(pthread_cond_wait(&c_exit_request, &m_exit_queue), "exit_management: trying to wait on m_exit_queue");
			ec_non0(pthread_mutex_lock(&m_no_more_customers), "exit_management: trying to acquire lock on m_no_more_customers");
		}

		if(no_more_customers){
			break;
		}

		ec_non0(pthread_mutex_unlock(&m_no_more_customers), "exit_management: trying to release lock on m_no_more_customers");

		if(sigquit){
			break;
		}

		cust = pop(customers_to_exit);
		ec_non0(pthread_mutex_unlock(&m_exit_queue), "exit_management: trying to release lock on m_exit_queue");

		ec_non0(pthread_mutex_lock(&cust->m_cust), "exit_management: trying to acquire lock on m_cust");
		cust->has_paid = 1;
		ec_non0(pthread_mutex_unlock(&cust->m_cust), "exit_management: trying to release lock on m_cust");
		ec_non0(pthread_cond_broadcast(&c_exit_granted), "exit_management: trying to broadcast c_exit_granted");
		ec_non0(pthread_mutex_lock(&m_exit_queue), "exit_management: trying to acquire lock on m_exit_queue");
		ec_non0(pthread_mutex_lock(&m_no_more_customers), "exit_management: trying to acquire lock on m_no_more_customers");
	}
	ec_non0(pthread_mutex_unlock(&m_no_more_customers), "exit_management: trying to release lock on m_no_more_customers");
	ec_non0(pthread_mutex_unlock(&m_exit_queue), "exit_management: trying to release lock on m_exit_queue");
	ec_non0(pthread_cond_broadcast(&c_exit_granted), "exit_management: trying to broadcast c_exit_granted");
	pthread_exit(NULL);
}
