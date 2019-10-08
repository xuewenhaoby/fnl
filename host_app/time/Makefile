CC=g++ -g
BIN=../../../bin/

t:main.o
	$(CC) -o $@ $^

main.o:main.cpp
	$(CC) -c $^ 

clean:
	rm *.o
