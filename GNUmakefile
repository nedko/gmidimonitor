PREFIX=/usr
BIN_DIR=$(PREFIX)/bin
DATA_DIR=$(PREFIX)/share/gmidimonitor

CFLAGS := $(strip $(shell pkg-config --cflags gtk+-2.0 libglade-2.0)) -Wall -Werror -DDATA_DIR='"$(DATA_DIR)"'
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

install: gmidimonitor
	install gmidimonitor $(BIN_DIR)
	install -d $(DATA_DIR)
	install --mode=644 gmidimonitor.glade $(DATA_DIR)
