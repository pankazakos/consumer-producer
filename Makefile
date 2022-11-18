ARGS = lines.txt 5 3

build: producer.c
	gcc producer.c -o producer -lpthread -lrt -Wall
	gcc consumer.c -o consumer -lpthread -lrt -Wall

run: build
	./producer $(ARGS)

valgrind: build
	valgrind ./producer $(ARGS)
	
	
valgrind_all:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./producer $(ARGS)

clean:
	rm -f *.o producer consumer
