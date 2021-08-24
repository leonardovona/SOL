#ifndef CUSTOMER_H_
#define CUSTOMER_H_


checkout_line* find_line(unsigned int seed);
void write_data(customer_data* cust_data);
checkout_line* check_change_line(checkout_line* line, customer_data* cust_data, unsigned int seed);
void no_products_exit(customer_data* cust_data);
void checkout(clock_t* time_in_queue, unsigned int seed, customer_data* cust_data);
void* customer(void *arg);

#endif /* CUSTOMER_H_ */
