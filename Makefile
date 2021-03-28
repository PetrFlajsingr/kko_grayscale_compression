CXX=g++-10 #g++-10.2 for merlin
CXXFLAGS= -fconcepts -fconcepts-diagnostics-depth=10 -fcoroutines -Werror=return-type -Wall -Wextra -Werror -Wpedantic -Wno-unknown-pragmas -Wno-unused-function -Wpointer-arith -Wno-cast-qual -Wno-type-limits -Wno-strict-aliasing -g -o3
BIN_NAME=huff_codec


main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_NAME): main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o $(BIN_NAME)

makezip:
	zip -r kko_xflajs00.zip *.cpp *.h Makefile include/* args/*