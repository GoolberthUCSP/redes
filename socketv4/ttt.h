#ifndef TTT_H
#define TTT_H
// Tic Tac Toe game

#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

struct TicTacToe{
    char board[3][3];
    pair<string, char> player_1, player_2;
    char turn;
    char winner;

    TicTacToe(){}
    TicTacToe(pair<string, char> player_1, pair<string, char> player_2, char turn){
        this->player_1 = player_1;
        this->player_2 = player_2;
        this->turn = turn;
        this->winner = 0;
        memset(board, '-', sizeof(board));
    }
    bool play(int pos){
        --pos;
        if (pos < 0 || pos > 8)
            return false;
        if (board[pos/3][pos%3] != '-')
            return false;
        board[pos/3][pos%3] = turn;
        turn = turn == 'X' ? 'O' : 'X';
        return true;
    }
    bool is_full(){
        for (int i = 0; i < 3; i++){
            for (int j = 0; j < 3; j++){
                if (board[i][j] == '-')
                    return false;
            }
        }
        return true;
    }
    bool verify_turn(char c){
        return c == turn;
    }
    bool verify_turn(string player){
        return (player == player_1.first && turn == player_1.second) || (player == player_2.first && turn == player_2.second);
    }
    bool verify_player(string player){
        return player == player_1.first || player == player_2.first;
    }
    bool verify_winner(){
        for (int i = 0; i < 3; i++){
            if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != '-'){
                winner = board[i][0];
                return true;
            }
            if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != '-'){
                winner = board[0][i];
                return true;
            }
        }
        if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != '-'){
        winner = board[0][0];
        return true;
        }
        if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != '-'){
        winner = board[0][2];
        return true;
        }
        return false;
    }
    string &get_turn_now(){
        return turn == player_1.second ? player_1.first : player_2.first;
    }
    string &get_turn_next(){
        return turn == player_1.second ? player_2.first : player_1.first;
    }
    string get_winner(){
        return winner == player_1.second ? player_1.first : player_2.first;
    }
    string get_loser(){
        return winner == player_1.second ? player_2.first : player_1.first;
    }
    string get_player_x(){
        return player_1.second == 'X' ? player_1.first : player_2.first;
    }
    string get_player_o(){
        return player_1.second == 'O' ? player_1.first : player_2.first;
    }
    string get_board(){
        // return : X012O65, where the next numbers are the positions where X and O are
        string xs, os;
        for (int i = 0; i < 3; i++){
            for (int j = 0; j < 3; j++){
                if (board[i][j] == 'X')
                    xs += to_string(i*3+j);
                else if (board[i][j] == 'O')
                    os += to_string(i*3+j);
            }
        }
        return "X" + xs + "O" + os;
    }
    static void print_board(string state){
        // decode and print X012O65, where the next numbers are the positions where X and O are
        size_t i = state.find("O");
        string board_(9, '-');
        cout << "Tic Tac Toe" << endl;
        for (int j = 1; j < i; j++) board_[state[j]-'0']= 'X';
        for (int j = i+1; j < state.size(); j++) board_[state[j]-'0']= 'O';
        for (int j = 0; j < 3; j++){
            for (int k = 0; k < 3; k++){
                cout << board_[j*3+k] << " ";
            }
            cout << endl;
        }
    }
    static void print_board(string state, string player_x, string player_o){
        TicTacToe::print_board(state);
        cout << "Player X: " << player_x << endl;
        cout << "Player O: " << player_o << endl;
    }
};

#endif // TTT_H