#ifndef DIRECTOR_H_
#define DIRECTOR_H_


void open_line(unsigned int seed, pthread_t * tid_cashiers);
void close_line(unsigned int seed, pthread_t * tid_cashiers);
void *director();

#endif /* DIRECTOR_H_ */
