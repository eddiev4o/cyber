CFLAGS = -I ./include
LFLAGS = -lrt -lX11 -lGLU -lGL -pthread -lm #-lXrandr

all: cyber

cyber: cyber.cpp ppm.cpp log.cpp
	g++ $(CFLAGS) cyber.cpp ppm.cpp log.cpp libggfonts.a -Wall -Wextra $(LFLAGS) -o cyber \
	#-D USE_OPENAL_SOUND \
	#/usr/lib/x86_64-linux-gnu/libopenal.so \
	#/usr/lib/x86_64-linux-gnu/libalut.so
clean:
	rm -f cyber
	rm -f *.o

