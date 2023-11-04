#ifndef LIB_H
#define LIB_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>


using namespace std;

struct Header{
    string nickname;
    string seq_num;
    Header(string n, string s) : nickname(n), seq_num(s) {}
};

string calc_hash(vector<unsigned char> packet){
    int sum = 0;
    ostringstream output;
    for (auto it : packet){
        sum += it;
    }
    output << setw(6) << setfill('0') << sum;
    return output.str();
}

string calc_hash(string packet){
    int sum = 0;
    ostringstream output;
    for (auto it : packet){
        sum += it;
    }
    output << setw(6) << setfill('0') << sum;
    return output.str();
}

#endif