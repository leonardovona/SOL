#ifndef STRUCTURES_H_
#define STRUCTURES_H_

#include "./queue.h"

typedef struct {
	int id;
	int closed;
	int tot_products;
	int served_customers;
	double open_time;
	int closures;
	pthread_cond_t c_cashier;
	pthread_cond_t c_customer;
	Queue_t* queue;
	pthread_mutex_t m_line;
} checkout_line;

typedef struct {
	double time_in_supermarket;
	double time_in_queue;
	int id;
	int products;
	int n_lines;
	int has_paid;
	pthread_mutex_t m_cust;
} customer_data;

typedef struct {
	int P;
	int T;
	int C;
	int K;
	int E;
	int S1;
	int S2;
	char* log_file;
} configuration;

#endif /* STRUCTURES_H_ */
