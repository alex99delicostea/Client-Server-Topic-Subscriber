#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <bits/stdc++.h>
#include "helpers.h"
#define INT        0
#define SHORT_REAL 1
#define FLOAT      2
#define STRING     3
using namespace std;




void decode_message(char *buffer) {
	uint16_t lenght = 0;;
	char *ip = buffer + 1;
	lenght = strlen(ip) + 1;
    uint16_t port = (uint16_t)(*(buffer + 1 + lenght));
	lenght += sizeof(uint16_t);
	char *topic = buffer + 1 + lenght;
	lenght += strlen(buffer + 1 + lenght) + 1;

	printf("%s:", ip);
	printf("%u - ", ntohs(port));
	printf("%s - ", topic);
	int sign;
	uint32_t value_for_int;
	uint16_t value_for_short;
	uint32_t value_for_float;
	uint8_t fractional_for_float;
	if(buffer[0] == INT){
       if(buffer[lenght + 1]){
       	sign = -1;
       }else{
       	sign = 1;
       }
       value_for_int = ntohl(*(uint32_t *)((buffer + lenght + 2)));
       value_for_int *= sign;
       printf("INT - %d\n", value_for_int);
	}else if(buffer[0] == SHORT_REAL){
		value_for_short = ntohs(*(uint16_t*)((buffer + lenght + 1)));
		printf("SHORT_REAL - %.02f\n", value_for_short / 100.0);
	}else if(buffer[0] == FLOAT){
       if(buffer[lenght + 1]){
       	sign = -1;
       }else{
       	sign = 1;
       }
       value_for_float = ntohl(*(uint32_t*)((buffer + lenght + 2)));
       fractional_for_float = *(uint8_t*)((buffer + lenght + 2 + sizeof(uint32_t)));
       double f_value = value_for_float;
       for(int i = 1; i <= fractional_for_float; i++){
       	   f_value *= 0.1;
       }
       f_value *= sign;
       printf("FLOAT - %f\n", f_value);

	}else if(buffer[0] == STRING){
		char *keep = buffer + 1 + lenght;
		printf("STRING - %s\n", keep);

	}else{
		printf("Unknown message\n");
	}


}

int main(int argc, char *argv[]) {

	if (argc != 4) {
		fprintf(stderr, "Usage: %s server_port\n", argv[0]);
		exit(0);
	}
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

    int ret;
	struct sockaddr_in serv_addr;
	serv_addr.sin_port = htons(atoi(argv[3]));
	serv_addr.sin_family = AF_INET;
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	char buffer[BUFLEN];

	
	fd_set read_fds;
	fd_set tmp_fds;
	
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);


	int i = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");


	char name[CLIENT_ID];
	memset(name, 0, CLIENT_ID);
	memcpy(name, argv[1], strlen(argv[1]));
	int n = send(sockfd, name, CLIENT_ID, 0);
	DIE(n < 0, "client_id");
	i = 1;

	while (1) {
		tmp_fds = read_fds;
        int fdmax = sockfd + 1;
  		ret = select(fdmax, &tmp_fds, NULL, NULL, NULL);
  		DIE(ret < 0, "select");
  		if (FD_ISSET(sockfd, &tmp_fds)) {
  			uint16_t lenght;
  			recv(sockfd, &lenght , sizeof(uint16_t), 0);
  			lenght = ntohs(lenght);
  			if (lenght == EXIT_CODE) {
  				break;
  			}

  			memset(buffer, 0, BUFLEN);
  			for(uint16_t size = 0; size < lenght; size += n){
						int size_read = lenght - size;
						n = recv(sockfd, buffer + size, size_read, 0);
						DIE(n < 0, "recv");
						if (n == 0) {
							break;
						}
					}
  			decode_message(buffer);

  		} else if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			TCP_Request req;
	        memset(&req, 0, sizeof(TCP_Request));
	        scanf("%s", req.request_type);
	         if(strcmp(req.request_type, subscribe) && strcmp(req.request_type, unsubscribe) && strcmp(req.request_type, exit_signal)) {
		          printf("Invalid command subscriber\n");
		          break;
	         }else if (!strcmp(req.request_type, exit_signal)) {
		          break;

	         }else{
	             scanf("%s", req.topic);

	             if (!strcmp(req.request_type, subscribe)) {
		                 scanf(" %c", &req.SF);
	              }
             }
	           int n = send(sockfd, &req, sizeof(TCP_Request), 0);
	           DIE(n < 0, "send");
               strcat(req.request_type, "d ");
               strcat(req.request_type, req.topic);
	           printf("%s\n", req.request_type);
	
		}
	}

	close(sockfd);
	return 0;
}
