CFLAGS := $(strip $(shell pkg-config --cflags gtk+-2.0 libglade-2.0)) -Wall -Werror
LIBS := $(strip $(shell pkg-config --libs gtk+-2.0 libglade-2.0 gmodule-2.0 gthread-2.0 alsa))

OBJECTS=about.o path.o glade.o main.o

# rebuild by default, until we have dependecies
default: rebuild

gmidimonitor: $(OBJECTS)
	gcc $(LIBS) -o $@ $(OBJECTS)

clean:
	rm -f gmidimonitor
	rm -rf *.o

rebuild: clean gmidimonitor
