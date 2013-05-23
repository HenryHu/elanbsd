elanbsd: elanbsd.cpp
	${CXX} ${CXXFLAGS} -I/usr/local/include -L/usr/local/lib -lXtst $< -o $@

test: elanbsd
	pkill elanbsd || true
	./elanbsd

run: elanbsd
	sudo hal-device -r psm_0 || true
	./elanbsd &

clean:
	rm -rf elanbsd

.PHONY: clean
