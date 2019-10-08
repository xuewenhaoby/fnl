CC=g++ -g
BIN=../../../bin/

${BIN}/send:main.o
	$(CC) -o $@ $^

main.o:main.cpp
	$(CC) -c $^ 

clean:
	rm *.o
