elanbsd: elanbsd.cpp
	${CXX} ${CXXFLAGS} -I/usr/local/include -L/usr/local/lib -lXtst $< -o $@

test: elanbsd
	./elanbsd
