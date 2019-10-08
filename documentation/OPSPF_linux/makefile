OPSPF: ./src/nfqnl.o ./src/opspf.o ./src/api.o
	g++ -o ./bin/OPSPF ./src/nfqnl.o ./src/opspf.o ./src/api.o -lnfnetlink -lnetfilter_queue

nfqnl.o: ./src/nfqnl.cpp ./include/opspf.h ./include/api.h
	g++ -c ./src/nfqnl.cpp -lnfnetlink -lnetfilter_queue

opspf.o: ./src/opspf.cpp ./include/opspf.h ./include/api.h
	g++ -c ./src/opspf.cpp

api.o: ./src/api.cpp ./include/api.h
	g++ -c ./src/api.cpp

clean:
	rm ./src/*.o ./bin/OPSPF
