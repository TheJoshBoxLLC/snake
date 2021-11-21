/*
* Snake Game
* CS 355 Final Project
* Benjamin Carlson
*
* To compile: gcc -o snake snake.c -lcurses
* To run: ./snake
* To terminate: press any key
*/

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <curses.h>

// ************* constants ************* //
#define SOMETHING 1 // example

// end constants *************

// ************* variables ************* //
static unsigned int something = 1; // example

// end variables *************

// ************* function prototypes ************* //
void init_pit_border();

// end function prototypes *************

/*
* Main function
*/
int main()
{
    initscr();         // initialize the curses library
    clear();           // clear the screen
    curs_set(0);       // turn off cursor
    init_pit_border(); // init the pit border
    refresh();         // apply changes

    getch();  // wait for any user input
    endwin(); // end curses and reset the tty
    exit(1);  // exit successfully
}

/*
* Snake Pit
* By: Benjamin Carlson
* What it does: Initializes the snake pit. The pit takes up the entire terminal.
* We can achieve this by using LINES and COLS
*/
void init_pit_border()
{
    addstr("Welcome to our snake game!\n");

    for (int i = 1; i < LINES; i++)
    { // outer loop
        for (int j = 0; j < COLS; j++)
        {               // inner loop
            move(i, j); // move cursor to new position
            // only add "X" to the outer perimeter
            if (i == 1 || i == LINES - 1)
                addstr("-");
            else if (j == 0 || j == COLS - 1)
                addstr("|");
        }
    }
}