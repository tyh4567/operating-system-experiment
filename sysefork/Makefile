
CC = gcc
CFLAGS = -lpthread

TARGET  = webserver
OBJ_DIR = ./obj
SRC_DIR = ./src
 
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, %.o, $(SRC))
 
VPATH = $(SRC_DIR)
vpath %.o $(OBJ_DIR)
 
all: $(TARGET)
 
$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(addprefix $(OBJ_DIR)/, $(OBJECTS)) $(CFLAGS)
 
%.o : %.c
	$(CC) -c $< -o $(OBJ_DIR)/$@

run:
	./webserver 8080 /home/tyh/syse/sysefork/web

.PHONY : clean

clean:
	rm -rf $(TARGET) $(OBJ_DIR)/*.o ./*.o
	./kill.sh
	