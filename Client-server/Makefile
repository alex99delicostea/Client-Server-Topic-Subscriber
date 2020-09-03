
CFLAGS = -Wall -Wextra -Werror -g -pedantic-errors -std=c++14

IP_SERVER = 127.0.0.1 


PORT = 8080


all: server subscriber


server: server.cpp
	g++ ${CFLAGS} server.cpp -o server


subscriber: subscriber.cpp
	g++ ${CFLAGS} subscriber.cpp -o subscriber

.PHONY: clean run_server run_subscriber


run_server: server
	./server ${PORT}


run_subscriber: subscriber
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
