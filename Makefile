wordcount: threadpool.o mapreduce.o distwc.o
	gcc -Wall -pthread -std=gnu99 threadpool.o mapreduce.o distwc.o -o wordcount

wordcounttest: threadpool.o mapreduce.o distwc.o
	gcc -Wall -Wextra -Werror -pthread -std=gnu99 -ggdb3 mapreduce.c threadpool.c distwc.c -o wordcount
	
threadpool.o: threadpool.c 
	gcc -Wall -pthread -std=gnu99 -c threadpool.c -o threadpool.o

mapreduce.o: mapreduce.c
	gcc -Wall -pthread -std=gnu99 -c mapreduce.c -o mapreduce.o

distwc.o: distwc.c
	gcc -Wall -pthread -std=gnu99 -c distwc.c -o distwc.o

clean:
	rm -f mapreduce.o threadpool.o distwc.o wordcount

cleanall:
	rm -f mapreduce.o threadpool.o distwc.o wordcount
	rm -f wordcount result-*.txt

cleano:
	rm -f wordcount result-*.txt
	rm -f valgrind-out.txt