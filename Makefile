all: bin/tool

bin/tool: main.cpp
	mkdir -p bin/
	clang main.cpp -o bin/tool -std=c++11 -O3 -Wall

run: bin/tool
	./bin/tool

clean:
	rm -rf output
	rm -rf bin
