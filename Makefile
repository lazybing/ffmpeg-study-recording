##complie tool
CC = gcc

DIR_INC = /usr/local/include
DIR_LIB = /usr/local/lib
LIBS    = -lavdevice -lavfilter -lfreetype -lpostproc -lavformat -lavcodec -lswscale -lswresample -lavutil -lpthread -lz -lm -lva -lx264 -lx265 -lasound -lxcb -lX11 -lsndio -lXt -lGL -lGLU 

## source file path
SRC_PATH := .

all:
##	$(CC) -O0 -g demuxing_decoding.c -o demuxing_decoding -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
##	$(CC) -O0 -g metadata.c -o metadata -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
##	$(CC) -O0 -g container_parse.c -o container_parse -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
	$(CC) -O0 -g decoder.c -o decoder -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
##	$(CC) -O0 -g encoder.c -o encoder -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
##	$(CC) -O0 -g demuxer.c -o demuxer -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
##	$(CC) -O0 -g filter.c -o filter -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)
##	$(CC) -O0 -g screen_capture.c -o screen_capture -L$(DIR_LIB) -I$(DIR_INC) $(LIBS)

clean:
	rm demuxing_decoding metadata container_parse encoder decoder demuxer

.PHONY:clean 

