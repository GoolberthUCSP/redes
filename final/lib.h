#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>


using namespace std;

string calc_hash(vector<unsigned char> packet){
    int sum = 0;
    ostringstream output;
    for (auto it : packet){
        sum += it;
    }
    output << setw(6) << setfill('0') << sum;
    return output.str();
}