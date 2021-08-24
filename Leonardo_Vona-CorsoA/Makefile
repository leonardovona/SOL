CC		=  gcc
CFLAGS  = -pthread -Wall -pedantic 

source_dir = ./source

objects = $(source_dir)/queue.o $(source_dir)/customer.o $(source_dir)/cashier.o $(source_dir)/customers_manager.o $(source_dir)/exit_management.o $(source_dir)/director.o \
$(source_dir)/main.o

.PHONY: test2 clean

all: $(objects) 
	$(CC) $(CFLAGS) $(objects) -o supermarket
	
main.o: $(source_dir)/main.c
	$(CC) $(CFLAGS) -c $(source_dir)/main.c
		
queue.o: $(source_dir)/queue.c $(source_dir)/header/queue.h
	$(CC) $(CFLAGS) -c $(source_dir)/queue.c
	
customer.o: $(source_dir)/customer.c $(source_dir)/header/customer.h
	$(CC) $(CFLAGS) -c $(source_dir)/customer.c
		
cashier.o: $(source_dir)/cashier.c $(source_dir)/header/cashier.h
	$(CC) $(CFLAGS) -c $(source_dir)/cashier.c
		
customers_manager.o: $(source_dir)/customers_manager.c $(source_dir)/header/customers_manager.h
	$(CC) $(CFLAGS) -c $(source_dir)/customers_manager.c		

exit_management.o: $(source_dir)/exit_management.c $(source_dir)/header/exit_management.h
	$(CC) $(CFLAGS) -c $(source_dir)/exit_management.c
	
director.o: $(source_dir)/director.c $(source_dir)/header/director.h
	$(CC) $(CFLAGS) -c $(source_dir)/director.c
	
test2:
	(./supermarket ./config.txt & echo $$! > supermarket.PID) &
	sleep 25s; \
	kill -1 $$(cat supermarket.PID); \
	tail --pid=$$(cat supermarket.PID) -f /dev/null;
	./analisi.sh ./log.txt; \
	
clean:
	rm -f $(source_dir)/*.o ./supermarket ./log.txt supermarket.PID

