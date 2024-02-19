
all:
	gcc -c palloc.c
	gcc -c webserver.c
	gcc webserver.o palloc.o -pthread -o webserver

run:
	./webserver 8080 /home/tyh/syse/sysepallocpthread/web

clean:
	rm -rf ./*.o
	rm -rf ./webserver
	rm -rf ./web/nweb.log
	
.PHONY : clean

