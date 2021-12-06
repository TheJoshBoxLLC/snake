/*
* Snake Game
* CS 355 Final Project
* Benjamin Carlson, Josh Magalhaes, Erik Marrro, Nick Sbado
*
* To compile: gcc -o snake snake.c -lcurses
* To run: ./snake
* To terminate: press command + c
*
* The snake pit:
* [X] The snake pit must utilize all available space of the current terminal window.
* [X] There must be a visible border delineating the snake pit.
* 
* The snake:
* [X] The initial length of the snake is three characters.
* [X] Initial direction of the snake's movement is chosen randomly.
* [X] The user can press one of the four arrow keys to change the direction of the snake's movement.
* [X] The snake's speed is proportional to its length.
* 
* The trophies:
* [X] Trophies are represented by a digit randomly chosen from 1 to 9.
* [X] There's always exactly one trophy in the snake pit at any given moment.
* [X] When the snake eats the trophy, its length is increased by the corresponding number of characters.
* [X] A trophy expires after a random interval from 1 to 9 seconds.
* [X] A new trophy is shown at a random location on the screen after the previous one has either expired or is eaten by the snake.
* 
* The gameplay:
* The snake dies and the game ends if:
* [X] It runs into the border; or
* [X] It runs into itself; or
* [X] The user attempts to reverse the snake's direction.
* 
* [X] The user wins the game if the snake's length grows to the length equal to half the perimeter of the border.
*/

#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <termios.h>

// ************* constants ************* //
#define STDIN_FD 0 // standard input file descriptor
// end constants *************

// ************* function prototypes ************* //
void init_pit_border();
void init_snake(int, int);
void move_snake();
void grow_snake(int);
char choose_random_direction();
void run_when_time_runs_out();
void set_nodelay();
void end_snake_game(int signum);
void detect_collisions();
void check_win();
int calculate_snake_length();
void init_trophy();
void run_every_second_check_trophy();
// end function prototypes *************

// ************* variables ************* //
int win_length = 0;         // length of snake required to win
int row = 0;                // current row
int col = 0;                // current column
char key = 'd';             // key input
short ticks = 0;            // ticks make up the game clock. They count to the timeUnit at a constant rate. when timeUnit gets smaller, ticks reach it faster meaning snake speeds up.
short timeUnitConstant = 0; // equal to the initial timeUnit. Unlike timeUnit, this variable will not change.
short timeUnit = 0;         // timeUnit is made up of ticks. The lower the time unit, the faster the snake will move.
short directionY = 0;       // y direction
short directionX = 0;       // x direction
struct timespec speed, rem; // refresh rate
int snake_head_y = 0;       // y coordinate of the snake's head
int snake_head_x = 0;       // x coordinate of the snake's head
struct node                 // LL representation of snake. *next points towards head, *prev points towards tail. row/colum are the x/y coordinates of the node.
{
    int row;
    int column;
    struct node *prev;
    struct node *next;
};
struct node *head;         // snake head
struct node *tail;         // snake tail
int trophy_value = 0;      // value of the trophy
int trophy_row = 0;        // row of the trophy
int trophy_col = 0;        // column of the trophy
int trophy_expiration = 0; // expiration time of the trophy
int trophy_ticks = 0;      // ticks to expiration
// end variables *************

int main()
{
    // housekeeping
    srand((unsigned)time(NULL));    // seed random number generator
    signal(SIGINT, end_snake_game); // when user terminates program, end game!
    initscr();                      // start curses
    clear();                        // clear the screen
    curs_set(0);                    // turn off cursor
    speed.tv_sec = 0;               // sleep timer
    speed.tv_nsec = 781250;         // 1/128th of a second in nanoseconds
    keypad(stdscr, TRUE);           // enable keypad mode
    start_color();                  // enable color
    // init_pair(1, COLOR_RED, COLOR_BLACK);   // set color pair 1
    // init_pair(2, COLOR_GREEN, COLOR_BLACK); // set color pair 2

    // set up the game
    init_pit_border();               // initialize pit border
    key = choose_random_direction(); // set a random initial direction for the snake. For first deliverable this is to the right
    char input;                      // the key input
    row = LINES;                     // num rows
    col = COLS;                      // num columns

    // center the snake
    snake_head_y = row / 2;
    snake_head_x = col / 2;
    init_snake(snake_head_y, snake_head_x); // initialize snake of size 5 in the center of the screen

    win_length = LINES + COLS;   // calculate length of snake required to win. Half the perimeter is required to win. Perimeter is 2*(rows + columns). Half of that is 2*(rows + columns)/2. We can simplify this to rows+columns.
    timeUnit = 50 + win_length;  // set time unit to 50 plus the length to win. This is because we want the use to end on a timeUnit of 50.
    timeUnitConstant = timeUnit; // set time unit constant to time unit. This is used to increase snake speed.

    // Print welcome message to center of screen
    move(row / 2, col / 2 - 50 / 2);
    addstr("Welcome to our snake game! Press any key to play!");

    refresh(); // buffer -> screen

    getchar();     // wait for user input to continue game
    noecho();      // don't echo user input
    set_nodelay(); // set no delay mode

    // clear "Welcome to our snake game! Press any key to start."
    move(row / 2, col / 2 - 50 / 2);
    addstr("                                                 ");
    move(row - 1, col - 1);

    init_trophy(); // initialize first trophy

    // start game and loop until user terminates, wins, or loses
    // By: Benjamin Carlson
    while (1)
    {
        while ((input = getch()) == ERR) // if no input, continue
        {
            nanosleep(&speed, &rem); // sleep for 1/128th of a second
            ticks++;                 // increment ticks
            trophy_ticks++;          // increment trophy ticks

            // run run_every_second() every second
            if (ticks % timeUnit == 0) // if ticks is a multiple of time unit == 0, increment by 1 gameunit
            {
                run_when_time_runs_out();
            }

            if (trophy_ticks == 1088) // 128 * 8.5 -> 1 second
            {
                trophy_ticks = 0;
                run_every_second_check_trophy();
            }
        }

        // if user presses a key, change direction
        // By: Benjamin Carlson
        switch (input) // if input is a valid key i.e - arrow key or left arrow keys, set direction
        {
        case (char)KEY_LEFT: // left
        case 'a':
            key = 'a';
            directionY = 0;
            directionX = -1;
            run_when_time_runs_out(); // if user presses a key, reset ticks
            break;
        case (char)KEY_DOWN: // down
        case 's':
            key = 's';
            directionY = 1;
            directionX = 0;
            run_when_time_runs_out();
            break;
        case (char)KEY_UP: // up
        case 'w':
            key = 'w';
            directionY = -1;
            directionX = 0;
            run_when_time_runs_out();
            break;
        case (char)KEY_RIGHT: // right
        case 'd':
            key = 'd';
            directionY = 0;
            directionX = 1;
            run_when_time_runs_out();
            break;
        default:
            break;
        }

        refresh(); // buffer -> screen
    }
}

/*
* Snake Pit
* By: Benjamin Carlson
* What it does: Initializes the snake pit. The pit takes up the entire terminal.
* We can achieve this by using LINES and COLS
*/
void init_pit_border()
{
    // attron(COLOR_PAIR(1)); // set color pair 1
    // attron(A_BOLD);
    for (int i = 1; i < LINES; i++)
    { // outer loop
        for (int j = 0; j < COLS; j++)
        {               // inner loop
            move(i, j); // move cursor to new position
            // only add "-" or "|" to the outer perimeter
            if (i == 1 || i == LINES - 1)
                addstr("X");
            else if (j == 0 || j == COLS - 1)
                addstr("X");
        }
    }
    // attroff(COLOR_PAIR(1)); // turn off color pair 1
    // attroff(A_BOLD);
}

/*
* Snake - Initializes the snake.
* By: Benjamin Carlson
* What it does: Initializes the snake. The snake is a linked list of nodes.
* The head and tail of the snake are the first and last nodes in the linked list.
* The snake is initialized in the center of the screen.
*/
void init_snake(int start_y, int start_x)
{
    head = (struct node *)malloc(sizeof(struct node)); // allocate memory for head
    tail = (struct node *)malloc(sizeof(struct node)); // allocate memory for tail
    head->prev = tail;                                 // head points to tail
    tail->next = head;                                 // tail points to head
    head->row = start_y;                               // set head row
    head->column = start_x;                            // set head column
    grow_snake(3);                                     // grow snake by 3 (to reach 5 total)
}

/*
* Snake - Grow Snake
* By: Benjamin Carlson
* What it does: Grow the snake by a specified amount.
*/
void grow_snake(int num_to_grow_by)
{
    // attron(COLOR_PAIR(2)); // set color pair 2
    // grow snake by num_to_grow_by
    for (int i = 0; i < num_to_grow_by; i++)
    {
        struct node *body = tail->next;                                            // body is the node that will be added to the snake
        struct node *new_snake_piece = (struct node *)malloc(sizeof(struct node)); // allocate memory for new piece
        new_snake_piece->row = tail->row;                                          // set new piece row
        new_snake_piece->column = tail->column;                                    // set new piece column
        new_snake_piece->next = tail->next;                                        // set new piece next
        new_snake_piece->prev = tail;                                              // set new piece prev
        tail->next = new_snake_piece;                                              // set tail next to new piece
        body->prev = new_snake_piece;                                              // set body prev to new piece
    }
    // attroff(COLOR_PAIR(2)); // turn off color pair 2
}

/*
* Snake - Move Snake
* By: Benjamin Carlson
* What it does: Move the snake.
*/
void move_snake()
{
    // attron(COLOR_PAIR(2));         // set color pair 2
    int y = head->row;             // y coordinate of the snake's head
    int x = head->column;          // x coordinate of the snake's head
    head->row += directionY;       // move snake head in direction of directionY
    head->column += directionX;    // move snake head in direction of directionX
    move(head->row, head->column); // move cursor to new position
    attron(A_STANDOUT);
    addstr("O"); // add "O" to the screen. This is the snake head
    attroff(A_STANDOUT);
    struct node *scanner = head->prev; // scanner is the node that will be removed from the snake
    while (scanner != NULL)            // move all nodes in the snake
    {
        int temp_y = y;                      // temp_y is the y coordinate of the node that will be removed
        int temp_x = x;                      // temp_x is the x coordinate of the node that will be removed
        y = scanner->row;                    // y is the y coordinate of the node that will be removed
        x = scanner->column;                 // x is the x coordinate of the node that will be removed
        scanner->row = temp_y;               // set scanner row to temp_y
        scanner->column = temp_x;            // set scanner column to temp_x
        move(scanner->row, scanner->column); // move cursor to new position
        addstr("o");                         // add "o" to the screen. This is the snake body
        scanner = scanner->prev;             // move scanner to the previous node
    }
    move(y, x);  // move cursor to old head position
    addstr(" "); // erase old head
    // attroff(COLOR_PAIR(2)); // turn off color pair 2
}

/*
* Snake - Length
* By: Benjamin Carlson
* What it does: A helper function to calculate the snake length.
*/
int calculate_snake_length()
{
    int length = 0;
    struct node *scanner = head;
    while (scanner != NULL)
    {
        length++;
        scanner = scanner->prev;
    }
    return length;
}

/*
* Snake Direction
* By: Benjamin Carlson
* What it does: Set the initial direction of the snake.
*/
char choose_random_direction()
{
    // choose random direction and set the direction variables. Doesn't matter which number corresponds to which direction.
    int random_direction = rand() % 4;
    switch (random_direction)
    {
    case 0:
        directionY = 0;
        directionX = -1;
        return 'a';
        break;
    case 1:
        directionY = 0;
        directionX = 1;
        return 'd';
        break;
    case 2:
        directionY = -1;
        directionX = 0;
        return 'w';
        break;
    case 3:
        directionY = 1;
        directionX = 0;
        return 's';
        break;
    default:
        return ' '; // should never happen
        break;
    }
}

/*
* Run When Time Runs Out
* By: Benjamin Carlson
* What it does: refreshes the screen to move the snake after a certian amount of time. This is what we will call "time running out". In this first deliverable,
* it will be at a standard rate, but in the final, it will be determined by the length of the snake.
*/
void run_when_time_runs_out()
{
    // if snake head is on the trophy, grow snake by trophy amount and call init_trophy again
    if (head->row == trophy_row && head->column == trophy_col)
    {
        grow_snake(trophy_value);
        init_trophy();
    }
    check_win(); // check if the snake has won
    // // get random number from 1 to 5. If number is 1, then the snake will grow.
    // int random_number = rand() % 5;
    // if (random_number == 1)
    //     grow_snake(1);                                      // grow snake by 1, testing purposes
    timeUnit = timeUnitConstant - calculate_snake_length(); // time unit is current time unit - snake length. This speeds up the snake as it gets longer.
    ticks = 0;                                              // reset ticks
    move_snake();                                           // move snake
    detect_collisions();                                    // detect collisions
    move(0, 0);
    addstr("Win Length: ");
    printw("%d", win_length);
    addstr(" | Snake Length: ");
    printw("%d", calculate_snake_length());
    addstr(" | Trophy Timer: ");
    printw("%d", trophy_expiration);
    refresh(); // buffer -> screen
}

/*
* Mode
* By: Benjamin Carlson
* What it does: Initializes the mode.
*/
void set_nodelay()
{
    int termflags;                        // terminal flags
    termflags = fcntl(STDIN_FD, F_GETFL); // get terminal flags
    termflags |= O_NDELAY;                // set no delay mode
    fcntl(STDIN_FD, F_SETFL, termflags);  // set terminal flags
}

/*
* End Game
* By: Benjamin Carlson
* What it does: Ends the game on user interrupt.
*/
void end_snake_game(int signum)
{
    endwin(); // end curses mode
    exit(1);  // exit program
}

/*
* Detect Collisions
* By: Benjamin Carlson
* What it does: Detects collisions.
* The snake dies and the game ends if:
*  - It runs into the border; or
*  - It runs into itself; or
*  - The user attempts to reverse the snake's direction.
*/
void detect_collisions()
{
    // detect if snake runs into border
    if (head->row == 1 || head->row == LINES - 1 || head->column == 0 || head->column == COLS - 1)
    {
        // if user runs into a border, write message to screen and end game
        move(row / 2, col / 2);
        attron(A_STANDOUT);
        addstr("You ran into a border!");
        attroff(A_STANDOUT);
        refresh();
        sleep(2);
        raise(SIGINT); // raise interrupt signal to end game
    }
    struct node *scanner = head->prev; // scanner is the node that will be removed from the snake
    while (scanner != NULL)            // move all nodes in the snake
    {
        // detect if snake runs into itself (also handles if snake reverses direction)
        if (head->row == scanner->row && head->column == scanner->column)
        {
            // if user runs into itself, write message to screen and end game
            move(row / 2, col / 2);
            attron(A_STANDOUT);
            addstr("You ran into yourself!");
            attroff(A_STANDOUT);
            refresh();
            sleep(2);
            raise(SIGINT); // raise interrupt signal to end game
        }
        scanner = scanner->prev; // move scanner to the previous node
    }
}

/*
* Win condition
* By: Benjamin Carlson
* What it does: Checks if the snake is the required length to win.
*/
void check_win()
{
    // calculate half the perimeter of the border -> P=(2(l+w)) / 2
    win_length = (2 * (LINES + COLS)) / 2;
    // move to top right corner and write the win_length
    move(0, 0);
    printw("%d", win_length);

    int length = calculate_snake_length(); // calculate length of snake
    // move(row / 2, col / 2);
    // printw("%d", length);

    // if snake is the required length, write message to screen and end game
    if (length >= win_length) // >= because we might go over. Ex, snake is one away from winning but eats a trophy of size 9
    {
        move(row / 2, col / 2);
        attron(A_STANDOUT);
        addstr("You won!");
        attroff(A_STANDOUT);
        refresh();
        sleep(2);
        raise(SIGINT); // raise interrupt signal to end game
    }
}

/*
* Trophy - Init
* By: Benjamin Carlson
* What it does: Creates a trophy at a random location with a random value between 1 and 9. Does not appear on the snake.
*/
void init_trophy()
{
    // get random number from 1 to 9
    trophy_value = rand() % 9 + 1;
    // get random location
    trophy_row = rand() % LINES;
    trophy_col = rand() % COLS;

    // if the trophy is on the snake, call init_trophy again
    struct node *scanner = head;
    while (scanner != NULL)
    {
        if (scanner->row == trophy_row && scanner->column == trophy_col)
            init_trophy();
        scanner = scanner->prev;
    }

    // if the trophy is on the border, call init_trophy again
    if (trophy_row == 1 || trophy_row == LINES - 1 || trophy_col == 0 || trophy_col == COLS - 1)
        init_trophy();

    // if the trophy is on the first row, call init_trophy again
    if (trophy_row == 0)
        init_trophy();

    // get trophy exprire time -> random value between 1 and 9
    trophy_expiration = (rand() % 9 + 1);

    // write trophy to screen
    move(trophy_row, trophy_col);
    printw("%d", trophy_value);
}

/*
* Trophy - Expire
* By: Benjamin Carlson
* What it does: Expires the trophy.
*/
void run_every_second_check_trophy()
{
    // check if trophy has expired
    if (trophy_expiration == 0)
    {
        // if trophy has expired, remove trophy from screen
        move(trophy_row, trophy_col);
        printw(" ");
        refresh();
        init_trophy();
    }
    else
    {
        trophy_expiration--;
    }
}