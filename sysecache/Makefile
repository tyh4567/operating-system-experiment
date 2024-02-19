all: 
	gcc -c cache.c
	gcc -c webserver.c
	gcc cache.o webserver.o -o webserver

run:
	./webserver 8080 /home/tyh/syse/sysecache/web
.PHONY : clean

clean:
	./kill.sh
	