CXX=g++-10 #g++-10.2 for merlin
CXXFLAGS= -std=c++20 -fconcepts -fconcepts-diagnostics-depth=10 -Werror=return-type -Wall -Wextra -Werror\
 -Wpedantic -Wno-unknown-pragmas -Wno-unused-function -Wpointer-arith -Wno-cast-qual -Wno-type-limits\
 -Wno-strict-aliasing -O3 -g

BIN_NAME=huff_codec
BENCH_BIN_NAME=bench
LIBS_PATH=libs

INC=-I$(LIBS_PATH)

default: $(BIN_NAME)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(BIN_NAME): main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

bench.o: bench.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(BENCH_BIN_NAME): bench.o
	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	rm -f *.o $(BIN_NAME) $(BENCH_BIN_NAME)

zip:
	zip -r kko_xflajs00.zip *.cpp *.h Makefile $(LIBS_PATH)/* args/*