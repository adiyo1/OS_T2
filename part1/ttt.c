#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void print_error_and_exit() {
    printf("Error\n");
    exit(1);
}

int check_win(char board[9], char player) {
    int win_conditions[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},  // rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},  // columns
        {0, 4, 8}, {2, 4, 6}              // diagonals
    };
    for (int i = 0; i < 8; i++) {
        if (board[win_conditions[i][0]] == player &&
            board[win_conditions[i][1]] == player &&
            board[win_conditions[i][2]] == player) {
            return 1;
        }
    }
    return 0;
}

int check_draw(char board[9]) {
    for (int i = 0; i < 9; i++) {
        if (board[i] == ' ') {
            return 0;
        }
    }
    return 1;
}

void play_ttt(char* strategy) {
    if (strlen(strategy) != 9) {
        print_error_and_exit();
    }

    for (int i = 0; i < 9; i++) {
        if (!isdigit(strategy[i]) || strategy[i] == '0') {
            print_error_and_exit();
        }
        for (int j = i + 1; j < 9; j++) {
            if (strategy[i] == strategy[j]) {
                print_error_and_exit();
            }
        }
    }

    char board[9];
    for (int i = 0; i < 9; i++) {
        board[i] = ' ';
    }

    while (1) {
        // Program's move
        for (int i = 0; i < 9; i++) {
            int pos = strategy[i] - '1';
            if (board[pos] == ' ') {
                board[pos] = 'X';
                printf("%d\n", pos + 1);
                fflush(stdout);
                break;
            }
        }

        if (check_win(board, 'X')) {
            printf("I win\n");
            return;
        }
        if (check_draw(board)) {
            printf("Draw\n");
            return;
        }

        // Player's move
        int player_move;
        if (scanf("%d", &player_move) != 1) {
            print_error_and_exit();
        }
        if (player_move < 1 || player_move > 9 || board[player_move - 1] != ' ') {
            print_error_and_exit();
        }

        board[player_move - 1] = 'O';

        if (check_win(board, 'O')) {
            printf("I lose\n");
            return;
        }
        if (check_draw(board)) {
            printf("Draw\n");
            return;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_error_and_exit();
    }
    play_ttt(argv[1]);
    return 0;
}