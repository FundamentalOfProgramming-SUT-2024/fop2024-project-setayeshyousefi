
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <ncurses.h>

enum Difficulty { EASY, MEDIUM, HARD };
enum Difficulty difficulty = MEDIUM;
enum FoodType {
    NORMAL_FOOD,    // 0
    SPECIAL_FOOD,   // 1
    MAGICAL_FOOD,   // 2
    ROTTEN_FOOD     // 3
};

enum PotionType {
    HEALTH_POT,     // 0
    SPEED_POT,      // 1
    DAMAGE_POT      // 2
};
//enum WeaponType { MACE, DAGGER, MAGIC_WAND, NORMAL_ARROW, SWORD };

#define USER_FILE "users.txt"
#define MAX_USERNAME_LEN 50
#define MAX_EMAIL_LEN 100
#define MAX_PASSWORD_LEN 50
#define MAX_ROOM_ATTEMPTS 500
#define MIN_ROOMS 5
#define MAX_ROOMS 10
#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
#define MAX_FOOD_TYPES 4
#define FOOD_DEGRADE_INTERVAL 50


char playerUsername[MAX_USERNAME_LEN];
const char *songs[] = {"song1.mp3", "song2.mp3", "song3.mp3"};
int selected_song = 0;
int py, px;
int sy, sx;
char race[9] = {0};
int att;
int hp;
int mana;
int stealth;
bool t_placed;
bool p_placed;
int r_placed;
int dlvl;
int turns = 0;
int lvl_turns;
int m_defeated;
int rows = 23;
int cols = 80;
char map[23][80];
char obj[23][80];
int playerColor = 0;
int gold = 0;
int experience = 0;

char state[5] = {0};

//struct Weapon {
//    enum WeaponType type;
//    int damage;
//    int range;
//    int quantity;
//    bool throwable;
//};

//struct Weapon currentWeapon = {MACE, 5, 0, 1, false};  // Default weapon is Mace
//struct Weapon inventory[5] = {
//        {MACE, 5, 0, 1, false},
//        {DAGGER, 12, 5, 10, true},
//        {MAGIC_WAND, 15, 10, 8, true},
//        {NORMAL_ARROW, 5, 5, 20, true},
//        {SWORD, 10, 0, 1, false}
//};
struct ScoreboardEntry {
    char username[MAX_USERNAME_LEN];
    int scores[1];
    int gold;
    int finished_games;
    int experience;
};
struct Inventory {
    int potions[3];
    int food[4];
    int potion_timer[3];
    int food_degrade_timer;
};
struct monsters
{
    int y;
    int x;
    int lvl;
    int type;
    int awake;
    int chase_steps;
};

struct monsters monster[20];
struct Inventory inv = {{0}, {0}, {0}, 0};





bool in_treasure_room = false;
int treasure_room_enemies = 0;

void display_action_message(const char *format, ...) {
    va_list args;
    va_start(args, format);

    move(0, 0);
    clrtoeol();
    vw_printw(stdscr, format, args);

    va_end(args);
    refresh();
    napms(2000);
    move(0, 0);
    clrtoeol();
    refresh();
}
void update_scoreboard(const char *username, int score, int gold, int time_played) {
    FILE *file = fopen("scoreboard.txt", "a");
    if (!file) {
        printw("Error: Could not update scoreboard.\n");
        return;
    }

    struct ScoreboardEntry entry;
    strncpy(entry.username, username, MAX_USERNAME_LEN - 1);
    entry.username[MAX_USERNAME_LEN - 1] = '\0';
    entry.scores[0] = score;
    entry.gold = gold;
    entry.finished_games = 1;
    entry.experience = time_played;

    fprintf(file, "%s %d %d %d %d\n",
            entry.username, entry.scores[0], entry.gold,
            entry.finished_games, entry.experience);
    fclose(file);
}
//void select_weapon() {
//    clear();
//    printw("Select a weapon:\n");
//    for (int i = 0; i < 5; i++) {
//        printw("%d. %s (Damage: %d, Range: %d, Quantity: %d)\n", i + 1,
//               (i == 0) ? "Mace" : (i == 1) ? "Dagger" : (i == 2) ? "Magic Wand" :
//                                                         (i == 3) ? "Arrow" : "Sword",
//               inventory[i].damage, inventory[i].range, inventory[i].quantity);
//    }
//    printw("Enter weapon number: ");
//    refresh();
//
//    int choice = getch() - '1';
//    if (choice >= 0 && choice < 5 && inventory[choice].quantity > 0) {
//        currentWeapon = inventory[choice];
//        display_action_message("Default weapon changed!");
//    } else {
//        display_action_message("Invalid selection or weapon unavailable.");
//    }
//}

//void attack(int direction) {
//    if (currentWeapon.throwable) {
//        if (currentWeapon.quantity > 0) {
//            currentWeapon.quantity--;
//            display_action_message("Weapon thrown!");
//        } else {
//            display_action_message("No more of this weapon left!");
//        }
//    } else {
//        display_action_message("Melee attack performed!");
//    }
//}
void save_game(int rows, int cols, char (* obj)[cols]) {
    FILE *file = fopen("savegame.txt", "w");
    if (!file) {
        printw("Error: Could not save game.\n");
        return;
    }
    fprintf(file, "%d %d %d %d\n",
            inv.food[0], inv.food[1],
            inv.food[2], inv.food[3]);
    fprintf(file, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
            py, px, att, hp, mana, stealth, dlvl, turns,
            lvl_turns, m_defeated, gold, experience);

    for (int i = 0; i < 20; i++) {
        fprintf(file, "%d %d %d %d %d\n",
                monster[i].y, monster[i].x, monster[i].lvl, monster[i].type, monster[i].awake);
    }

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (obj[y][x] == '^') {
                fprintf(file, "%d %d\n", y, x);
            }
        }
    }

    fclose(file);
    printw("Game saved successfully!\n");
}
int is_password_valid(const char *password) {
    int has_digit = 0, has_upper = 0, has_lower = 0;
    for (int i = 0; password[i] != '\0'; i++) {
        if (isdigit(password[i])) has_digit = 1;
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
    }
    return (has_digit && has_upper && has_lower && strlen(password) >= 7);
}

void generate_random_password(char *password) {
    const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    srand(time(NULL));
    do {
        for (int i = 0; i < 10; i++) {
            password[i] = chars[rand() % (strlen(chars))];
        }
        password[10] = '\0';
    } while (!is_password_valid(password));
}
void generate_treasure_room(int rows, int cols, char (*map)[cols]) {
    for(int y=0; y<rows; y++)
        for(int x=0; x<cols; x++)
            map[y][x] = ' ';

    int room_size = 15;
    int start_y = (rows - room_size)/2;
    int start_x = (cols - room_size)/2;

    for(int y=start_y; y<start_y+room_size; y++) {
        for(int x=start_x; x<start_x+room_size; x++) {
            if(y == start_y || y == start_y+room_size-1) map[y][x] = '_';
            else if(x == start_x || x == start_x+room_size-1) map[y][x] = '|';
            else map[y][x] = (rand() % 5 == 0) ? '^' : '.'; // 20% traps
        }
    }

    treasure_room_enemies = 10 + rand()%5;
    for(int i=0; i<treasure_room_enemies; i++) {
        int ey, ex;
        do {
            ey = start_y + 1 + rand()%(room_size-2);
            ex = start_x + 1 + rand()%(room_size-2);
        } while(map[ey][ex] != '.');
        map[ey][ex] = 't';
    }

    py = start_y + 2;
    px = start_x + 2;
    in_treasure_room = true;
}
//void consume_item(int type) {
//    switch(type) {
//        case '1':
//            if(inv.potions[0] > 0) {
//                inv.potions[0]--;
//                inv.potion_timer[0] = 30;
//                mvprintw(0, 0, "Health regeneration doubled!");
//            }
//            break;
//
//        case '2':
//            if(inv.potions[1] > 0) {
//                inv.potions[1]--;
//                inv.potion_timer[1] = 30;
//                mvprintw(0, 0, "Movement speed increased!");
//            }
//            break;
//
//        case '3':
//            if(inv.potions[2] > 0) {
//                inv.potions[2]--;
//                inv.potion_timer[2] = 30;
//                mvprintw(0, 0, "Damage output boosted!");
//            }
//            break;
//
//        case '4': case '5': case '6': case '7':
//        {
//            int food_type = type - '4';
//            if(inv.food[food_type] > 0) {
//                inv.food[food_type]--;
//                switch(food_type) {
//                    case 0: hp += 10; break;
//                    case 1: hp += 15; att += 3; break;
//                    case 2: hp += 10; stealth += 2; break;
//                    case 3: hp -= 20; break;
//                }
//            }
//            break;
//        }
//    }
//}
int is_email_valid(const char *email) {
    int at_count = 0, dot_count = 0;
    for (int i = 0; email[i] != '\0'; i++) {
        if (email[i] == '@') at_count++;
        if (email[i] == '.') dot_count++;
    }
    return (at_count == 1 && dot_count >= 1);
}

int is_username_taken(const char *username) {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) return 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char saved_username[MAX_USERNAME_LEN];
        sscanf(line, "%[^:]:", saved_username);
        if (strcmp(saved_username, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

void save_user(const char *username, const char *email, const char *password) {
    FILE *file = fopen(USER_FILE, "a");
    if (!file) {
        printf("Error: Could not open user file.\n");
        return;
    }
    fprintf(file, "%s:%s:%s\n", username, email, password);
    fclose(file);
}

int authenticate_user(const char *username, const char *password) {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) return 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char saved_username[MAX_USERNAME_LEN], saved_email[MAX_EMAIL_LEN], saved_password[MAX_PASSWORD_LEN];
        sscanf(line, "%[^:]:%[^:]:%[^\n]", saved_username, saved_email, saved_password);
        if (strcmp(saved_username, username) == 0 && strcmp(saved_password, password) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int auth_menu() {
    int choice;
    char username[MAX_USERNAME_LEN], email[MAX_EMAIL_LEN], password[MAX_PASSWORD_LEN];
    char confirm_password[MAX_PASSWORD_LEN];

    while (1) {
        clear();
        printw("\n\n\t\t\t\tSign Up / Login\n\n");
        printw("\t1. Sign Up\n");
        printw("\t2. Log In\n");
        printw("\t3. Play as Guest\n");
        printw("\t4. Exit\n");
        printw("\n\tEnter your choice: ");
        refresh();
        choice = getch() - '0';

        switch (choice) {
            case 1:
                clear();
                printw("\n\tEnter username: ");
                echo();
                scanw("%s", username);
                noecho();

                if (is_username_taken(username)) {
                    printw("\n\tUsername already taken. Please try again.\n");
                    getch();
                    break;
                }

                printw("\tEnter email: ");
                echo();
                scanw("%s", email);
                noecho();

                if (!is_email_valid(email)) {
                    printw("\n\tInvalid email format. Please try again.\n");
                    getch();
                    break;
                }

                printw("\tEnter password (or type 'gen' to generate one): ");
                echo();
                scanw("%s", password);
                noecho();

                if (strcmp(password, "gen") == 0) {
                    generate_random_password(password);
                    printw("\n\tGenerated password: %s\n", password);
                }

                if (!is_password_valid(password)) {
                    printw("\n\tPassword does not meet requirements. Please try again.\n");
                    getch();
                    break;
                }

                printw("\tConfirm password: ");
                echo();
                scanw("%s", confirm_password);
                noecho();

                if (strcmp(password, confirm_password) != 0) {
                    printw("\n\tPasswords do not match. Please try again.\n");
                    getch();
                    break;
                }

                save_user(username, email, password);
                strcpy(playerUsername, username);
                printw("\n\tSign up successful! Press any key to continue.\n");
                getch();
                return 1;

            case 2:
                clear();
                printw("\n\tEnter username: ");
                echo();
                scanw("%s", username);
                noecho();

                printw("\tEnter password: ");
                echo();
                scanw("%s", password);
                noecho();

                if (authenticate_user(username, password)) {
                    strcpy(playerUsername, username);
                    printw("\n\tLogin successful! Press any key to continue.\n");
                    getch();
                    return 1;
                } else {
                    printw("\n\tInvalid username or password. Please try again.\n");
                    getch();
                }
                break;

            case 3:
                strcpy(playerUsername, "Guest");
                return 1;

            case 4:
                return 0;

            default:
                printw("\n\tInvalid choice. Please try again.\n");
                getch();
                break;
        }
    }
}


void set_color(int color_pair) {
    attron(A_BOLD | COLOR_PAIR(color_pair));
}

void reset_color() {
    attroff(A_BOLD | COLOR_PAIR(WHITE));
}

int dungeon_draw(int rows, int cols, char (*map)[cols], char (*obj)[cols]) {
    move(0, 0);
    clrtobot();
    mvprintw(rows + 1, 0, " %s    HP: %d   Att: %d   Mana: %d   Gold: %d   XP: %d \t\t Dlvl: %d",
             race, hp, att, mana, gold, experience, dlvl);
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int draw_y = y + 1;
            char tile = map[y][x];

            switch (tile) {
                case '|':
                    attron(COLOR_PAIR(CYAN));
                    mvaddch(draw_y, x, '|');
                    attroff(COLOR_PAIR(CYAN));
                    break;
                case '_':
                    attron(COLOR_PAIR(CYAN));
                    mvaddch(draw_y, x, '_');
                    attroff(COLOR_PAIR(CYAN));
                    break;
                case '.':
                    mvaddch(draw_y, x, '.');
                    break;
                case '+':
                    attron(COLOR_PAIR(YELLOW));
                    mvaddch(draw_y, x, '+');
                    attroff(COLOR_PAIR(YELLOW));
                    break;
                case '#':
                    mvaddch(draw_y, x, '#');
                    break;
                case 'O':
                    attron(COLOR_PAIR(CYAN));
                    mvaddch(draw_y, x, 'O');
                    attroff(COLOR_PAIR(CYAN));
                    break;
                case '=':
                    attron(COLOR_PAIR(BLUE));
                    mvaddch(draw_y, x, '=');
                    attroff(COLOR_PAIR(BLUE));
                    break;
                case '>':
                    attron(A_BOLD);
                    mvaddch(draw_y, x, '>');
                    attroff(A_BOLD);
                    break;
                default:
                    mvaddch(draw_y, x, ' ');
                    break;
            }
        }
    }
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int draw_y = y + 1;
            char tile = map[y][x];
            char object = obj[y][x];


            if (object != ' ') {
                switch(object) {
                    case 't':
                        attron(COLOR_PAIR(RED) | A_BOLD);
                        mvaddch(draw_y, x, 'M');
                        attroff(COLOR_PAIR(RED) | A_BOLD);
                        break;
                    case 'N':
                        attron(COLOR_PAIR(GREEN));
                        mvaddch(draw_y, x, 'F');
                        attroff(COLOR_PAIR(GREEN));
                        break;
                    case 'S':
                        attron(COLOR_PAIR(YELLOW));
                        mvaddch(draw_y, x, 'C');
                        attroff(COLOR_PAIR(YELLOW));
                        break;
                    case 'M':
                        attron(COLOR_PAIR(MAGENTA));
                        mvaddch(draw_y, x, 'K');
                        attroff(COLOR_PAIR(MAGENTA));
                        break;
                    case 'R':
                        attron(COLOR_PAIR(RED));
                        mvaddch(draw_y, x, 'R');
                        attroff(COLOR_PAIR(RED));
                        break;
                    case '>':
                        attron(A_BOLD | COLOR_PAIR(WHITE));
                        mvaddch(draw_y, x, '>');
                        attroff(A_BOLD | COLOR_PAIR(WHITE));
                        break;
                    case '$':
                        attron(A_BOLD | COLOR_PAIR(YELLOW));
                        mvaddch(draw_y, x, '$');
                        attroff(A_BOLD | COLOR_PAIR(YELLOW));
                        break;
                    case 'H':
                        attron(COLOR_PAIR(RED));
                        mvaddch(draw_y, x, 'Q');
                        attroff(COLOR_PAIR(RED));
                        break;
                    case 'P':
                        attron(COLOR_PAIR(BLUE));
                        mvaddch(draw_y, x, 'Z');
                        attroff(COLOR_PAIR(BLUE));
                        break;
                    case 'D':
                        attron(COLOR_PAIR(MAGENTA));
                        mvaddch(draw_y, x, 'X');
                        attroff(COLOR_PAIR(MAGENTA));
                        break;
                    case '^':
                        attron(COLOR_PAIR(YELLOW));
                        mvaddch(draw_y, x, '^');
                        attroff(COLOR_PAIR(YELLOW));
                        break;
                }
            }
        }
    }
    if (py >= 0 && py < rows && px >= 0 && px < cols) {
        switch(playerColor){
            case 0:{
                attron(A_BOLD | COLOR_PAIR(WHITE));
                mvaddch(py + 1, px, '@');
                attron(A_BOLD | COLOR_PAIR(WHITE));
            }
            case 1:{
                attroff(A_BOLD | COLOR_PAIR(COLOR_RED));
                mvaddch(py + 1, px, '@');
                attroff(A_BOLD | COLOR_PAIR(COLOR_RED));
            }
            case 2:{
                attroff(A_BOLD | COLOR_PAIR(COLOR_GREEN));
                mvaddch(py + 1, px, '@');
                attroff(A_BOLD | COLOR_PAIR(COLOR_GREEN));
            }
            case 3: {
                attroff(A_BOLD | COLOR_PAIR(COLOR_BLUE));
                mvaddch(py + 1, px, '@');
                attroff(A_BOLD | COLOR_PAIR(COLOR_BLUE));
            }
            case 4:{
                attroff(A_BOLD | COLOR_PAIR(COLOR_CYAN));
                mvaddch(py + 1, px, '@');
                attroff(A_BOLD | COLOR_PAIR(COLOR_CYAN));
            }
        }
    }
//
//    mvprintw(rows + 2, 0, "Food: N[%d] 1 | S[%d] 2 | M[%d] 3 | R[%d] 4",
//             inv.food[NORMAL_FOOD],
//             inv.food[SPECIAL_FOOD],
//             inv.food[MAGICAL_FOOD],
//             inv.food[ROTTEN_FOOD]);
//    mvprintw(rows + 1, 0, " %s    HP: %d   Att: %d   Mana: %d \t\t Dlvl: %d",
//             race, hp, att, mana, dlvl);

    return 0;
}




int rip(int rows, int cols, int killer,const char *logged_user) {
    int c;
    int final_score = dlvl * 10 + m_defeated;
    update_scoreboard(logged_user, final_score, gold, turns);

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(RED));
        mvprintw(rows / 2 - 3, cols / 2 - 10, "You were captured by %c\n\n\n", killer);
        attroff(A_BOLD | COLOR_PAIR(RED));
        printw("\tLevel reached: %d\n"
               "\tMonsters defeated: %d\n"
               "\tTurns: %d\n\n"
               "\tAttack: %d\n"
               "\tMana: %d\n"
               "\n\n\tPress 'n' to start a new game or 'ESC' to exit.", dlvl, m_defeated, turns, att, mana);
        c = getch();
        if (c == 'n' || c == 27)
            return c;
    }
}


int check_trap(int rows, int cols, char (* obj)[cols]) {
    if (obj[py][px] == '^') {
        attron(A_BOLD | COLOR_PAIR(YELLOW));
        mvprintw(rows, cols - 10, "[Trap]");
        attroff(A_BOLD | COLOR_PAIR(YELLOW));

        if (!strcmp(race, "Halfling") && rand() % 2)
            ;
        else if (rand() % 2) {
            mvprintw(0, 0, " You've stepped into a trap... dart hits you!");
            hp -= dlvl / 2 + 1;
            if (hp < 1)
                return '^';
        } else {
            mvprintw(0, 0, " You've stepped into a trap... you are confused!");
            strncpy(state, "conf\0", 5);
        }
    }

    return 0;
}

//int monster_turn(int cols, char (*map)[cols]) {
//    int dist_y, dist_x;
//
//    for (int m = 0; m < 10 + dlvl / 2; m++) {
//        if (monster[m].lvl < 1) continue;
//
//        dist_y = abs(monster[m].y - py);
//        dist_x = abs(monster[m].x - px);
//
//        if (!(rand() % (dlvl + 1))) {
//            if (dist_y < dlvl + 3 - stealth && dist_x < dlvl + 3 - stealth) {
//                monster[m].awake = 1;
//            }
//        }
//
//        if (monster[m].awake == 0) continue;
//
//        int dir_y = monster[m].y;
//        int dir_x = monster[m].x;
//
//        if (monster[m].type == 'S') {
//            dir_y += (py > dir_y) ? 1 : -1;
//            dir_x += (px > dir_x) ? 1 : -1;
//        }
//        else if (monster[m].type == 'U' && dist_y <= 1 && dist_x <= 1) {
//            monster[m].chase_steps = 5;
//        }
//
//        if (monster[m].chase_steps > 0) {
//            dir_y += (py > dir_y) ? 1 : -1;
//            dir_x += (px > dir_x) ? 1 : -1;
//            monster[m].chase_steps--;
//        } else {
//            if (dist_y > dist_x) {
//                dir_y += (py > dir_y) ? 1 : -1;
//            } else {
//                dir_x += (px > dir_x) ? 1 : -1;
//            }
//        }
//
//        if (map[dir_y][dir_x] == '#' || map[dir_y][dir_x] == '%') {
//            if (dist_y > dist_x) {
//                dir_x += (px > dir_x) ? 1 : -1;
//            } else {
//                dir_y += (py > dir_y) ? 1 : -1;
//            }
//        }
//
//        if (map[dir_y][dir_x] == ' ' || map[dir_y][dir_x] == '.') {
//            map[monster[m].y][monster[m].x] = ' ';
//            monster[m].y = dir_y;
//            monster[m].x = dir_x;
//            map[dir_y][dir_x] = 't';
//        }
//
//        if (abs(py - monster[m].y) <= 1 && abs(px - monster[m].x) <= 1) {
//            int dmg = 0;
//            switch(monster[m].type) {
//                case 'D': dmg = 2; break;
//                case 'F': dmg = 4; break;
//                case 'G': dmg = 6; break;
//                case 'S': dmg = 8; break;
//                case 'U': dmg = 10; break;
//            }
//
//            if ((rand() % 20) > stealth) {
//                hp -= dmg;
//                mvprintw(0, 0, " %c hits you for %d damage!", monster[m].type, dmg);
//                if (hp < 1) return monster[m].type;
//            }
//        }
//    }
//    return 0;
//}

//int battle(int cols, char (* map)[cols], int dir_y, int dir_x) {
//    for (int m = 0; m < 10 + dlvl / 2; m++) {
//        if (dir_y == monster[m].y && dir_x == monster[m].x) {
//            if (monster[m].lvl < 1) {
//                m_defeated++;
//                switch(monster[m].type) {
//                    case 'D':
//                        att += 1;
//                        break;
//                    case 'F':
//                        mana += 2;
//                        break;
//                    case 'G':
//                        hp += 10;
//                        break;
//                    case 'S':
//                        stealth += 1;
//                        break;
//                    case 'U':
//                        att += 2;
//                        mana += 2;
//                        hp += 15;
//                        break;
//                }
//
//                mvprintw(0, cols/2, " You defeated %c!", monster[m].type);
//
//                map[dir_y][dir_x] = ' ';
//                monster[m].y = 0;
//                monster[m].x = 0;
//            } else {
//                mvprintw(0, cols/2, " You hit %c (%d HP remaining)",
//                         monster[m].type, monster[m].lvl);
//            }
//        }
//    }
//    return 0;
//}

int p_action(int c, int rows, int cols, char (*map)[cols], char (*obj)[cols]) {
    int dir_y = py, dir_x = px;

    if (c == 'w' || c == 'k') c = KEY_UP;
    else if (c == 's' || c == 'j') c = KEY_DOWN;
    else if (c == 'a' || c == 'h') c = KEY_LEFT;
    else if (c == 'd' || c == 'l') c = KEY_RIGHT;

    if (!strcmp(state, "conf") && (c == KEY_UP || c == KEY_DOWN || c == KEY_LEFT || c == KEY_RIGHT)) {
        c = rand() % 4 + KEY_UP;
    }

    switch (c) {
        case KEY_UP: dir_y--; break;
        case KEY_DOWN: dir_y++; break;
        case KEY_LEFT: dir_x--; break;
        case KEY_RIGHT: dir_x++; break;
    }

    if (map[dir_y][dir_x] == '.' || map[dir_y][dir_x] == '#' || map[dir_y][dir_x] == '+') {
        py = dir_y;
        px = dir_x;
    }
    else if (obj[py][px] == '>' && (c == '>' || c == '\n' || c == KEY_ENTER)) {
        t_placed = p_placed = r_placed = 0;
        return 1;
    }
//    else if (map[dir_y][dir_x] == 't') {
//        battle(cols, map, dir_y, dir_x);
//    }
    char food = obj[py][px];
    switch(food) {
        case 'N': inv.food[NORMAL_FOOD]++; break;
        case 'S': inv.food[SPECIAL_FOOD]++; break;
        case 'M': inv.food[MAGICAL_FOOD]++; break;
        case 'R': inv.food[ROTTEN_FOOD]++; break;
        default: return 0;
    }

//    if(food >= 'N' && food <= 'R') {
//        obj[py][px] = ' ';
//        mvprintw(0, 0, "Picked up %s food!",
//                 food == 'N' ? "Normal" :
//                 food == 'S' ? "Special" :
//                 food == 'M' ? "Magical" : "Rotten");
//    }
    return 0;
}
//void consume_food(enum FoodType type) {
//    if(inv.food[type] <= 0) return;
//
//    inv.food[type]--;
//
//    switch(type) {
//        case NORMAL_FOOD:
//            hp += 5;
//            break;
//        case SPECIAL_FOOD:
//            hp += 8;
//            att += 2;
//            break;
//        case MAGICAL_FOOD:
//            hp += 5;
//            stealth += 1;
//            break;
//        case ROTTEN_FOOD:
//            hp -= 10;
//            break;
//    }
//
//    mvprintw(0, 0, "Consumed %s food!",
//             type == NORMAL_FOOD ? "Normal" :
//             type == SPECIAL_FOOD ? "Special" :
//             type == MAGICAL_FOOD ? "Magical" : "Rotten");
//}
//int spawn_t(int rows, int cols, char (* map)[cols]) {
//    if (!t_placed) {
//        int my, mx;
//        int boss = 0;
//
//        for (int m = 0; m < 10 + dlvl / 2; m++) {
//            do {
//                my = rand() % rows;
//                mx = rand() % cols;
//            } while (map[my][mx] != ' ' || (my == py && mx == px));
//
//            monster[m].y = my;
//            monster[m].x = mx;
//
//            if (dlvl <= 3) {
//                monster[m].type = 'D';
//                monster[m].lvl = 5;
//            } else if (dlvl <= 6) {
//                monster[m].type = 'F';
//                monster[m].lvl = 10;
//            } else if (dlvl <= 9) {
//                monster[m].type = 'G';
//                monster[m].lvl = 15;
//            } else if (dlvl <= 12) {
//                monster[m].type = 'S';
//                monster[m].lvl = 20;
//            } else {
//                monster[m].type = 'U';
//                monster[m].lvl = 30;
//            }
//
//            if ((dlvl == 13 || dlvl == 14) && boss != 9) {
//                monster[m].type = 'Z';
//                monster[m].lvl = 666;
//                monster[m].awake = 1;
//                boss++;
//            } else {
//                monster[m].awake = 0;
//                monster[m].chase_steps = 0;
//            }
//
//            map[monster[m].y][monster[m].x] = 't';
//        }
//        t_placed = 1;
//    }
//    return 0;
//}

int spawn_p(int rows, int cols, char (*map)[cols], char (*obj)[cols]) {
    if (!p_placed) {
        int attempts = 0;
        do {
            py = (rand() % (rows/2)) + rows/4;
            px = (rand() % (cols/2)) + cols/4;
            attempts++;

            if (map[py][px] == '.' && py > 1 && py < rows-2 && px > 1 && px < cols-2) break;

            if (attempts > 50) {
                for (int y = 1; y < rows-1; y++) {
                    for (int x = 1; x < cols-1; x++) {
                        if (map[y][x] == '.') {
                            py = y;
                            px = x;
                            goto found;
                        }
                    }
                }
            }
        } while (1);
        found:
        p_placed = 1;
    }
    return 0;
}

void spawn_objects(int rows, int cols, char (*map)[cols], char (*obj)[cols]) {
    if (lvl_turns == 0 || turns == 0) {
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                obj[y][x] = ' ';
            }
        }

        for (int i = 0; i < 5; i++) {
            int y, x;
            do {
                y = rand() % rows;
                x = rand() % cols;
            } while (map[y][x] != '.' || (y == py && x == px));
            map[y][x] = 'O';
        }

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if ((y == 0 || y == rows - 1 || x == 0 || x == cols - 1) &&
                    (map[y][x] == '_' || map[y][x] == '|')) {
                    if (rand() % 10 == 0) {
                        map[y][x] = '=';
                    }
                }
            }
        }

        do {
            sy = rand() % rows;
            sx = rand() % cols;
        } while (map[sy][sx] != '.');
        obj[sy][sx] = '>';

        int final_lvl = 13 + rand() % 2;
        if (dlvl == final_lvl) {
            int ty, tx;
            do {
                ty = rand() % rows;
                tx = rand() % cols;
            } while (map[ty][tx] != '.');
            map[ty][tx] = 'T';

            for (int i = 0; i < 10; i++) {
                int trap_y = ty + rand() % 3 - 1;
                int trap_x = tx + rand() % 3 - 1;
                if (map[trap_y][trap_x] == '.') {
                    obj[trap_y][trap_x] = '^';
                }
            }
        }
    }

    if (rand() % ((18 - dlvl) / 2)) {
        int y, x;
        do {
            y = rand() % rows;
            x = rand() % cols;
        } while (map[y][x] != ' ' && obj[y][x] != ' ');
        obj[y][x] = '^';
    }
    for(int y=0; y<rows; y++) {
        for(int x=0; x<cols; x++) {
            if(map[y][x] == '.' && rand() % 25 == 0) {
                char food_char = 'N';
                switch(rand() % 10) {
                    case 0: case 1: food_char = 'S'; break;
                    case 2: case 3: food_char = 'M'; break;
                    case 4: food_char = 'R'; break;
                }
                obj[y][x] = food_char;
            }
        }
    }
}

#define MIN_ROOM_WIDTH 6
#define MAX_ROOM_WIDTH 11
#define MIN_ROOM_HEIGHT 5
#define MAX_ROOM_HEIGHT 8

#define MIN_ROOM_WIDTH 6
#define MAX_ROOM_WIDTH 11
#define MIN_ROOM_HEIGHT 5
#define MAX_ROOM_HEIGHT 8

int dungeon_gen(int rows, int cols, char (*map)[cols]) {
    if (!r_placed) {
        srand(time(NULL));
        int room_attempts = 0;


        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (y == 0 || y == rows - 1) {
                    map[y][x] = '_';
                } else if (x == 0 || x == cols - 1) {
                    map[y][x] = '|';
                } else {
                    map[y][x] = ' ';
                }
            }
        }

        int max_rooms = rand() % (MAX_ROOMS - MIN_ROOMS + 1) + MIN_ROOMS;
        int room_centers[max_rooms][2];

        for (int r = 0; r < max_rooms; r++) {
            if (room_attempts > MAX_ROOM_ATTEMPTS) {
                printw("Warning: Room limit exceeded.\n");
                break;
            }

            int room_w = rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1) + MIN_ROOM_WIDTH;
            int room_h = rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1) + MIN_ROOM_HEIGHT;
            int room_x = rand() % (cols - room_w - 2) + 1;
            int room_y = rand() % (rows - room_h - 2) + 1;

            bool collision = false;
            for (int y = room_y - 1; y < room_y + room_h + 1; y++) {
                for (int x = room_x - 1; x < room_x + room_w + 1; x++) {
                    if (map[y][x] == '.' || map[y][x] == '_' || map[y][x] == '|') {
                        collision = true;
                        break;
                    }
                }
                if (collision) break;
            }

            if (collision) {
                r--;
                room_attempts++;
                continue;
            }

            for (int y = room_y; y < room_y + room_h; y++) {
                for (int x = room_x; x < room_x + room_w; x++) {
                    if (y == room_y || y == room_y + room_h - 1) {
                        map[y][x] = '_';
                    } else if (x == room_x || x == room_x + room_w - 1) {
                        map[y][x] = '|';
                    } else {
                        map[y][x] = '.';
                    }
                }
            }

            for (int d = 0; d < 2; d++) {
                int door_side = rand() % 4;
                int door_y, door_x;
                switch (door_side) {
                    case 0:
                        door_y = room_y;
                        door_x = room_x + rand() % (room_w - 2) + 1;
                        break;
                    case 1:
                        door_y = room_y + room_h - 1;
                        door_x = room_x + rand() % (room_w - 2) + 1;
                        break;
                    case 2:
                        door_y = room_y + rand() % (room_h - 2) + 1;
                        door_x = room_x;
                        break;
                    case 3:
                        door_y = room_y + rand() % (room_h - 2) + 1;
                        door_x = room_x + room_w - 1;
                        break;
                }
                if (map[door_y][door_x] == '|' || map[door_y][door_x] == '_') {
                    map[door_y][door_x] = '+';
                }
            }

            room_centers[r][0] = room_y + room_h / 2;
            room_centers[r][1] = room_x + room_w / 2;
            r_placed++;
        }

        for (int i = 1; i < r_placed; i++) {
            int y1 = room_centers[i - 1][0], x1 = room_centers[i - 1][1];
            int y2 = room_centers[i][0], x2 = room_centers[i][1];

            int step_x = (x1 < x2) ? 1 : -1;
            for (int x = x1; x != x2; x += step_x) {
                if (map[y1][x] == '|' || map[y1][x] == '_') {
                    map[y1][x] = '+';
                } else if (map[y1][x] != '.') {
                    map[y1][x] = '#';
                }
            }

            int step_y = (y1 < y2) ? 1 : -1;
            for (int y = y1; y != y2; y += step_y) {
                if (map[y][x2] == '|' || map[y][x2] == '_') {
                    map[y][x2] = '+';
                } else if (map[y][x2] != '.') {
                    map[y][x2] = '#';
                }
            }
        }
    }
    return 0;
}



int create_char(int c)
{
    att = 1;
    mana = 1;
    stealth = 0;
    t_placed = 0;
    p_placed = 0;
    r_placed = 0;
    dlvl = 1;
    lvl_turns = 0;
    m_defeated = 0;
    strncpy(state, "\0\0\0\0\0", 5);

    if (c == 'n')
    {
        if (!strcmp(race, "Human"))
            c = '1';
        else if (!strcmp(race, "Dwarf"))
            c = '2';
        else if (!strcmp(race, "Elf"))
            c = '3';
        else if (!strcmp(race, "Halfling"))
            c = '4';
        else if (!strcmp(race, "Orc"))
            c = '5';
    }

    switch (c)
    {
        case '2':
        {
            hp = 10 + rand() % 3 + 2;
            att += 2;
            stealth = -2;
            strncpy(race, "Dwarf\0", 6);
            break;
        }
        case '3':
        {
            hp = 10 + rand() % 3;
            att += 2;
            stealth = 1;
            strncpy(race, "Elf\0", 4);
            break;
        }
        case '4':
        {
            hp = 10 - rand() % 2;
            stealth = 2;
            strncpy(race, "Halfling\0", 9);
            break;
        }
        case '5':
        {
            hp = 10 - rand() % 2 + 1;
            att += 1;
            stealth = -1;
            strncpy(race, "Orc\0", 4);
            break;
        }
        default:
        {
            hp = 10 + rand() % 2;
            stealth = 0;
            strncpy(race, "Human\0", 6);
            break;
        }
    }

    return 0;
}

int game_loop(int c, int rows, int cols, char (*map)[cols], char (*obj)[cols], const char *logged_user) {
    static char username[MAX_USERNAME_LEN] = "Guest";
    int action_result = 0;
    int killer = 0;
    srand(time(NULL));

    move(0, 0);
    clrtoeol();

    if (turns == 0 || c == 'n') {
        create_char(c);
        strncpy(username, logged_user, MAX_USERNAME_LEN);
        memset(&inv, 0, sizeof(inv));
        in_treasure_room = false;
    }

    dungeon_gen(rows, cols, map);
    spawn_objects(rows, cols, map, obj);
    spawn_p(rows, cols, map, obj);
//    spawn_t(rows, cols, map);
//
//    display_action_message("Weapon equipped: %s (Damage: %d)",
//                           (currentWeapon.type == MACE) ? "Mace" :
//                           (currentWeapon.type == DAGGER) ? "Dagger" :
//                           (currentWeapon.type == MAGIC_WAND) ? "Magic Wand" :
//                           (currentWeapon.type == NORMAL_ARROW) ? "Arrow" : "Sword",
//                           currentWeapon.damage);

    // Check treasure room entrance
    if(!in_treasure_room && dlvl == 10 && obj[py][px] == '$') {
        generate_treasure_room(rows, cols, map);
        in_treasure_room = true;
        mvprintw(0, 0, "Entered the Treasure Room!");
        refresh();
        napms(2000);
    }

    // Main game logic
    if (turns > 0) {
        if (c != 0) {
            action_result = p_action(c, rows, cols, map, obj);
//            killer = monster_turn(cols, map);
        }

        for(int m = 0; m < 20; m++) {
            if(monster[m].lvl < 1 && monster[m].type != 0) {
                if(in_treasure_room) treasure_room_enemies--;
                monster[m].type = 0;
            }
        }

        if (hp > 0) {
            killer = check_trap(rows, cols, obj);
        } else {
            c = rip(rows, cols, killer, logged_user);
            turns = 0;
            return c;
        }
    }

    if (action_result == 1) {
        dlvl++;
        hp += dlvl;
        lvl_turns = 0;
        dungeon_gen(rows, cols, map);
        spawn_objects(rows, cols, map, obj);
        spawn_p(rows, cols, map, obj);
//        spawn_t(rows, cols, map);
        dungeon_draw(rows, cols, map, obj);
        save_game(rows - 1, cols - 1, obj);
        mvprintw(0, 0, "Checkpoint saved!");
    }

    dungeon_draw(rows, cols, map, obj);

    if(++inv.food_degrade_timer >= 50) {
        inv.food_degrade_timer = 0;
        inv.food[NORMAL_FOOD] += inv.food[SPECIAL_FOOD];
        inv.food[SPECIAL_FOOD] = 0;
        inv.food[NORMAL_FOOD] += inv.food[MAGICAL_FOOD];
        inv.food[MAGICAL_FOOD] = 0;
        inv.food[ROTTEN_FOOD] += inv.food[NORMAL_FOOD]/2;
        inv.food[NORMAL_FOOD] -= inv.food[NORMAL_FOOD]/2;
    }

    hp += (inv.potion_timer[HEALTH_POT] > 0) ? 2 : 1;
    for(int i = 0; i < 3; i++) {
        if(inv.potion_timer[i] > 0) inv.potion_timer[i]--;
    }

    if(c >= '1' && c <= '7') {
//        consume_item(c);
    }

    mvprintw(rows + 1, 0, " %s    HP: %d   Att: %d   Mana: %d   Dlvl: %d",
             race, hp, att, mana, dlvl);
    mvprintw(rows + 2, 0, "Potions: H[%d]1 S[%d]2 D[%d]3   Food: N[%d]4 Sp[%d]5 M[%d]6 R[%d]7",
             inv.potions[0], inv.potions[1], inv.potions[2],
             inv.food[0], inv.food[1], inv.food[2], inv.food[3]);

    if(in_treasure_room && treasure_room_enemies <= 0) {
        clear();
        attron(A_BOLD | COLOR_PAIR(GREEN));
        mvprintw(rows/2-2, cols/2-15, "CONGRATULATIONS, %s!", logged_user);
        mvprintw(rows/2, cols/2-20, "You've conquered the Treasure Room!");
        mvprintw(rows/2+2, cols/2-15, "Final Score: %d",
                 dlvl*1000 + m_defeated*100 + gold);
        attroff(A_BOLD | COLOR_PAIR(GREEN));
        refresh();
        getch();
        update_scoreboard(logged_user, dlvl*1000 + m_defeated*100 + gold, gold, turns);
        return 'W';
    }

    c = getch();
    if (c == 27) {
        printw("\nSave before exit? (y/n): ");
        c = getch();
        if (c == 'y' || c == 'Y') save_game(rows - 1, cols - 1, obj);
        return c;
    }

    turns++;
    lvl_turns++;
    if (!(turns % 50 - (dlvl * 2)) && hp > 1) hp--;

    return c;
}

void load_game() {
    FILE *file = fopen("savegame.txt", "r");
    if (!file) {
        printw("Error: No saved game found.\n");
        return;
    }

    fscanf(file, "%d %d %d %d\n",
           &inv.food[0], &inv.food[1],
           &inv.food[2], &inv.food[3]);
    fscanf(file, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
           &py, &px, &att, &hp, &mana, &stealth, &dlvl, &turns,
           &lvl_turns, &m_defeated, &gold, &experience);

    for (int i = 0; i < 20; i++) {
        fscanf(file, "%d %d %d %d %d\n",
               &monster[i].y, &monster[i].x, &monster[i].lvl, &monster[i].type, &monster[i].awake);
    }

    int y, x;
    while (fscanf(file, "%d %d\n", &y, &x) == 2) {
        obj[y][x] = '^';
    }

    fclose(file);
    printw("Game loaded successfully!\n");
}

void display_scoreboard(const char *logged_user) {
    FILE *file = fopen("scoreboard.txt", "r");
    if (!file) {
        printw("Error: Scoreboard is empty.\n");
        return;
    }

    clear();
    printw("\n\t\t\t\tScoreboard\n\n");

    struct ScoreboardEntry entries[100];
    int entry_count = 0;

    while (fscanf(file, "%s %d %d %d %d\n",
                  entries[entry_count].username,
                  &entries[entry_count].scores[0],
                  &entries[entry_count].gold,
                  &entries[entry_count].finished_games,
                  &entries[entry_count].experience) == 5) {
        entry_count++;
    }
    fclose(file);


    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = i + 1; j < entry_count; j++) {
            if (entries[i].scores[0] < entries[j].scores[0]) {
                struct ScoreboardEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    for (int i = 0; i < entry_count; i++) {
        if (i < 3) {
            attron(A_BOLD | COLOR_PAIR(YELLOW));
            printw("\t%d. %s\tScore: %d\tGold: %d\tGames: %d\tExp: %d sec\n",
                   i + 1, entries[i].username, entries[i].scores[0], entries[i].gold, entries[i].finished_games, entries[i].experience);
            attroff(A_BOLD | COLOR_PAIR(YELLOW));
        } else if (strcmp(entries[i].username, logged_user) == 0) {
            attron(A_BOLD);
            printw("\t%d. %s\tScore: %d\tGold: %d\tGames: %d\tExp: %d sec\n",
                   i + 1, entries[i].username, entries[i].scores[0], entries[i].gold, entries[i].finished_games, entries[i].experience);
            attroff(A_BOLD);
        } else {
            printw("\t%d. %s\tScore: %d\tGold: %d\tGames: %d\tExp: %d sec\n",
                   i + 1, entries[i].username, entries[i].scores[0], entries[i].gold, entries[i].finished_games, entries[i].experience);
        }
    }

    printw("\n\tPress any key to return to the main menu.\n");
    getch();
}
void profile_settings() {
    int choice;
    while (1) {
        clear();
        printw("\n\t\t\t\tProfile Settings\n\n");
        printw("\t1. Set Difficulty\n");
        printw("\t2. Select Background Music\n");
        printw("\t3. Change Rogue Color\n");
        printw("\t4. Return to Main Menu\n");
        printw("\n\t Enter your choice: ");
        refresh();
        choice = getch() - '0';

        switch (choice) {
            case 1:
                clear();
                printw("\n\tSelect Difficulty:\n");
                printw("\t1. Easy\n");
                printw("\t2. Medium\n");
                printw("\t3. Hard\n");
                printw("\n\tEnter your choice: ");
                refresh();
                difficulty = getch() - '1';
                break;

            case 2:
                clear();
                printw("\n\tSelect Background Music:\n");
                for (int i = 0; i < 3; i++) {
                    printw("\t%d. %s\n", i + 1, songs[i]);
                }
                printw("\n\tEnter your choice: ");
                refresh();
                selected_song = getch() - '1';
                printw("\n\tSelected song: %s\n", songs[selected_song]);
                getch();
                break;
            case 3:
                clear();
                printw("\n\tSelect Color:\n");
                attroff(A_BOLD | COLOR_PAIR(COLOR_RED));
                printw("\t1. Red\n");
                attroff(A_BOLD | COLOR_PAIR(COLOR_GREEN));
                printw("\t2. Green\n");
                attroff(A_BOLD | COLOR_PAIR(COLOR_BLUE));
                printw("\t3. Blue\n");
                attroff(A_BOLD | COLOR_PAIR(WHITE));
                printw("\t4. White\n");
                attroff(A_BOLD | COLOR_PAIR(COLOR_CYAN));
                printw("\t5. Cyan\n");
                int colorChoice = 0;
                colorChoice = getch();
                switch(colorChoice){
                    case 49:{
                        printw("Color Selected: RED");
                        playerColor = 1;
                        getch();
                        break;
                    }
                    case 50:{
                        printw("Color Selected:Green");
                        playerColor = 2;
                        getch();
                        break;
                    }
                    case 51:{
                        printw("Color Selected: Blue");
                        playerColor = 3;
                        getch();
                        break;
                    }
                    case 52:{
                        printw("Color Selected: White");
                        playerColor = 4;
                        getch();
                        break;
                    }
                    case 53:{
                        printw("Color Selected: Cyan");
                        playerColor = 5;
                        getch();
                        break;
                    }
                    default: break;
                }

                getch();
                break;

            case 4:
                return;

            default:
                printw("\n\tInvalid choice. Please try again.\n");
                getch();
                break;
        }
    }
}

void spawn_password_doors(int rows, int cols, char (* map)[cols], char (* obj)[cols]) {
    if (rand() % 5 == 0) {
        int y, x;
        do {
            y = rand() % rows;
            x = rand() % cols;
        } while (map[y][x] != ' ');
        map[y][x] = 'P';
    }
}

int check_password_door(int rows, int cols, char (* map)[cols], char (* obj)[cols]) {
    if (map[py][px] == 'P') {

        int code = rand() % 10000;
        mvprintw(0, 0, " Password Door: Enter code (4 digits): ");
        refresh();


        for (int i = 0; i < 3; i++) {
            int input;
            scanw("%d", &input);
            if (input == code) {
                mvprintw(0, 0, " Door unlocked!");
                map[py][px] = ' ';
                return 1;
            } else {
                mvprintw(0, 0, " Incorrect code. Attempts left: %d", 2 - i);
            }
        }

        mvprintw(0, 0, " Door locked!");
        return 0;
    }

    return 1;
}
int main_menu(const char *username) {
    int choice;
    while (1) {
        clear();
        printw("\n\t\t\t\tMain Menu\n\n");
        printw("\t1. Start New Game\n");
        printw("\t2. Load Last Checkpoint\n");
        printw("\t3. Save Game\n");
        printw("\t4. Scoreboard\n");
        printw("\t5. Profile Settings\n");
        printw("\t6. Exit\n");
        printw("\n\tEnter your choice: ");
        refresh();
        choice = getch() - '0';

        switch (choice) {
            case 1:
                return 1;

            case 2:
                load_game();
                return 1;

            case 3:
                save_game(rows - 1, cols - 1, obj);
                printw("\n\tGame saved successfully! Press any key to continue.\n");
                getch();
                break;

            case 4:
                display_scoreboard(username);
                break;

            case 5:
                profile_settings();
                break;

            case 6:
                return 0;

            default:
                printw("\n\tInvalid choice. Please try again.\n");
                getch();
                break;
        }
    }
}

int main(void) {
    int c;

    initscr();
    start_color();
    use_default_colors();

    init_pair(BLACK, COLOR_BLACK, COLOR_WHITE);
    init_pair(RED, COLOR_RED, COLOR_BLACK);
    init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(WHITE, COLOR_WHITE, COLOR_WHITE);
    init_pair(8, COLOR_WHITE, COLOR_BLACK);
    init_pair(9, COLOR_CYAN, COLOR_BLACK);

    keypad(stdscr, 1);
    noecho();
    curs_set(0);


    char map[rows][cols];
    char obj[rows][cols];


    if(!auth_menu(playerUsername)){
        endwin();
        return 0;
    }

    if (!main_menu(playerUsername)) {
        endwin();
        return 0;
    }

    while (1) {
        c = game_loop(c, rows - 1, cols - 1, map, obj,playerUsername);
        if (c == 27) {
            endwin();
            return 0;
        }
    }
}