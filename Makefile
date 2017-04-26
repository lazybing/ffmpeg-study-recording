all:
	gcc -g demuxing_decoding.c -o demuxing_decoding -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva
	gcc -g metadata.c -o metadata -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva
	gcc -g container_parse.c -o container_parse -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva
	gcc -g encoder.c -o encoder -L/usr/local/lib -I/usr/local/include -lavformat -lavdevice -lavcodec -lswscale -lswresample -lavutil -lavfilter -lpthread -lz -lm -lva

clean:
	rm demuxing_decoding metadata container_parse encoder
	rm videodst audiodst

.PHONY:clean
	

