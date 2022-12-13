build: libscheduler.so

libscheduler.so: scheduler.o queue.o thread.o
	gcc -Wall -shared scheduler.o queue.o thread.o -o libscheduler.so

scheduler.o: scheduler.c
	gcc -Wall -fPIC -o scheduler.o -c -lpthread scheduler.c

thread.o: thread.c
	gcc -Wall -fPIC -o thread.o -c thread.c

queue.o: queue.c
	gcc -Wall -fPIC -o queue.o -c queue.c

.PHONY: clean

clean:
	rm -f *.o libscheduler.so
