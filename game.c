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

#define EMPTY 0x20
#define GROUND 0x5E
#define PLATFORM 0x5F

#define PLAYER_CHAR 0x23 // '#'
#define PLAYER_COLOR 2   // red
#define BG_COLOR 6       // blue
#define GROUND_COLOR 3   // cyan
#define PLATFORM_COLOR 5 // green

typedef unsigned char u8;
typedef unsigned short u16;

typedef struct player {
    u8 x;
    u8 y;
    u8 vx;
    u8 vy;
    u8 frame;
    u8 dir; // 0 = left, 1 = right
} Player;
Player player = {WIDTH / 2, HEIGHT - 2, 0, 0, 0, 1};

u8 level[WIDTH * HEIGHT] = {0};

// game state
u8 score = 0;
u8 lives = 3;
u8 gameRunning = 1;

void draw_player(void) {
    const u16 pos = SCREEN + player.y * WIDTH + player.x;
    const u16 col = COLOR + player.y * WIDTH + player.x;
    
    // simple animation
    const u8 charToDraw = (player.frame < 8 ? PLAYER_CHAR : 0x24); // '$'
    
    POKE(pos, charToDraw);
    POKE(col, PLAYER_COLOR);
    
    // draw direction indicator
    if (player.dir) {
        POKE(pos + 1, 0x3E); // '>'
        POKE(col + 1, PLAYER_COLOR);
    } else {
        POKE(pos - 1, 0x3C); // '<'
        POKE(col - 1, PLAYER_COLOR);
    }
}

void draw_level(void) {
    // extremely expensive
    u16 i;
    for (i = 0; i < WIDTH * HEIGHT; i += 1) {
        const u8 screenChar = level[i];
        if (EMPTY != screenChar) {
            POKE(SCREEN + i, screenChar);
            POKE(COLOR + i, (GROUND == screenChar) ? GROUND_COLOR : PLATFORM_COLOR);
        }
    }

    // better performance but no different colors
    // memcpy((u8*)SCREEN, level, WIDTH * HEIGHT);
    // memset((u8*)COLOR, PLATFORM_COLOR, WIDTH * HEIGHT);
}

void clear_screen(void) {
    memset((u8*)SCREEN, EMPTY, WIDTH * HEIGHT);
    memset((u8*)COLOR, BG_COLOR, WIDTH * HEIGHT);
}

u8 check_collision(u8 x, u8 y) {
    return (x >= WIDTH || y >= HEIGHT) ? 1 : EMPTY != level[y * WIDTH + x];
}

void init_level(void) {
    u16 i;
    u8 x;

    // clear level
    for (i = 0; i < WIDTH * HEIGHT; i += 1) {
        level[i] = EMPTY;
    }
    
    // ground
    for (x = 0; x < WIDTH; x += 1) {
        level[(HEIGHT - 1) * WIDTH + x] = GROUND;
    }
    
    // platforms
    for (x = 5; x < 15; x += 1) {
        level[(HEIGHT - 8) * WIDTH + x] = PLATFORM;
    }
    for (x = 20; x < 30; x += 1) {
        level[(HEIGHT - 12) * WIDTH + x] = PLATFORM;
    }
    for (x = 10; x < 20; x += 1) {
        level[(HEIGHT - 16) * WIDTH + x] = PLATFORM;
    }
}

void handle_input(void) {
    if (kbhit()) {
        const char key = cgetc();
        switch(key) {
            case 'a': {
                if (player.dir) {
                    player.x += 1;
                }
                player.vx = -1;
                player.dir = 0;
            } break;
            case 'd': {
                if (!player.dir) {
                    player.x -= 1;
                }
                player.vx = 1;
                player.dir = 1;
            } break;
            case 'w': {
                player.vy = -1;
            } break;
            case 's': {
                player.vy = 1;
            } break;
            case 'p': {
                gameRunning = 0;
            } break;
        }
    } else {
        player.vx = player.vy = 0;
    }
}

void update_player(void) {
    // horizontal movement
    if (0 != player.vx) {
        const u8 nx = player.x + player.vx;
        if (!check_collision(nx, player.y) &&
            !check_collision(nx + (player.dir ? 1 : -1), player.y)
        ) {
            player.x = nx;
        }
    }
    
    // vertical movement
    if (0 != player.vy) {
        const u8 ny = player.y + player.vy;
        if (!check_collision(player.x, ny) &&
            !check_collision(player.x + (player.dir ? 1 : -1), ny)
        ) {
            player.y = ny;
        }
    }
    
    // screen wrapping
    if (player.x >= WIDTH) {
        player.x = 1;
    } else if (player.x < 1) {
        player.x = WIDTH - 1;
    }
    if (player.y >= HEIGHT) {
        player.y = HEIGHT - 1;
    } else if (player.y < 1) {
        player.y = 1;
    }
    
    // animation frame
    player.frame = (player.frame + 1) % 16;
}

void draw_hud(void) {
    char scoreStr[10];
    char livesStr[10];
    u8 i;

    // draw score
    sprintf(scoreStr, "SCORE:%d", score);
    for (i = 0; i < strlen(scoreStr); i += 1) {
        POKE(SCREEN + i, scoreStr[i]);
        POKE(COLOR + i, 1); // white
    }
    
    // draw lives
    sprintf(livesStr, "LIVES:%d", lives);
    for (i = 0; i < strlen(livesStr); i += 1) {
        POKE(SCREEN + WIDTH - strlen(livesStr) + i, livesStr[i]);
        POKE(COLOR + WIDTH - strlen(livesStr) + i, 1); // white
    }
}

void main() {
    clrscr();
    bgcolor(BG_COLOR);
    bordercolor(BG_COLOR);
    
    init_level();
    clear_screen();
    draw_level();
    
    while (gameRunning) {
        // wait for vblank
        while (0 != PEEK(0xD012)) { }
        
        // erase old player
        POKE(SCREEN + player.y * WIDTH + player.x, EMPTY);
        POKE(SCREEN + player.y * WIDTH + player.x + (player.dir ? 1 : -1), EMPTY);

        // update
        handle_input();
        update_player();
        
        // draw
        draw_player();
        draw_hud();
        
        // I was going to make this into a platformer game but didn't
        score = (HEIGHT - player.y) / 2;
    }
    
    // game over screen
    clrscr();
    printf("GAME OVER\nFINAL SCORE: %d", score);
    cgetc();
}