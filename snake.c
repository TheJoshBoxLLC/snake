/*
* Snake Game
* CS 355 Final Project
* Benjamin Carlson, Josh Magalhaes, Erik Marrro, Nick Sbado
*
* To compile: gcc -o snake snake.c -lcurses
* To run: ./snake
* To terminate: press command + c
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

// ************* variables ************* //
int row = 0;                // current row
int col = 0;                // current column
char key = 'd';             // key input
short ticks = 0;            // ticks make up the game clock. ticks will eventually get faster when trophies are introduced
short timeUnit = 128;       // timeUnit is made up of ticks
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
struct node *head; // snake head
struct node *tail; // snake tail
// end variables *************

// ************* function prototypes ************* //
void init_pit_border();
void init_snake(int, int);
void move_snake();
void grow_snake(int);
char choose_random_direction();
void run_when_time_runs_out();
void set_nodelay();
void end_snake_game(int signum);
// end function prototypes *************

int main()
{
    signal(SIGINT, end_snake_game);  // when user terminates program, end game!
    initscr();                       // start curses
    clear();                         // clear the screen
    curs_set(0);                     // turn off cursor
    init_pit_border();               // initialize pit border
    key = choose_random_direction(); // set a random initial direction for the snake. For first deliverable this is to the right
    speed.tv_sec = 0;                // sleep timer
    speed.tv_nsec = 781250;          // 1/128th of a second in nanoseconds
    char input;                      // the key input
    keypad(stdscr, TRUE);            // enable keypad mode
    row = LINES;                     // num rows
    col = COLS;                      // num columns
    // center the snake
    snake_head_y = row / 2;
    snake_head_x = col / 2;
    init_snake(snake_head_y, snake_head_x); // initialize snake of size 5 in the center of the screen

    // Print welcome message to center of screen
    move(row / 2, col / 2 - 50 / 2);
    addstr("Welcome to our snake game! Press any key to start.");
    move(row - 1, col - 1);

    refresh(); // buffer -> screen

    getchar();     // wait for user input
    noecho();      // don't echo user input
    set_nodelay(); // set no delay mode

    // clear "Welcome to our snake game! Press any key to start."
    move(row / 2, col / 2 - 50 / 2);
    addstr("                                                  ");
    move(row - 1, col - 1);

    // start game and loop until user terminates
    // By: Benjamin Carlson
    while (1)
    {
        while ((input = getch()) == ERR) // if no input, continue
        {
            nanosleep(&speed, &rem); // sleep for 1/128th of a second
            ticks++;                 // increment ticks

            if (ticks % timeUnit == 0) // if ticks is a multiple of time unit == 0, increment by 1 gameunit
            {
                run_when_time_runs_out();
            }
        }

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
    addstr("Welcome to our snake game! This is where trophy and other info will go\n");

    for (int i = 1; i < LINES; i++)
    { // outer loop
        for (int j = 0; j < COLS; j++)
        {               // inner loop
            move(i, j); // move cursor to new position
            // only add "-" or "|" to the outer perimeter
            if (i == 1 || i == LINES - 1)
                addstr("-");
            else if (j == 0 || j == COLS - 1)
                addstr("|");
        }
    }
}

/*
* Snake
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
* Snake
* By: Benjamin Carlson
* What it does: Grow the snake by a specified amount.
*/
void grow_snake(int num_to_grow_by)
{
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
}

/*
* Snake
* By: Benjamin Carlson
* What it does: Move the snake.
*/
void move_snake()
{
    int y = head->row;                 // y coordinate of the snake's head
    int x = head->column;              // x coordinate of the snake's head
    head->row += directionY;           // move snake head in direction of directionY
    head->column += directionX;        // move snake head in direction of directionX
    move(head->row, head->column);     // move cursor to new position
    addstr("O");                       // add "O" to the screen. This is the snake head
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
}

/*
* Snake Direction
* By: Benjamin Carlson
* What it does: Set the initial direction of the snake.
*/
char choose_random_direction()
{
    // this will eventually be the random direction. For first deliverable it will be to the right
    directionY = 0;
    directionX = 1;
    return 'd';
}

/*
* Run When Time Runs Out
* By: Benjamin Carlson
* What it does: refreshes the screen to move the snake after a certian amount of time. This is what we will call "time running out". In this first deliverable,
* it will be at a standard rate, but in the final, it will be determined by the length of the snake.
*/
void run_when_time_runs_out()
{
    ticks = 0;    // reset ticks
    move_snake(); // move snake
    refresh();    // buffer -> screen
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