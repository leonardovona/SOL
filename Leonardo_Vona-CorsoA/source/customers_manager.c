#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "./header/structures.h"
#include "./header/util.h"
#include "./header/customer.h"
#include "./header/customers_manager.h"

extern pthread_cond_t c_exit_request;
extern configuration* cfg;
extern int customers_in_supermarket;
extern pthread_mutex_t m_customers_in_supermarket;
extern volatile sig_atomic_t sigquit, sighup;
extern pthread_cond_t c_exit_customer;
extern pthread_mutex_t m_no_more_customers;
extern int no_more_customers;

void* customers_manager() {
	pthread_t *tid_customers;	//tids of customers threads
	int num_customers = cfg->C;

	tid_customers = calloc(sizeof(pthread_t), cfg->C);
	ec_null(tid_customers, "customers_manager: Allocating tid_customers");

	//initial customer creation
	ec_non0(pthread_mutex_lock(&m_customers_in_supermarket), "customers manager: acquiring lock on m_customers_in_supermarket");
	for (int i = 0; i < cfg->C; i++) {
		ec_non0(pthread_create(&tid_customers[i], NULL, &customer, (void*) (intptr_t) i), "customers manager: Unable to create customer");
		customers_in_supermarket++;
	}

	//new customer creation
	while (sighup == 0 && sigquit == 0) {
		while ((customers_in_supermarket >= cfg->C - cfg->E) && (sigquit == 0) && (sighup == 0)) {
			ec_non0(pthread_cond_wait(&c_exit_customer, &m_customers_in_supermarket), "customers manager: wait on c_exit_customer");
		}

		if (sighup == 1 || sigquit == 1) {
			break;
		}

		num_customers += cfg->E;
		tid_customers = realloc(tid_customers, sizeof(pthread_t) * num_customers);
		ec_null(tid_customers, "customers_manager: Reallocating tid_customers");

		for (int i = num_customers - cfg->E; i < num_customers; i++) {
			ec_non0(pthread_create(&tid_customers[i], NULL, &customer, (void*) (intptr_t) i), "customers manager: Unable to create customer");
			customers_in_supermarket++;
		}
	}

	ec_non0(pthread_mutex_unlock(&m_customers_in_supermarket), "customers manager: releasing lock on m_customers_in_supermarket");

	//join customers
	for (int i = 0; i < num_customers; i++) {
		pthread_join(tid_customers[i], NULL);
	}

	//notifies exit customers
	ec_non0(pthread_mutex_lock(&m_no_more_customers), "customers manager: acquiring lock on m_no_more_customers");
	no_more_customers = 1;
	ec_non0(pthread_mutex_unlock(&m_no_more_customers), "customers manager: releasing lock on m_no_more_customers");
	ec_non0(pthread_cond_signal(&c_exit_request), "customers manager: trying to signal on c_exit_request");

	free(tid_customers);
	pthread_exit(NULL);
}
