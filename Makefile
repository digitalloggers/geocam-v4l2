CFLAGS+=-fPIC -Wall -Werror

libv4l_geocam.so: v4l-geocam.o geocam_demux.o
	$(CC) -shared -export-dynamic -o libv4l_geocam.so v4l-geocam.o geocam_demux.o

v4l-geocam.o: v4l-geocam.c geocam.h
geocam_demux.o: geocam_demux.c geocam.h
