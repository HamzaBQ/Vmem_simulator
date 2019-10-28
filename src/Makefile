all: os.o request_queue.o main.o targa_creator.o netio.o
	gcc -g ./bin/os.o ./bin/request_queue.o ./bin/main.o ./bin/targa_creator.o ./bin/netio.o -lpthread -o ./bin/vsystem

main.o : main.c
	gcc -g -std=gnu99 -c main.c -o ./bin/main.o
os.o : os.c
	gcc -g -std=gnu99 -c os.c -o ./bin/os.o
request_queue.o : request_queue.c
	gcc -g -std=gnu99 -c request_queue.c -o ./bin/request_queue.o
targa_creator.o : targa_creator.c
	gcc -g -std=gnu99 -c targa_creator.c -o ./bin/targa_creator.o
netio.o : netio.c
	gcc -g -std=gnu99 -c netio.c -o ./bin/netio.o


client:
	gcc -g -std=gnu99 client.c -o ./bin/client
stegano_extractor:
	gcc -g -std=gnu99 stegano_extractor.c -o ./bin/stegano_extractor
