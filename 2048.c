#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
int _getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

#define SIZE 4
#define WIN_TILE 2048

int board[SIZE][SIZE];
int score = 0;

/* --- ANSI color helpers --- */
const char *tile_color(int val) {
    switch (val) {
        case 0:    return "\033[48;5;235m";    /* empty dark */
        case 2:    return "\033[48;5;229m\033[38;5;235m"; /* cream */
        case 4:    return "\033[48;5;222m\033[38;5;235m";
        case 8:    return "\033[48;5;215m\033[38;5;16m";  /* orange */
        case 16:   return "\033[48;5;209m\033[38;5;16m";
        case 32:   return "\033[48;5;203m\033[38;5;16m";  /* red */
        case 64:   return "\033[48;5;196m\033[38;5;16m";
        case 128:  return "\033[48;5;227m\033[38;5;235m"; /* gold */
        case 256:  return "\033[48;5;226m\033[38;5;235m";
        case 512:  return "\033[48;5;220m\033[38;5;235m";
        case 1024: return "\033[48;5;46m\033[38;5;16m";   /* green */
        case 2048: return "\033[48;5;51m\033[38;5;16m";   /* cyan WIN */
        default:   return "\033[48;5;129m\033[38;5;16m";  /* purple 4096+ */
    }
}

void clear_screen()    { printf("\033[2J\033[H"); }
void bold()            { printf("\033[1m"); }
void reset()           { printf("\033[0m"); }

/* --- game logic --- */

void add_random_tile() {
    int empty[SIZE * SIZE][2];
    int n = 0;
    for (int r = 0; r < SIZE; r++)
        for (int c = 0; c < SIZE; c++)
            if (board[r][c] == 0) {
                empty[n][0] = r;
                empty[n][1] = c;
                n++;
            }
    if (n == 0) return;
    int idx = rand() % n;
    board[empty[idx][0]][empty[idx][1]] = (rand() % 10 < 9) ? 2 : 4;
}

void init_board() {
    memset(board, 0, sizeof(board));
    score = 0;
    srand((unsigned)time(NULL));
    add_random_tile();
    add_random_tile();
}

/* slide one row left, merge tiles, return whether anything moved */
int slide_row_left(int row[SIZE]) {
    int moved = 0;
    /* compact */
    int pos = 0;
    for (int i = 0; i < SIZE; i++)
        if (row[i] != 0) row[pos++] = row[i];
    while (pos < SIZE) row[pos++] = 0;

    /* merge */
    for (int i = 0; i < SIZE - 1; i++) {
        if (row[i] && row[i] == row[i + 1]) {
            row[i] *= 2;
            score += row[i];
            row[i + 1] = 0;
            moved = 1;
        }
    }

    /* compact again */
    pos = 0;
    for (int i = 0; i < SIZE; i++)
        if (row[i] != 0) row[pos++] = row[i];
    while (pos < SIZE) row[pos++] = 0;

    return moved;
}

int move_left() {
    int moved = 0;
    for (int r = 0; r < SIZE; r++) {
        int orig[SIZE];
        memcpy(orig, board[r], sizeof(orig));
        if (slide_row_left(board[r]) || memcmp(orig, board[r], sizeof(orig)))
            moved = 1;
    }
    return moved;
}

int move_right() {
    int moved = 0;
    for (int r = 0; r < SIZE; r++) {
        int rev[SIZE];
        for (int i = 0; i < SIZE; i++) rev[i] = board[r][SIZE - 1 - i];
        int orig_after = rev[0];
        (void)orig_after;
        int orig[SIZE];
        memcpy(orig, rev, sizeof(orig));
        if (slide_row_left(rev) || memcmp(orig, rev, sizeof(orig)))
            moved = 1;
        for (int i = 0; i < SIZE; i++) board[r][SIZE - 1 - i] = rev[i];
    }
    return moved;
}

int move_up() {
    int moved = 0;
    for (int c = 0; c < SIZE; c++) {
        int col[SIZE];
        for (int r = 0; r < SIZE; r++) col[r] = board[r][c];
        int orig[SIZE];
        memcpy(orig, col, sizeof(orig));
        if (slide_row_left(col) || memcmp(orig, col, sizeof(orig)))
            moved = 1;
        for (int r = 0; r < SIZE; r++) board[r][c] = col[r];
    }
    return moved;
}

int move_down() {
    int moved = 0;
    for (int c = 0; c < SIZE; c++) {
        int col[SIZE];
        for (int r = 0; r < SIZE; r++) col[r] = board[SIZE - 1 - r][c];
        int orig[SIZE];
        memcpy(orig, col, sizeof(orig));
        if (slide_row_left(col) || memcmp(orig, col, sizeof(orig)))
            moved = 1;
        for (int r = 0; r < SIZE; r++) board[SIZE - 1 - r][c] = col[r];
    }
    return moved;
}

int can_move() {
    for (int r = 0; r < SIZE; r++)
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] == 0) return 1;
            if (c + 1 < SIZE && board[r][c] == board[r][c + 1]) return 1;
            if (r + 1 < SIZE && board[r][c] == board[r + 1][c]) return 1;
        }
    return 0;
}

int has_won() {
    for (int r = 0; r < SIZE; r++)
        for (int c = 0; c < SIZE; c++)
            if (board[r][c] >= WIN_TILE) return 1;
    return 0;
}

void print_tile(int val) {
    printf("%s", tile_color(val));
    if (val == 0)
        printf("     . ");
    else
        printf(" %5d ", val);
    reset();
}

void draw() {
    clear_screen();
    bold();
    printf("         2048\n\n");
    reset();
    printf("Score: %d\n\n", score);

    printf("    +-------+-------+-------+-------+\n");
    for (int r = 0; r < SIZE; r++) {
        printf("    |");
        for (int c = 0; c < SIZE; c++) {
            print_tile(board[r][c]);
            printf("|");
        }
        printf("\n    +-------+-------+-------+-------+\n");
    }

    printf("\n  W/A/S/D or Arrow keys to move  R: restart  Q: quit\n");
}

int main() {
    init_board();
    int won_shown = 0;

    while (1) {
        draw();
        if (!won_shown && has_won()) {
            printf("\n  *** You reached 2048! Keep going or press Q to quit. ***\n");
            won_shown = 1;
        }
        if (!can_move()) {
            printf("\n  *** Game Over! No moves left. ***\n");
            printf("Press R to restart or Q to quit.\n");
        }

        int ch = _getch();
        if (ch == 224 || ch == 0) {  /* arrow key prefix on Windows */
            ch = _getch();
            switch (ch) {
                case 72: ch = 'w'; break; /* up    */
                case 80: ch = 's'; break; /* down  */
                case 75: ch = 'a'; break; /* left  */
                case 77: ch = 'd'; break; /* right */
            }
        }
        /* handle ANSI arrow escapes */
        if (ch == 27) {
            _getch();                /* skip '[' */
            int dir = _getch();
            switch (dir) {
                case 'A': ch = 'w'; break;
                case 'B': ch = 's'; break;
                case 'C': ch = 'd'; break;
                case 'D': ch = 'a'; break;
            }
        }

        switch (ch) {
            case 'w': case 'W':
                if (move_up()) add_random_tile();
                break;
            case 's': case 'S':
                if (move_down()) add_random_tile();
                break;
            case 'a': case 'A':
                if (move_left()) add_random_tile();
                break;
            case 'd': case 'D':
                if (move_right()) add_random_tile();
                break;
            case 'r': case 'R':
                init_board();
                won_shown = 0;
                break;
            case 'q': case 'Q':
                clear_screen();
                printf("Final score: %d. Bye!\n", score);
                return 0;
        }
    }
}
