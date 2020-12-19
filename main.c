#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <curses.h>

WINDOW * mainwin;
int tick = 0;

void drawchar(char c, int x, int y, int color) {
    attron(COLOR_PAIR(color));
    mvaddch(y, x, c);
    attroff(COLOR_PAIR(color));
}


void finish() {
    clear();
    refresh();
    delwin(mainwin);
    endwin();
    refresh();
    exit(0);
}


#define CHAR_PLAYER '@'
#define CHAR_FLOOR '.'
#define CHAR_WALL '#'
#define CHAR_SPAWN 'S'

#define WINDOW_WIDTH 80
#define WINDOW_HEIGHT 24
#define SCREEN_WIDTH 17
#define SCREEN_HEIGHT 9

char map[] = (
    "###################################################################\n"
    "S...................................###############################\n"
    "#########.......#################...###############################\n"
    "#########.......#################...###############################\n"
    "#########.......#################...###############################\n"
    "#########.......#################...###############################\n"
    "#########......s#################...###############################\n"
    "#########..###################.........############################\n"
    "#########..###################......................###############\n"
    "#########..###################....z.................###############\n"
    "#########........#############.........############################\n"
    "#########........#####################.############################\n"
    "#########..####..#####################.############################\n"
    "#########..####..#####################.############################\n"
    "#########..............................############################\n"
    "###################################################################\n"
);

int map_width = 0, map_height = 0;
char *current_message = 0;

int player_x = 0, player_y = 0, player_health = 100;

#define REGEN_HEALTH_EVERY_TICKS 5
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MONSTER_ZOMBIE 'z'
#define MONSTER_SKELETON 's'

typedef struct Monster {
    char type;
    int x;
    int y;
    
    int slowness; // tick%slowness != 0, monster is allowed to move
    int speed; // monster gets to move speed times per tick

    int shoot_every;
    int max_shoot_distance;

    int melee_damage;
    int shoot_damage;
} Monster;

#define MAX_MONSTERS 32
Monster monsters[MAX_MONSTERS];


char getcharat(int x, int y) {
    return (
        x >= 0 && x < map_width && y >= 0 && y < map_height ?
        *(map + x + (y * (map_width + 1))) :
        CHAR_WALL
    );
}

char setcharat(int x, int y, char c) {
    *(map + x + (y * (map_width + 1))) = c;
}


void parsemap() {
    memset(monsters, 0, sizeof(Monster) * MAX_MONSTERS);
    map_width = strstr(map, "\n") - map;
    map_height = strlen(map) / (map_width + 1); // +1 because of \n

    int monsteri = 0;

    char c, new_c;
    for (register int x = 0; x < map_width; x++) {
        for (register int y = 0; y < map_height; y++) {
            c = getcharat(x, y);
            new_c = c;
            switch (c) {
                case MONSTER_ZOMBIE:
                    monsters[monsteri].type = c;
                    monsters[monsteri].x = x;
                    monsters[monsteri].y = y;
                    monsters[monsteri].speed = 2;
                    monsters[monsteri].melee_damage = 15;
                    monsteri++;
                    new_c = CHAR_FLOOR;
                    break;

                case MONSTER_SKELETON:
                    monsters[monsteri].type = c;
                    monsters[monsteri].x = x;
                    monsters[monsteri].y = y;
                    monsters[monsteri].slowness = 2;
                    monsters[monsteri].shoot_every = 4;
                    monsters[monsteri].max_shoot_distance = 3;
                    monsters[monsteri].shoot_damage = 30;
                    monsteri++;
                    new_c = CHAR_FLOOR;
                    break;

                case CHAR_SPAWN:
                    player_x = x;
                    player_y = y;
                    new_c = CHAR_FLOOR;
                    break;
            }

            if (new_c && new_c != c) {
                setcharat(x, y, new_c);
            }
        }
    }

}


#define KEY_CONTROL 27
#define KEY_ARROW_DOWN 66
#define KEY_ARROW_UP 65
#define KEY_ARROW_LEFT 68
#define KEY_ARROW_RIGHT 67

#define IS_TRAVERSABLE(x, y) getcharat((x), (y)) != CHAR_WALL
#define IS_TRAVERSABLE_MONSTER(x, y) (IS_TRAVERSABLE((x), (y)) && !(player_x == (x) && player_y == (y)))


void updategame(int c) {
    switch (c) {
        // welcome to the game
        case -1:
            current_message = "Welcome to dvori. Use the arrow keys to move around.";
            break;

        case 'q':
            finish();
            break;

        case 'w':
keypress_up:
            if (IS_TRAVERSABLE(player_x, player_y -1))
                player_y--;
            break;

        case 's':
keypress_down:
            if (IS_TRAVERSABLE(player_x, player_y + 1))
                player_y++;
            break;

        case 'a':
keypress_left:
            if (IS_TRAVERSABLE(player_x - 1, player_y))
                player_x--;
            break;

        case 'd':
keypress_right:
            if (IS_TRAVERSABLE(player_x + 1, player_y))
                player_x++;
            break;

        case KEY_CONTROL:
            if (getchar() != 91) {
                finish();
                exit(0);
            }

            switch (getchar()) {
                case KEY_ARROW_DOWN:
                    goto keypress_down;
                case KEY_ARROW_UP:
                    goto keypress_up;
                case KEY_ARROW_RIGHT:
                    goto keypress_right;
                case KEY_ARROW_LEFT:
                    goto keypress_left;

            }
            break;
    }
    
    // update monsters
    Monster *monster;
    for (register int i = 0; i < MAX_MONSTERS; i++) {
        monster = &(monsters[i]);
        if (!monster || monster->type == 0) break;

        register int may_move = 1;

        const int monster_player_offset_x = abs(monster->x - player_x);
        const int monster_player_offset_y = abs(monster->y - player_y);

        // check if monster is close enough to shoot from
        if (monster->shoot_every && tick % monster->shoot_every == 0 && monster_player_offset_x <= monster->max_shoot_distance && monster_player_offset_y <= monster->max_shoot_distance) {
            // check if player is lined up to shoot
            if (abs(monster->x - player_x) <= 1) {
                if (monster->y < player_y) {
                    // shoot upwards
                    for (int my = monster->y; my < player_y; my++) {
                        // register to render
                    }
                } else if (monster->y > player_y) {
                    // shoot downwards
                    for (int my = player_y; my > player_y; my--) {
                        // register to render
                    }
                }

                current_message = "You were shot by a monster!";
                player_health -= monster->shoot_damage;
            }
        }

        // check if can damage
        if (monster->melee_damage && tick % 2 == 0) { // XXX: make the tick%2==0 configurable!
            if (monster_player_offset_x <= 1 && monster_player_offset_y <= 1) {
                current_message = "You were hit by a monster!";
                player_health -= monster->melee_damage;
            }
        }

        // only draw the monster if on screen
        if (may_move) {
            if (!(monster->slowness) || tick % monster->slowness == 0) {
                if (!(monster->speed) || tick % monster->speed != 0) {
                    if (monster_player_offset_x <= SCREEN_WIDTH && monster_player_offset_y <= SCREEN_HEIGHT) {
                        if (monster->x > player_x && IS_TRAVERSABLE_MONSTER(monster->x - 1, monster->y)) {
                            monster->x--;
                        } else if (monster->x < player_x && IS_TRAVERSABLE_MONSTER(monster->x + 1, monster->y)) {
                            monster->x++;
                        } else if (monster->y > player_y && IS_TRAVERSABLE_MONSTER(monster->x, monster->y - 1)) {
                            monster->y--;
                        } else if (monster->y < player_y && IS_TRAVERSABLE_MONSTER(monster->x, monster->y + 1)) {
                            monster->y++;
                        }
                    }
                }
            }
        }

        if (player_health <= 0) {
            printf("You lost the game.");
            finish();
        } else if (tick % REGEN_HEALTH_EVERY_TICKS == 0) {
            player_health = MIN(player_health + 5, 100);
        }
    }

    tick++;
}


void drawscreen() {
    clear();

    for (register int i = 0; i < WINDOW_WIDTH; i++) drawchar('-', i, 0, 1);
    for (register int i = 0; i < WINDOW_WIDTH; i++) drawchar('-', i, WINDOW_HEIGHT - 1, 1);
    for (register int i = 0; i < WINDOW_HEIGHT; i++) drawchar('|', 0, i, 1);
    for (register int i = 0; i < WINDOW_HEIGHT; i++) drawchar('|', WINDOW_WIDTH - 1, i, 1);

    const int start_window_x = (WINDOW_WIDTH - SCREEN_WIDTH) / 2;
    const int start_window_y = (WINDOW_HEIGHT - SCREEN_HEIGHT) / 2;
    const int screen_player_x = SCREEN_WIDTH / 2;
    const int screen_player_y = SCREEN_HEIGHT / 2;
    char c;

    for (register int x = 0; x < SCREEN_WIDTH; x++) {
        for (register int y = 0; y < SCREEN_HEIGHT; y++) {
            c = getcharat(player_x + x - screen_player_x, player_y + y - screen_player_y);
            drawchar(c, start_window_x + x, start_window_y + y, 1);
        }
    }

    Monster *monster;
    for (register int i = 0; i < MAX_MONSTERS; i++) {
        monster = &(monsters[i]);
        if (!monster || monster->type == 0) break;

        // only draw the monster if on screen
        if (abs(monster->x - player_x) <= screen_player_x && abs(monster->y - player_y) <= screen_player_y) {
            drawchar(monster->type, monster->x + start_window_x - player_x + screen_player_x, monster->y + start_window_y - player_y + screen_player_y, 1);
        }
    }

    if (current_message) {
        attron(COLOR_PAIR(1));
        mvaddstr(WINDOW_HEIGHT - 3, 4, current_message);
        attroff(COLOR_PAIR(1));
        current_message = 0;
    }

    drawchar(CHAR_PLAYER, start_window_x + screen_player_x, start_window_y + screen_player_y, 1);
    move(start_window_y + screen_player_y, start_window_x + screen_player_x);
}


int main() {
    // TODO: delete this for sall
    struct termios info;
    tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin */
    info.c_lflag &= ~ICANON;      /* disable canonical mode */
    info.c_cc[VMIN] = 1;          /* wait until at least one keystroke available */
    info.c_cc[VTIME] = 0;         /* no timeout */
    tcsetattr(0, TCSANOW, &info); /* set immediately */

    if ((mainwin = initscr()) == NULL) {
	fprintf(stderr, "Error initialising ncurses.\n");
	exit(EXIT_FAILURE);
    }

    parsemap();
    updategame(-1);
    drawscreen();
    refresh();
    while (1) {
        char c = getchar();
        updategame(c);
        drawscreen();
        refresh();
    }
    
    finish();
}
