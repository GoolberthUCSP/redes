#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <iomanip>
#include <vector>
#include <sstream>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <mutex>
#include "ttt.h"

#define SIZE 200000

using namespace std;

struct Connection{
    int SocketFD;
    mutex mtx, mtx2;
    vector<unsigned char> send_buff;
    vector<unsigned char> recv_buff;
    unsigned char tmp_recv[SIZE];
    char recv_size[11];
    string size_nickname;
    bool disconnected;
    stringstream ss, ss2; // ss is for sending, ss2 is for receiving
    char type;
    ifstream infile;
    ofstream outfile;

    Connection(int SocketFD){
        this->SocketFD = SocketFD;
        disconnected = false;
    }
    void send_request(string send_str){ //Send request to server by send_buff with send_str
        vector<unsigned char> os;
        os.insert(os.begin(), send_str.begin(), send_str.end());
        send_request(os);
    }

    void send_request(vector<unsigned char> &send_vect){ //Send request to server by send_buff with send_vect
        
        mtx.lock(); // Critical section: encoding and decoding use this function, the two functions are called in different threads
        send_buff.clear();

        ostringstream send_buff_size;
        send_buff_size << setw(10) << setfill('0') << send_vect.size();
        string send_size = send_buff_size.str();
        send_buff.insert(send_buff.begin(), send_size.begin(), send_size.end());
        send_buff.insert(send_buff.end(), send_vect.begin(), send_vect.end());

        int bytes_sent = 0, bytes_to_send = send_buff.size();
        while (bytes_to_send > 0){
            bytes_sent = send(SocketFD, send_buff.data() + bytes_sent, bytes_to_send, 0);
            if (bytes_sent < 0)
                perror("ERROR writing to socket");
            bytes_to_send -= bytes_sent;
        }
        mtx.unlock();

    }

    void recv_and_decode(){ //Load response to recv_buff
        
        recv_buff.clear();
        memset(recv_size, 0, 11);

        int n = recv(SocketFD, recv_size, 10, 0);
        if (n < 0)
            perror("ERROR reading from socket");

        int resp_size = stoi(recv_size);
        while(resp_size > 0){
            bzero(tmp_recv, SIZE);
            n = recv(SocketFD, tmp_recv, resp_size, 0);
            if (n < 0)
                perror("ERROR reading from socket");
            
            recv_buff.insert(recv_buff.end(), tmp_recv, tmp_recv+n);
            resp_size -= n;
        }
        for (int i = 0; i < recv_buff.size(); i++){
            cout << recv_buff[i];
        } cout << endl;
        send_confirmation(recv_buff.size());
        decoding();
    }
    // Encoding functions
    void encoding(string input){ //Encoding the user input and send it to the server
        // input : type,[content]
        ss.clear();
        ss.write(input.c_str(), input.size());
        ss >> type;
        // Ignore the comma after the type
        char cm; ss >> cm;

        type = tolower(type);

        if (type == 'n'){ //Change nickname request
            change_nickname();
        }
        else if (type == 'm'){ //Message to one request
            send_message_to_one();
        }
        else if (type == 'w'){ //Message to all request
            send_message_to_all();
        }
        else if (type == 'f'){ //File to one request
            send_file_to_one();
        }
        else if (type == 'p'){ //Play game request
            play_game();
        }
        else if (type == 'l'){ //Online clients list request
            string send_str = "L00";
            send_request(send_str);
        }
        else if (type == 'v'){ //View board request
            string send_str = "VTTT";
            send_request(send_str);
        }
        else if (type == 'b'){ //Register to game request
            string send_str = "BTTT";
            send_request(send_str);
        }
        else if (type == 'q'){ //Quit: Disconnect request
            string send_str = "Q00";
            disconnected = true;
            send_request(send_str);
        }
        else if (type == 'c'){ //Clear screen
            system("clear || cls");
        }
        else {
            cout << "ERROR: Invalid message format, please try again" << endl;
        }
    }
    string get_hash(vector<unsigned char> &buffer, int buff_size){
        // Hash value is only the sum of the ASCII characters of the message
        ostringstream hash_value;
        int hash;
        for (int i = 0; i < buff_size; i++){
            hash += buffer[i];
        }
        hash_value << setw(10) << setfill('0') << hash;
        return hash_value.str();
    }
    string get_datetime(){
        // Return the current date and time in YYYYMMDDHHMMSS format
        time_t now = time(0);
        tm *ltm = localtime(&now);
        ostringstream datetime;
        datetime << to_string(1900 + ltm->tm_year);
        datetime << setw(2) << setfill('0') << to_string(1 + ltm->tm_mon);
        datetime << setw(2) << setfill('0') << to_string(ltm->tm_mday);
        datetime << setw(2) << setfill('0') << to_string(ltm->tm_hour);
        datetime << setw(2) << setfill('0') << to_string(ltm->tm_min);
        datetime << setw(2) << setfill('0') << to_string(ltm->tm_sec);
        return datetime.str();
    }
    void send_confirmation(int num_bytes){
        ostringstream n_bytes;
        n_bytes << setw(10) << setfill('0') << num_bytes;
        string send_str = "C" + n_bytes.str();
        send_request(send_str);
    }
    void change_nickname(){
        // ss : nickname
        // out: N00nickname 
        string new_nickname;
        getline(ss, new_nickname, '\0');
        ostringstream size;
        size << setw(2) << setfill('0') << new_nickname.size();
        string send_str = "N" + size.str() + new_nickname; 
        send_request(send_str);
    }
    void send_message_to_one(){
        // ss : receiver,message
        // out: M00receiver000message
        string receiver, message;
        getline(ss, receiver, ',');
        getline(ss, message, '\0');
        ostringstream size_receiver, size_message;
        size_receiver << setw(2) << setfill('0') << receiver.size();
        size_message << setw(3) << setfill('0') << message.size();
        string send_str = "M" + size_receiver.str() + receiver + size_message.str() + message;
        send_request(send_str);
    }
    void send_message_to_all(){
        // ss : message
        // out: W000message
        string message;
        getline(ss, message, '\0');
        ostringstream size_message;
        size_message << setw(3) << setfill('0') << message.size();
        string send_str = "W" + size_message.str() + message;
        send_request(send_str);
    }
    void send_file_to_one(){
        // ss : receiver,filename
        // out: F00receiver00000filename(10B size of file)(file)(10B hash value)(14B datetime of sent file in YYYYMMDDhhmmss)
        string receiver, filename;
        getline(ss, receiver, ',');
        getline(ss, filename, '\0');
        ostringstream size_receiver, size_filename, size_file_str;
        vector<unsigned char> file;
        int size_file;
        size_receiver << setw(2) << setfill('0') << receiver.size();
        size_filename << setw(5) << setfill('0') << filename.size();
        
        infile.open("data/" + filename, ios::binary);
        if (!infile.good()) {
          cout << "ERROR: File not found in data/" + filename << endl;
          return;
        }

        try{
          // Open existent file
          infile.seekg(0, infile.end);
          size_file = infile.tellg();
          size_file_str << setw(10) << setfill('0') << size_file;

          infile.seekg(0, infile.beg); 
          file.resize(size_file);
          infile.read((char *)file.data(), size_file);
          infile.close();
        } catch (exception &e){
          cout << "ERROR: " << e.what() << endl;
        }
        string hash_data = get_hash(file, size_file);
        string send_str = "F" + size_receiver.str() + receiver + size_filename.str() + filename + size_file_str.str();
        vector<unsigned char> send_vect;
        send_vect.insert(send_vect.begin(), send_str.begin(), send_str.end());
        send_vect.insert(send_vect.end(), file.begin(), file.end());
        send_str = hash_data + get_datetime();
        send_vect.insert(send_vect.end(), send_str.begin(), send_str.end());
        send_request(send_vect);
        cout << "File sent with hash: " << hash_data << endl;
    }
    void play_game(){
        // ss : 1 digit of game number
        // out: P0
        char num;
        ss >> num;
        int number = num - '0';
        if (number < 1 || number > 9){
            cout << "ERROR: Invalid game number, please try again" << endl;
            return;
        }
        string send_str = "P" + to_string(number);
        send_request(send_str);
    }
    // Decoding functions
    void decoding(){ // Decoding the response and process it
        // recv_buff : type[Content]
        ss2.clear();
        ss2.write((char *)recv_buff.data(), recv_buff.size());
        char type;
        ss2 >> type;

        if (type == 'N'){ //Get notification
            read_notification();
        }
        else if (type == 'M'){ //Get message from "to one"
            read_message_one();
        }
        else if (type == 'W'){ //Get message from "to all"
            read_message_all();
        }
        else if (type == 'F'){ //Get file from "to one"
            read_file_one();
        }
        else if (type == 'R'){ //Get hash calculated by receiver
            read_hash_rcv();
        }
        else if (type == 'V'){ //Get actualization of game board
            read_board();
        }
        else if (type == 'L'){ //Get online clients list
            read_online_clients();
        }
    }

    void read_notification(){
        // ss : 00notification
        string size(2, '0');
        ss2.read(size.data(), size.size());
        string notification(stoi(size), '0');
        ss2.read(notification.data(), notification.size());
        cout << "Notification: " << notification << endl;
    }
    void read_message_one(){
        // ss2 : 00sender000message
        string sdr_size(2, '0'), msg_size(3, '0');

        ss2.read(sdr_size.data(), sdr_size.size());
        string sender(stoi(sdr_size), '0');
        ss2.read(sender.data(), sender.size());

        ss2.read(msg_size.data(), msg_size.size());
        string message(stoi(msg_size), '0');
        ss2.read(message.data(), message.size());

        cout << "Message from " << sender << ": " << message << endl;
    }
    void read_message_all(){
        // ss2 : 000message
        string msg_size(3, '0');

        ss2.read(msg_size.data(), msg_size.size());
        string message(stoi(msg_size), '0');
        ss2.read(message.data(), message.size());

        cout << "Message to all: " << message << endl;
    }
    void read_file_one(){
        // ss2 : 00sender00000filename(10B size of file)(file)(10B hash value)(14B datetime of sent file in YYYYMMDDhhmmss)
        string sdr_size(2, '0'), filename_size(5, '0'), file_size(10, '0'), hash_data_rcv(10, '0'), datetime(14, '0');

        ss2.read(sdr_size.data(), sdr_size.size());
        string sender(stoi(sdr_size), '0');
        ss2.read(sender.data(), sender.size());

        ss2.read(filename_size.data(), filename_size.size());
        string filename(stoi(filename_size), '0');
        vector<unsigned char> file(stoi(file_size));
        ss2.read((char *)file.data(), file.size());

        ss2.read(hash_data_rcv.data(), hash_data_rcv.size());
        string hash_data_calc = get_hash(file, stoi(file_size));
        string datetime_rcv = get_datetime();

        // TODO: Check hash data

        try{
            outfile.open("out/" + filename, ios::binary);
            outfile.write((char *)file.data(), file.size());
            outfile.close();
            cout << "File " << filename << " received with hash: " << hash_data_calc << endl;
        } catch (exception &e){
            cout << "ERROR: " << e.what() << endl;
        }
        // Send hash calculated: R00sender(10B hash value)
        string send_str = "R" + sdr_size + sender + hash_data_calc;
        send_request(send_str);
    }
    void read_hash_rcv(){
        // ss2 : 00receiver(10B hash value)
        string size(2, '0'), hash_data_rcv(10, '0');
        ss2.read(size.data(), size.size());
        string receiver(stoi(size), '0');
        ss2.read(receiver.data(), receiver.size());
        ss2.read(hash_data_rcv.data(), hash_data_rcv.size());
        cout << "Hash calculated by " << receiver << ": " << hash_data_rcv << endl;
    }
    void read_board(){    
        // ss2 : 00player_x00player_o00(X1234Y5678= encoded board)
        string size_board(2, '0'), size_p_x(2, '0'), size_p_o(2, '0');

        ss2.read(size_p_x.data(), size_p_x.size());
        string player_x(stoi(size_p_x), '0');
        ss2.read(player_x.data(), player_x.size());

        ss2.read(size_p_o.data(), size_p_o.size());
        string player_o(stoi(size_p_o), '0');
        ss2.read(player_o.data(), player_o.size());

        ss2.read(size_board.data(), size_board.size());
        string board(stoi(size_board), '0');
        ss2.read(board.data(), board.size());
        TicTacToe::print_board(board, player_x, player_o);
    }
    void read_online_clients(){
        // ss2 : 000name1,name2,name3...
        string size(3, '0'), name;
        char c;
        vector<string> names;
        ss2.read(size.data(), size.size());
        for (int i = 0; i < stoi(size); i++){
        ss2 >> c;
        if (c == ','){
            names.push_back(name);
            name = "";
        }
        else name += c; 
        } 
        names.push_back(name);
        cout << "Online users: " << names.size() << endl;
        for (auto str: names){
        cout << "- " << str << endl; 
        }
    }
};

#endif