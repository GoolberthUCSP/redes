// Redes y Comunicación
// Socket UDP Primary Server

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
#include <utility>
#include <algorithm>
#include <thread>

#define SIZE 1024

using namespace std;

int clientFD, primaryFD;
vector<int> storage_port= {5001, 5002, 5003, 5004};
vector<int> storageFDs(4);

struct sockaddr_in primary_addr, client_addr;
vector<struct sockaddr_in> storages_addr(4);

typedef void (*func_ptr)(stringstream&, struct sockaddr_in);

void processing(vector<unsigned char> buffer, struct sockaddr_in client_addr);
void send_message(stringstream &ss, struct sockaddr_in client_addr);
void save_client(stringstream &ss, struct sockaddr_in client_addr);
void process_struct(stringstream &ss, struct sockaddr_in client_addr);

map<char, func_ptr> functions({
    {'N', &save_client},
    {'M', &send_message},
    {'P', &process_struct}
});

int main(){
    int bytes_readed;
    vector<unsigned char> recv_buffer(SIZE);
    int addr_len= sizeof(struct sockaddr_in);

    if ((primaryFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket");
        exit(1);
    }

    memset(&primary_addr, 0, sizeof(primary_addr));
    primary_addr.sin_family = AF_INET;
    primary_addr.sin_port = htons(5000);
    primary_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(primaryFD,(struct sockaddr *)&primary_addr, sizeof(struct sockaddr)) == -1){
        perror("Bind");
        exit(1);
    }
	cout << "UDPServer Waiting for client on port 5000..." << endl;
    
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(primary_addr, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
        thread(processing, recv_buffer, client_addr).detach();
    }
}


void processing(vector<unsigned char> buffer, struct sockaddr_in client_addr){
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    char type;
    ss >> type;
    if (functions.find(type) == functions.end()){
        cout << "Bad type" << endl;
        return;
    }
    functions[type](ss, client_addr);
}
