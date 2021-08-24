#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#include "./header/structures.h"
#include "./header/director.h"
#include "./header/util.h"

#define FILELENGTH 256

volatile sig_atomic_t sighup = 0;
volatile sig_atomic_t sigquit = 0;

int customers_in_supermarket = 0;
pthread_mutex_t m_customers_in_supermarket = PTHREAD_MUTEX_INITIALIZER;

Queue_t* customers_to_exit;
pthread_mutex_t m_exit_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_exit_request = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_exit_granted = PTHREAD_COND_INITIALIZER;

pthread_mutex_t m_log = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t c_exit_customer = PTHREAD_COND_INITIALIZER;


pthread_mutex_t m_no_more_customers = PTHREAD_MUTEX_INITIALIZER;
int no_more_customers = 0;

checkout_line** checkout_lines;

configuration* cfg;

//creates log file
void init_log_file(){
	FILE *log_fp = fopen(cfg->log_file, "w");
	ec_null(log_fp, "Aprendo logfile in init_log_file");
	ec_non0(fclose(log_fp), "init_log_file: closing log file");
}

//writes checkout lines stats into log file
void write_log(){
	FILE *log_fp = fopen(cfg->log_file, "a");
	ec_null(log_fp, "Aprendo logfile in write_log");
	for(int i = 0; i < cfg->K; i++){
		if(fprintf(log_fp, "checkout_line;%d;%d;%d;%0.3f;%d\n", i,
													checkout_lines[i]->tot_products,
													checkout_lines[i]->served_customers,
													checkout_lines[i]->open_time,
													checkout_lines[i]->closures) < 0){
			fprintf(stderr, "write_log: writing on log file");
			exit(EXIT_FAILURE);
		}
	}
	ec_non0(fclose(log_fp), "write_log: closing log file");
}

//reads configuration from file and sets values
void configure(char* config){
	FILE *config_fp = fopen(config, "r");
	ec_null(config_fp, "configure: opening config file");
	if(fscanf(config_fp, "log file: %s K: %d C: %d E: %d P: %d T: %d S1: %d S2: %d", cfg->log_file, &cfg->K, &cfg->C, &cfg->E, &cfg->P, &cfg->T, &cfg->S1, &cfg->S2) != 8){
		fprintf(stderr, "configure: Error while scanning from configuration file\n");
		exit(EXIT_FAILURE);
	}
	ec_non0(fclose(config_fp), "configure: closing config file");
}

//sets global variables when receives sighup or sigquit
void signal_handler (int signum) {
	if (signum == SIGHUP) {
		sighup = 1;
		printf("Supermarket is closing with sighup\n");
	}
	if (signum == SIGQUIT) {
		sigquit = 1;
		printf("Supermarket is closing with sigquit\n");
	}
}

int main(int argc, char *argv[]) {
	printf("Supermarket initialization\n");
	pthread_t tid_direttore;
	sigset_t set;
	struct sigaction sa;

	if(argc != 2){
		fprintf(stderr,"main: No configuration file selected\n");
		exit(EXIT_FAILURE);
	}

	//ignore all signals before signal handler is set
	ec_meno1(sigfillset(&set), "main: Error on sigfillset");
	ec_meno1(pthread_sigmask(SIG_SETMASK,&set,NULL), "main: Error on pthread_sigmask");

	//sets signal handler
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = signal_handler;
	ec_meno1(sigaction(SIGHUP,&sa,NULL), "main: Error on sigaction");
	ec_meno1(sigaction(SIGQUIT,&sa,NULL), "main: Error on sigaction");
	ec_meno1(sigemptyset(&set), "main: Error on sigemptyset");
	ec_meno1(pthread_sigmask(SIG_SETMASK,&set,NULL), "main: Error on pthread_sigmask");

	cfg = malloc(sizeof(configuration));
	cfg->log_file = calloc(sizeof(char), FILELENGTH);

	configure(argv[1]);
	printf("Parameters configured\n");

	checkout_lines = calloc(sizeof(checkout_line*), cfg->K);
	ec_null(checkout_lines, "main: Allocating checkout_lines");

	//initializes checkout lines
	for(int i = 0; i < cfg->K; i++){
		checkout_lines[i] = malloc(sizeof(checkout_line));
		ec_null(checkout_lines[i], "main: Allocating checkout_lines");
		checkout_lines[i]->id = i;
		checkout_lines[i]->closed = 1;
		checkout_lines[i]->tot_products = 0;
		checkout_lines[i]->served_customers = 0;
		checkout_lines[i]->open_time = 0;
		checkout_lines[i]->closures = 0;
		pthread_cond_init(&checkout_lines[i]->c_customer, NULL);
		pthread_cond_init(&checkout_lines[i]->c_cashier, NULL);
		checkout_lines[i]->queue = initQueue();
		ec_null(checkout_lines[i]->queue, "main: Allocating checkout_lines queue");
		pthread_mutex_init(&checkout_lines[i]->m_line, NULL);
	}

	customers_to_exit = initQueue();
	init_log_file();

	ec_non0(pthread_create(&tid_direttore, NULL, &director, NULL), "main: Unable to create direttore");
	printf("Director initiated\n");
	pthread_join(tid_direttore, NULL);

	write_log();
	printf("Log written\n");
	for(int i = 0; i < cfg->K; i++){
		deleteQueue(checkout_lines[i]->queue);
		free(checkout_lines[i]);
	}
	free(cfg->log_file);
	free(cfg);
	deleteQueue(customers_to_exit);
	free(checkout_lines);
	printf("Supermarket closed\n");
	return 0;
}
