#include <conio.h>
#include <cbm.h>
#include <stdio.h>
#include <string.h>

#define PEEK(addr) (*(u8*)(addr))
#define POKE(addr, val) (*(u8*)(addr) = (val))

#define SCREEN 0x0400
#define COLOR  0xD800

#define WIDTH 40
#define HEIGHT 25

#define CHAR_EMPTY 0x20
#define CHAR_BLOCK 0x23 // '#'
#define CHAR_SEL   0x2E // '.'
#define CHAR_O     0x4F // 'O'
#define CHAR_X     0x58 // 'X'

#define COLOR_BG    11 // dark gray
#define COLOR_BLOCK 15 // light gray
#define COLOR_O     10 // light red
#define COLOR_X     7  // yellow

typedef char i8;
typedef short i16;
typedef unsigned char u8;
typedef unsigned short u16;

#define BOARD_POS_X (WIDTH / 2 - 7)
#define BOARD_POS_Y (HEIGHT / 2 - 7)

u8 shouldQuit = 0;

u8 board[9] = {
    CHAR_EMPTY, CHAR_EMPTY, CHAR_EMPTY,
    CHAR_EMPTY, CHAR_EMPTY, CHAR_EMPTY,
    CHAR_EMPTY, CHAR_EMPTY, CHAR_EMPTY
};

u8 player = CHAR_X;
u8 selected = 4;

void clear_screen(void) {
    memset((u8*)SCREEN, CHAR_EMPTY, WIDTH * HEIGHT);
    memset((u8*)COLOR, COLOR_BG, WIDTH * HEIGHT);
}

void draw_board(void) {
    u8 y, x;
    for (y = 0; y < 13; y += 1) {
        for (x = 0; x < 13; x += 1) {
            const u16 ind = (y + BOARD_POS_Y) * WIDTH + (x + BOARD_POS_X);
            const u16 pos = SCREEN + ind;
            const u16 col = COLOR + ind;
            if (0 == x % 4 || 0 == y % 4) {
                POKE(pos, CHAR_BLOCK);
                POKE(col, COLOR_BLOCK);
            } else {
                POKE(pos, CHAR_EMPTY);
                POKE(col, COLOR_BG);
            }
        }
    }
}

void draw_ui(void) {
    i8 turnStr[8] = "TURN: ";
    u8 i;
    turnStr[6] = CHAR_X == player ? 'X' : 'O';
    for (i = 0; i < 7; i += 1) {
        POKE(SCREEN + i, turnStr[i]);
        POKE(COLOR + i, 1); // white
    }

    // for (i = 0; i < 9; i += 1) {
    //     POKE(SCREEN + i, board[i]);
    //     POKE(COLOR + i, 1);
    // }
}

void draw_selected(u8 playerChar, u8 selChar, u8 drawCol) {
    u8 y, x, i, py, px;
    for (y = 0, i = 0, py = BOARD_POS_Y + 1; y < 3; y += 1, py += 4) {
        for (x = 0, px = BOARD_POS_X + 1; x < 3; x += 1, i += 1, px += 4) {
            const u16 ind = py * WIDTH + px;
            const u16 pos = SCREEN + ind;
            const u16 col = COLOR + ind;

            if (selected != i) {
                POKE(pos + 1 + WIDTH * 1, board[i]);
                POKE(col + 1 + WIDTH * 1, CHAR_X == board[i] ? COLOR_X : (CHAR_O == board[i] ? COLOR_O : COLOR_BG));
                continue;
            }

            POKE(pos + 0 + WIDTH * 0, selChar);
            POKE(pos + 1 + WIDTH * 0, selChar);
            POKE(pos + 2 + WIDTH * 0, selChar);
            POKE(pos + 0 + WIDTH * 1, selChar);
            POKE(pos + 1 + WIDTH * 1, playerChar);
            POKE(pos + 2 + WIDTH * 1, selChar);
            POKE(pos + 0 + WIDTH * 2, selChar);
            POKE(pos + 1 + WIDTH * 2, selChar);
            POKE(pos + 2 + WIDTH * 2, selChar);

            POKE(col + 0 + WIDTH * 0, drawCol);
            POKE(col + 1 + WIDTH * 0, drawCol);
            POKE(col + 2 + WIDTH * 0, drawCol);
            POKE(col + 0 + WIDTH * 1, drawCol);
            POKE(col + 1 + WIDTH * 1, drawCol);
            POKE(col + 2 + WIDTH * 1, drawCol);
            POKE(col + 0 + WIDTH * 2, drawCol);
            POKE(col + 1 + WIDTH * 2, drawCol);
            POKE(col + 2 + WIDTH * 2, drawCol);
        }
    }
}

void handle_input(void) {
    if (!kbhit()) {
        return;
    }

    switch(cgetc()) {
        case 'a': {
            selected = 0 == selected % 3 ? selected + 2 : selected - 1;
        } break;
        case 'd': {
            selected = 2 == selected % 3 ? selected - 2 : selected + 1;
        } break;
        case 'w': {
            selected = selected < 3 ? selected + 6 : selected - 3;
        } break;
        case 's': {
            selected = selected >= 6 ? selected - 6 : selected + 3;
        } break;
        case ' ': {
            if (CHAR_EMPTY == board[selected]) {
                board[selected] = player;
                player = CHAR_X == player ? CHAR_O : CHAR_X;
            }
        } break;
        case 'p': {
            shouldQuit = 1;
        } break;
    }
}

u8 is_game_over(void) {
    u8 i, ec = 0;
    for (i = 0; i < 3; i += 1) {
        // vertical
        if (CHAR_EMPTY != board[i] && board[i] == board[i + 3] && board[i] == board[i + 6]) {
            return board[i];
        }

        // horizontal
        if (CHAR_EMPTY != board[i * 3] && board[i * 3] == board[i * 3 + 1] && board[i * 3] == board[i * 3 + 2]) {
            return board[i * 3];
        }
    }

    // diagonal
    if (CHAR_EMPTY != board[0] && board[0] == board[4] && board[0] == board[8]) {
        return board[0];
    }
    if (CHAR_EMPTY != board[2] && board[2] == board[4] && board[2] == board[6]) {
        return board[2];
    }

    for (i = 0; i < 9; i += 1) {
        if (CHAR_EMPTY == board[i]) {
            ec += 1;
        }
    }

    return 0 == ec ? 1 : 0;
}

i16 minimax(u8 depth, u8 maximizingPlayer, i16 alpha, i16 beta) {
    i16 score, bestScore;
    u8 i;

    u8 winner = is_game_over();
    if (winner) {
        return 1 == winner ? 0 : (CHAR_X == winner ? 10 - depth : depth - 10);
    }

    bestScore = maximizingPlayer ? -32000 : 32000;

    for (i = 0; i < 9; i += 1) {
        if (CHAR_EMPTY != board[i]) {
            continue;
        }

        board[i] = maximizingPlayer ? CHAR_X : CHAR_O;
        score = minimax(depth + 1, !maximizingPlayer, alpha, beta);
        board[i] = CHAR_EMPTY;

        if (maximizingPlayer) {
            if (score > bestScore) bestScore = score;
            if (bestScore >= beta) break; // beta cutoff
            if (bestScore > alpha) alpha = bestScore;
        } else {
            if (score < bestScore) bestScore = score;
            if (bestScore <= alpha) break; // alpha cutoff
            if (bestScore < beta) beta = bestScore;
        }
    }

    return bestScore;
}

void handle_ai(u8 xMove) {
    u8 i, move = 0, ec = 0;
    i16 score, bestScore = xMove ? -32000 : 32000;

    for (i = 0; i < 9; i += 1) {
        if (CHAR_EMPTY != board[i]) {
            ec += 1;
        }
    }

    // precomputed first move, either take center or corner
    if (ec <= 1) {
        if (CHAR_EMPTY == board[4]) {
            move = 4;
        } else {
            for (i = 0; i < 9; i += 2) {
                if (CHAR_EMPTY == board[i]) {
                    move = i;
                    break;
                }
            }
        }

        goto RET;
    }

    // normal minimax for other moves
    for (i = 0; i < 9; i += 1) {
        if (CHAR_EMPTY != board[i]) {
            continue;
        }

        board[i] = xMove ? CHAR_X : CHAR_O;
        score = minimax(0, !xMove, -32000, 32000);
        board[i] = CHAR_EMPTY;

        if ((xMove && score > bestScore) || (!xMove && score < bestScore)) {
            bestScore = score;
            move = i;
        }
    }

    RET:
    board[move] = xMove ? CHAR_X : CHAR_O;
    player = xMove ? CHAR_O : CHAR_X;
}

void main() {
    u8 winner;

    clrscr();
    bgcolor(COLOR_BG);
    bordercolor(COLOR_BG);
    
    clear_screen();
    draw_board();

    for (; !shouldQuit && !winner; winner = is_game_over()) {
        // wait for vblank
        while (0 != PEEK(0xD012)) { }

        // erase previous
        draw_selected(CHAR_EMPTY, CHAR_EMPTY, COLOR_BG);

        // update game
        if (CHAR_X == player) {
            handle_input();
        } else {
            handle_ai(0);
        }

        // draw current
        draw_ui();
        if (CHAR_X == player) {
            draw_selected(CHAR_X, CHAR_SEL, COLOR_X);
        } else {
            draw_selected(CHAR_O, CHAR_SEL, COLOR_O);
        }
    }
    
    clrscr();
    if (1 == winner) {
        printf("GAME OVER, DRAW");
    } else if (winner) {
        printf("GAME OVER, VICTOR: %c", CHAR_X == winner ? 'X' : 'O');
    } else {
        printf("GAME OVER");
    }
    cgetc();
}
