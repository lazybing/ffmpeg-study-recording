all:
	gcc -o demuxing_decoding demuxing_decoding.c -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva
	gcc -o metadata metadata.c -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva
	gcc -o container_parse container_parse.c -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva

clean:
	rm demuxing_decoding metadata container_parse

.PHONY:clean
	

