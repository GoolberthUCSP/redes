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
#include "cache.h"
#include "lib.h"

#define SIZE 1024

using namespace std;

int primaryFD;
vector<int> storage_port= {5001, 5002, 5003, 5004};
Cache packets(100);

struct sockaddr_in primary_addr, connect_addr;
map<string, struct sockaddr_in> connects_addr;

typedef void (*func_ptr)(stringstream&, struct sockaddr_in);

void verify_storage(int n_storage);
void replay_ack(string nickname, string seq_num, bool one_ack);
void process_ack(string seq_num);
void send_message(stringstream &ss, struct sockaddr_in connect_addr);
void process_struct(stringstream &ss, struct sockaddr_in connect_addr);

map<char, func_ptr> process_functions({
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

        bytes_readed = recvfrom(primaryFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&connect_addr, (socklen_t *)&addr_len);
        
        cout << "Received " << bytes_readed << "B from " << inet_ntoa(connect_addr.sin_addr) << ":" << ntohs(connect_addr.sin_port) << endl;
        
        stringstream ss;
        ss.write((char *)recv_buffer.data(), recv_buffer.size());
        // ss : seq_num|hash|type|msg_id|flag|<data>
        string seq_num(2, 0), hash(6, 0), type(1, 0), msg_id(3, 0), flag(1, 0), nick_size(2, 0);
        ss.read(seq_num.data(), seq_num.size());
        ss.read(hash.data(), hash.size());
        ss.read(type.data(), type.size());
        ss.read(msg_id.data(), msg_id.size());
        ss.read(flag.data(), flag.size());
        ss.read(nick_size.data(), nick_size.size());
        string nickname(stoi(nick_size), 0);
        ss.read(nickname.data(), nickname.size());
        
        if (type == "A"){
            process_ack(seq_num);
            continue;
        }
        
        // Push packet into packets(Cache) if it's not corrupted, else send NAK
        bool is_good= (hash == calc_hash(recv_buffer))? true : false; // Calc hash only to data, without header
        replay_ack(nickname, seq_num, is_good);

        if (is_good) packets.insert(stoi(seq_num), recv_buffer);

        // Verify if flag = 1 (incomplete) else (complete)
        
        thread(process_functions[type[0]], ref(ss), connect_addr).detach();
    }
}

void verify_storage(int n_storage){
    return;
}