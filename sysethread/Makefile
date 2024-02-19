
all: webserver.c
	gcc -std=gnu99 -o webserver webserver.c -pthread 

run:
	./webserver 8080 /home/tyh/syse/sysethread/web

.PHONY : clean
clean:
	./kill.sh

