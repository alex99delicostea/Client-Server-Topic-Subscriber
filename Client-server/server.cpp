#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <vector>
#include "helpers.h"
#define INT        0
#define SHORT_REAL 1
#define FLOAT      2
#define STRING     3
using namespace std;



struct UDP_ClientMsg {
	char topic[TOPIC];
	char data_type;
	char content[CONTENT];
};


struct Subscriber {
	char SF;
	string name;
	string message;

	Subscriber(char SF = 0, string name = "", string message = "") {
		this -> SF = SF;
		this -> name = name;
		this -> message = message;
	}
};
std::unordered_map<string, unordered_map<string, Subscriber>> topics_all_info;
std::unordered_map<int, pair<string, int>> ip_and_port_keeper;
std::unordered_map<string, int> online_clients;
std::unordered_map<int, string> all_tcp_clients;

void Authentificate_new_client(string id, int sock) {
	all_tcp_clients[sock] = id;
	char buffer[BUFLEN];
	if (online_clients.find(all_tcp_clients[sock]) == online_clients.end()) {
		online_clients.insert(make_pair(id, sock));
	} else {
		online_clients[id] = sock;
		for (auto it : topics_all_info) {
			if (it.second.find(id) != it.second.end() && it.second[id].SF == '1') {
					memset(buffer, 0, BUFLEN);
					uint16_t start;
					for(unsigned int j = 0; j < it.second[id].message.size(); j++){
						buffer[j] = it.second[id].message[j];
					}
					
					memmove(&start, buffer, sizeof(uint16_t));
					start = ntohs(start);
					send(sock, buffer, start + sizeof(uint16_t), 0);
					//it.second[id].message.erase();	
			}
		}
	}
	char message_about_client[50];
	memset(message_about_client, 0, 50);
	strcat(message_about_client, "New Client ");
	strcat(message_about_client, id.c_str());
	strcat(message_about_client, " connected from ");
	strcat(message_about_client, ip_and_port_keeper[sock].first.c_str());
	strcat(message_about_client, " : ");
	printf("%s%u\n", message_about_client, ip_and_port_keeper[sock].second);
	
}


int request_from_client(char *buffer, int sock) {
	TCP_Request request_package;
	memset(&request_package, 0, sizeof(TCP_Request));
	memmove(&request_package, buffer, sizeof(TCP_Request));

	char message_about_client[50];
	memset(message_about_client, 0, 50);
	strcat(message_about_client, "Client ");
	strcat(message_about_client, all_tcp_clients[sock].c_str());
	strcat(message_about_client, " ");
	strcat(message_about_client, request_package.request_type);
	strcat(message_about_client, "d ");
	strcat(message_about_client, request_package.topic);
	printf("%s\n", message_about_client);

	string Topic;
	Topic = request_package.topic;
	if (topics_all_info.find(Topic) == topics_all_info.end()) {
		topics_all_info.insert(make_pair(Topic, unordered_map<string, Subscriber>()));
	}

	if (!strcmp(request_package.request_type, subscribe)) {
		if (topics_all_info[Topic].find(all_tcp_clients[sock]) != topics_all_info[Topic].end()) {
			topics_all_info[Topic][all_tcp_clients[sock]].SF = request_package.SF;
			return 0;
		}

        string s;
        s.reserve(BUFLEN);
		topics_all_info[Topic].insert(std::pair<string,
			Subscriber>(all_tcp_clients[sock], Subscriber(request_package.SF, all_tcp_clients[sock],
				s)));

	} else if (!strcmp(request_package.request_type, unsubscribe)){

		if (topics_all_info[Topic].find(all_tcp_clients[sock]) == topics_all_info[Topic].end()) {
			printf("Client already unsubscribed\n");
			return 1;
		}
		topics_all_info[Topic].erase(all_tcp_clients[sock]);
	}
	return 0;
}

int main(int argc, char *argv[]) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s client_id server_address server_port\n", argv[0]);
		exit(0);
	}
	int newsockfd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t clilen;
	char buffer[BUFLEN];

	
	fd_set tmp_fds;
	fd_set read_fds;
	
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    
	int udp_sockfd;
	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "socket udp");
	
	int tcp_sockfd;
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "socket tcp");

	
	int portno;
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");
	memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portno);

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	FD_SET(tcp_sockfd, &read_fds);

	int i = 1;
	int n,ret;
	ret = setsockopt(tcp_sockfd, IPPROTO_TCP,
		TCP_NODELAY, (void *)&i, sizeof(i));

	ret = bind(tcp_sockfd,
		(struct sockaddr *) &server_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = bind(udp_sockfd,
		(struct sockaddr *) &server_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");


	ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen tcp");

	
	int fdmax = max(tcp_sockfd, udp_sockfd);

	
	int exit_time = 0;
	while (1) {
		tmp_fds = read_fds; 

		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				
				if (i == udp_sockfd) {
                    
	                UDP_ClientMsg msg;	   
	                memset(buffer, 0, BUFLEN);
	                memset(&msg, 0, sizeof(UDP_ClientMsg));
	                n = recvfrom(udp_sockfd, &msg, sizeof(UDP_ClientMsg), 0, (struct sockaddr *) &client_addr, &clilen);
	                DIE(n < 0, "recv");
	                if (n > 0) {
		                 std::string Topic;               
		                 Topic = msg.topic;
		            if (topics_all_info.find(Topic) == topics_all_info.end()) {
			              topics_all_info.insert(make_pair(Topic, unordered_map<string, Subscriber>()));
		            }
		             uint16_t lenght = 0;
		
		             buffer[2] = msg.data_type;
		             memmove(buffer + 3, inet_ntoa(client_addr.sin_addr), strlen(inet_ntoa(client_addr.sin_addr)));
		             lenght += strlen(inet_ntoa(client_addr.sin_addr)) + 1;
		             memmove(buffer + 3 + lenght, &client_addr.sin_port, sizeof(unsigned short));
		             lenght += sizeof(unsigned short);
		             memmove(buffer + 3 + lenght, msg.topic, TOPIC);
		             lenght += strlen(buffer + 3 + lenght) + 1;
		

		             if(msg.data_type == INT){
			             memmove(buffer + 3 + lenght,
			             msg.content, 1 + sizeof(uint32_t));
			             lenght += 1 + sizeof(uint32_t);
		             }else if(msg.data_type == SHORT_REAL){
			             memmove(buffer + 3 + lenght,
			             msg.content, sizeof(uint16_t));
			             lenght += sizeof(uint16_t);
		              }else if(msg.data_type == FLOAT){
			             memmove(buffer + 3 + lenght, msg.content,
			             1 + sizeof(uint32_t) + sizeof(uint8_t));
			             lenght += 1 + sizeof(uint32_t) + sizeof(uint8_t);
		              }else if(msg.data_type == STRING){
			             memmove(buffer + 3 + lenght, msg.content, CONTENT);
			             lenght += strlen(buffer + 3 + lenght);
		             }
         
		                 lenght++;
		
		                 uint16_t nlen = htons(lenght);
		                 memmove(buffer, &nlen, sizeof(uint16_t));
		                
		                 for (auto it : topics_all_info[Topic]) {
			                  if(online_clients[it.second.name] == -1 && it.second.SF == '1') {
					        
			                      string s;	
					              for(unsigned int j = 0; j < sizeof(buffer); j++){
					          	          s.push_back(buffer[j]);
					          	  
					               }
					              topics_all_info[Topic][it.second.name].message = s;
					              
					        
			        }
			                send(online_clients[it.second.name], buffer, lenght + sizeof(uint16_t), 0);
		            }
		
	                 } else {
		                      printf("UDP - connection dropped\n");
	               }
				
				} else if (i == tcp_sockfd) {
                    //cout <<"aiciiiii";
                    char ip[INET_ADDRSTRLEN];
					clilen = sizeof(client_addr);
					newsockfd = accept(tcp_sockfd, (struct sockaddr *) &client_addr, &clilen);
					inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); 

					DIE(newsockfd < 0, "accept tcp");
					FD_SET(newsockfd, &read_fds);
					fdmax = max(newsockfd, fdmax);
					all_tcp_clients.insert(make_pair(newsockfd, ""));
					ip_and_port_keeper.insert(make_pair(newsockfd, make_pair(string(ip), ntohs(client_addr.sin_port))));

				} else if (i == STDIN_FILENO) {
					memset(buffer, 0, BUFLEN);
					scanf("%s", buffer);
					
					if (!strcmp(buffer, exit_signal)) {
						uint16_t nlen = htons(EXIT_CODE);
						for (auto it : all_tcp_clients) {
							send(it.first, &nlen, sizeof(uint16_t), 0);
							close(it.first);
						}
						exit_time = 1;
						break;
					} else {
						printf("Usage - Just exit command for server\n");
					}
				} else {
					
					uint16_t recv_size;
					if(!all_tcp_clients[i].size()){
						recv_size = CLIENT_ID;
					}else{
						recv_size = sizeof(TCP_Request);
					}
					memset(buffer, 0, BUFLEN);
					
					for(uint16_t size = 0; size < recv_size; size += n){
						int size_read = recv_size - size;
						n = recv(i, buffer + size, size_read, 0);
						DIE(n < 0, "recv");
						if (n == 0) {
							break;
						}
					}

					if(n) {
						if (!all_tcp_clients[i].size()) {
							
							string name;
							name = buffer;
							Authentificate_new_client(name, i);
							continue;
						}

						if (request_from_client(buffer, i) != 0) {
							printf("An error occured\n");
						}
						
					} else {
						if (all_tcp_clients.find(i) != all_tcp_clients.end()) {
							online_clients[all_tcp_clients[i]] = -1;
							ip_and_port_keeper.erase(i);
							all_tcp_clients.erase(i);
							printf("Client %s disconnected\n", all_tcp_clients[i].c_str());
						}
						close(i);
						FD_CLR(i, &read_fds);
					}
				}
			}
		}
		if(exit_time == 1){
			break;
		}
	}

    topics_all_info.clear();
    online_clients.clear();
    all_tcp_clients.clear();
    ip_and_port_keeper.clear();

	close(tcp_sockfd);
	close(udp_sockfd);

	return 0;
}