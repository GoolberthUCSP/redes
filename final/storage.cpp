// Redes y Comunicación
// Socket UDP Storage Server

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <thread>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>

#define SIZE 1024

using namespace std;

int socketFD;
struct sockaddr_in primary_addr;

map<string, vector<string>> database; // database[node]: {nodes that are connected to node}
vector<int> storage_port= {5001, 5002, 5003, 5004};

typedef void (*func_ptr)(stringstream&);

// CRUD request functions
void create_request(stringstream &ss);
void read_request(stringstream &ss);
void update_request(stringstream &ss);
void delete_request(stringstream &ss);

map<char, func_ptr> crud_requests({
    {'C', &create_request},
    {'R', &read_request},
    {'U', &update_request},
    {'D', &delete_request}
});

int main(int argc, char *argv[]){
    if (argc != 2){
        cout << "Bad number of arguments" << endl;
        return 1;
    }
    int port = storage_port[atoi(argv[1])%4];

    int bytes_readed;
    vector<unsigned char> recv_buffer(SIZE);
    int addr_len= sizeof(struct sockaddr_in);

    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket");
        exit(1);
    }

    memset(&primary_addr, 0, sizeof(primary_addr));
    primary_addr.sin_family = AF_INET;
    primary_addr.sin_port = htons(port);
    primary_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketFD,(struct sockaddr *)&primary_addr, sizeof(struct sockaddr)) == -1){
        perror("Bind");
        exit(1);
    }

	cout << "UDPServer Waiting for client on port " << port << "..." << endl;
    
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(socketFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&primary_addr, (socklen_t *)&addr_len);
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(primary_addr.sin_addr) << ":" << ntohs(primary_addr.sin_port) << endl;
        thread(processing, recv_buffer).detach();
    }
}


void processing(vector<unsigned char> buffer, struct sockaddr_in primary_addr){
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    char type;
    ss >> type;
    if (crud_requests.find(type) == crud_requests.end()){
        cout << "Bad type" << endl;
        return;
    }
    crud_requests[type](ss, primary_addr);
}