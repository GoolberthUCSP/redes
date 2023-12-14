#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
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
#include <set>
#include <mutex>
#include "lib/cache.h"
#include "lib/main.h"
#include "lib/util.h"

#define SIZE 1024
#define SEC_TIMEOUT 3
#define ERROR(s) {perror(s); exit(1);}

using namespace std;

int clientFD, keep_aliveFD;
struct sockaddr_in client_addr, keep_alive_addr;
int seq_number = 0;
int msg_id = 0;
struct timeval tv;
    
bool is_alive[4]{0}; // all storages start in death state
struct sockaddr_in storage_addr[4], storage_conn;
int storageFD[4];
int storage_port[4] = {5001, 5002, 5003, 5004};

void keep_alive();
void answer_query(struct sockaddr_in client, vector<unsigned char> buffer);

int main(){
    vector<unsigned char> recv_buffer(SIZE);
    string THIS_IP = "127.0.0.1";
    tv.tv_sec = SEC_TIMEOUT;
    tv.tv_usec = 0;
    
    memset(&client_addr, 0, sizeof(client_addr));
    memset(&keep_alive_addr, 0, sizeof(keep_alive_addr));
    
    if ((clientFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1)     ERROR("Socket")
    if ((keep_aliveFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1) ERROR("Socket")
    
    // configure client's socket
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(5000);
    if (inet_pton(AF_INET, THIS_IP.c_str(), &client_addr.sin_addr) == -1)                ERROR("inet_pton")
    if (bind(clientFD,(struct sockaddr *)&client_addr, sizeof(struct sockaddr)) == -1)   ERROR("Bind")
    if (setsockopt(clientFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) ERROR("setsockopt")

    // configure keep-alive socket
    keep_alive_addr.sin_family = AF_INET;
    keep_alive_addr.sin_port = htons(5005);
    if (inet_pton(AF_INET, THIS_IP.c_str(), &keep_alive_addr.sin_addr) == -1)                  ERROR("inet_pton")
    if (bind(keep_aliveFD,(struct sockaddr *)&keep_alive_addr, sizeof(struct sockaddr)) == -1) ERROR("Bind")
    if (setsockopt(keep_aliveFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)   ERROR("setsockopt")   

    // configure storage's sockets
    for (int i = 0; i < 4; i++){
        memset(&storage_addr[i], 0, sizeof(storage_addr[i]));
        storage_addr[i].sin_family = AF_INET;
        storage_addr[i].sin_port = htons(storage_port[i]);
        if (inet_pton(AF_INET, THIS_IP.c_str(), &storage_addr[i].sin_addr) == -1)                   ERROR("inet_pton")
        if (bind(storageFD[i],(struct sockaddr *)&storage_addr[i], sizeof(struct sockaddr)) == -1)  ERROR("Bind")
        if (setsockopt(storageFD[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)    ERROR("setsockopt")
    }

    thread(keep_alive).detach();
    while(true){
        memset(recv_buffer.data(), 0, SIZE);
        int bytes_readed = recvfrom(clientFD, recv_buffer.data(), SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, (socklen_t *)sizeof(struct sockaddr_in));
        if (bytes_readed == -1){
            // Timeout ?????????????????
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
                ERROR("recvfrom")
        }
        else{
            // Received on time
            thread(answer_query, client_addr, recv_buffer).detach();
        }
    }
}

void answer_query(struct sockaddr_in client, vector<unsigned char> buffer){
    // ss : seq_num|hash|type|msg_id|flag|nick_size|nickname|<data>
    string seq_num(2, 0), hash(6, 0), type(1, 0), msg_id(3, 0), flag(1, 0), nick_size(2, 0);
    stringstream ss;
    ss.write((char *)buffer.data(), buffer.size());
    ss.read(seq_num.data(), seq_num.size());
    ss.read(hash.data(), hash.size());
    ss.read(type.data(), type.size());
    ss.read(msg_id.data(), msg_id.size());
    ss.read(flag.data(), flag.size());
    ss.read(nick_size.data(), nick_size.size());
    // TODO: implement CRUD
}

void keep_alive(){
    struct timeval start, end, elapsed;
    string msg(1, '0');
    int num;
    while (true){
        for (int i = 0; i < 4; i++){
            if (is_alive[i])
                sendto(keep_aliveFD, msg.data(), msg.size(), MSG_CONFIRM, (struct sockaddr *)&storage_addr[i], sizeof(struct sockaddr));
            
            gettimeofday(&start, NULL);
            num = recvfrom(keep_aliveFD, msg.data(), msg.size(), MSG_WAITALL, (struct sockaddr *)&storage_addr[i], (socklen_t *)sizeof(struct sockaddr));
            if (num == -1){
                // Timeout
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    is_alive[i] = false;
                    cout << "Storage" << i << " is not alive" << endl;
                }
                else 
                    ERROR("recvfrom")
            }
            else{
                // Received on time
                gettimeofday(&end, NULL);
                timersub(&end, &start, &elapsed);
                int uremaining = (tv.tv_sec - elapsed.tv_sec) * 1000000 + (tv.tv_usec - elapsed.tv_usec);
                is_alive[i] = true;
                // Sleep for remaining time
                if (uremaining > 0)
                    usleep(uremaining);
            }
        }
    }    
}