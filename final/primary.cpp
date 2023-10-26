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

void init_storages();
void verify_storage(int n_storage);
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
    //Initialize storages
    init_storages();

    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        bytes_readed = recvfrom(primaryFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
        
        stringstream ss;
        ss.write((char *)recv_buffer.data(), recv_buffer.size());
        string seq_num(2, 0), hash(6, 0), type(1, 0), msg_id(3, 0), flag(1, 0);
        ss.read(seq_num.data(), seq_num.size());
        ss.read(hash.data(), hash.size());
        ss.read(type.data(), type.size());
        ss.read(msg_id.data(), msg_id.size());
        ss.read(flag.data(), flag.size());
        // Verify if flag = 1 (incomplete) else (complete)
        
        thread(processing, recv_buffer, client_addr).detach();
    }
}


void processing(vector<unsigned char> buffer, struct sockaddr_in client_addr){
    

    if (functions.find(type) == functions.end()){
        cout << "Bad type, not processed" << endl;
        return;
    }
    functions[type](ss, client_addr);
}

void verify_storage(int n_storage){
    return;
}

void init_storages(){
    for (int i=0; i<4; i++){
        if ((storageFDs[i] = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
            perror("Storage socket");
            exit(1);
        }
        memset(&storages_addr[i], 0, sizeof(storages_addr[i]));
        storages_addr[i].sin_family = AF_INET;
        storages_addr[i].sin_port = htons(storage_port[i]);
        if (inet_pton(AF_INET, "127.0.0.1", &storages_addr[i].sin_addr) == -1){
            perror("Storage inet_pton");
            exit(1);
        }
    }
}


