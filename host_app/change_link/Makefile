CC=g++ -g -std=c++11
BIN=../../bin

${BIN}/changelink:main.o
	$(CC) -o $@ $^

main.o:main.cpp
	$(CC) -c $^ 

clean:
	rm *.o
