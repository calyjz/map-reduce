wordcount: threadpool.o mapreduce.o distwc.o
	gcc -Wall -pthread -std=c99 threadpool.o mapreduce.o distwc.o -o wordcount

threadpool.o: threadpool.c 
	gcc -Wall -pthread -std=c99 -c threadpool.c -o threadpool.o

mapreduce.o: mapreduce.c
	gcc -Wall -pthread -std=c99 -c mapreduce.c -o mapreduce.o

distwc.o: distwc.c
	gcc -Wall -pthread -std=c99 -c distwc.c -o distwc.o

clean:
	rm -f mapreduce.o threadpool.o distwc.o wordcount

cleanall:
	rm -f mapreduce.o threadpool.o distwc.o wordcount
	rm -f wordcount result-*.txt