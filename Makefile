CXXFLAGS=-I/usr/local/include
LDFLAGS=-L/usr/local/lib -lXtst

OBJS=elanbsd.o

elanbsd.o: config.h mouse.h xdisplay.h finger.h button.h

elanbsd: ${OBJS}
	${CXX} -g ${CXXFLAGS} ${LDFLAGS} ${OBJS} -o $@

test: elanbsd
	pkill elanbsd || true
	./elanbsd

run: elanbsd
	sudo hal-device -r psm_0 || true
	./elanbsd &

clean:
	rm -rf elanbsd

.PHONY: clean
