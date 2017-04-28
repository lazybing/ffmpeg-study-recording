##complie tool
CC = gcc

DIR_INC = /usr/local/include
LIBS = -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva -lx264 -lx265
DIR_LIB = /usr/local/lib

## source file path
SRC_PATH := .

all:
##	$(CC) -g demuxing_decoding.c -o demuxing_decoding -L$() -I$(DIR_INC) $(LIBS)
##	$(CC) -g metadata.c -o metadata -L$() -I$(DIR_INC) $(LIBS)
##	$(CC) -g container_parse.c -o container_parse -L$() -I$(DIR_INC) $(LIBS)
##	$(CC) -g decoder.c -o decoder -L$() -I$(DIR_INC) $(LIBS)
##	$(CC) -g encoder.c -o encoder -L$() -I$(DIR_INC) $(LIBS)
	$(CC) -g demuxer.c -o demuxer -L$() -I$(DIR_INC) $(LIBS)

clean:
	rm demuxing_decoding metadata container_parse encoder decoder

.PHONY:clean 
	

