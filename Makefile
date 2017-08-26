all: main.cpp
	g++ -std=c++11 -ggdb3 -O0 main.cpp

test: all
	valgrind ./a.out

clean:
	rm -f ./a.out
