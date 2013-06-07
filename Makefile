CXXFLAGS=-I/usr/local/include -pthread
LDFLAGS=-L/usr/local/lib -lXtst -pthread

OBJS=elanbsd.o

all: elanbsd

elanbsd.o: config.h mouse.h xdisplay.h finger.h button.h

elanbsd: ${OBJS}
	${CXX} -g ${LDFLAGS} ${OBJS} -o $@

test: elanbsd
	pkill elanbsd || true
	./elanbsd

run: elanbsd
	sudo hal-device -r psm_0 || true
	./elanbsd &

clean:
	rm -rf elanbsd ${OBJS}

.PHONY: clean
