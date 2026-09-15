#include <stdint.h>
#include <stdbool.h>
#include "../drivers/VBE/VBE.h"


#define WIDTH 128
#define HEIGHT 48
#define TERMINAL_HEIGHT 256
#define TERMINAL_ROTATE_MASK 0xFF
#define BOTTOM_PADDING 5    // leave 5 lines at the bottom clear


char terminal[TERMINAL_HEIGHT][WIDTH*2+1];  // larger width just in case the entire line is filled with color symbols
volatile int top = 0;    // always points to the last entry to the terminal
volatile int cursor = 0; // always points to the 1st entry visible on the screen
volatile bool modifiedSinceLastRefresh = false;
volatile bool updateDone = true;      // if false, then render will skip that refresh frame until its done

void clear() {
    updateDone = false;
    for (int i = 0; i < TERMINAL_HEIGHT; i++) {
        for (int j = 0; j <= WIDTH *2; j++) {
            terminal[i][j] = ' ';
        }
    }

    top = 0;
    cursor = 0;
    modifiedSinceLastRefresh = true;
    updateDone = true;
}

void SHELL_Init() {
    clear();
}


// will be run from a timer interrupt, so no other interrupts can happen while this is running. is this right?
void SHELL_Render() {
    if (modifiedSinceLastRefresh && updateDone) {
        modifiedSinceLastRefresh = false;

        VBE_ClearScreen(VBEC_BLACK);

        for (int i = 0; i < HEIGHT; i++) {
            int index = (cursor + i) & TERMINAL_ROTATE_MASK;
            // draw entry to screen
            VBE_PutTerminalString(terminal[index], 0, i << 4, WIDTH*2+1, VBEC_WHITE, VBEC_BLACK);   // has a max string limit of WIDTH*2+1

            if (index == top) {
                // reached the last point in history; must exit
                break;
            }
        }
    }
}

void _nextLine() {
    top = (top+1) & TERMINAL_ROTATE_MASK;
    // if (((top >= cursor) && (HEIGHT - (top - cursor) < BOTTOM_PADDING)) || (HEIGHT - (TERMINAL_HEIGHT - (cursor - top) - 1) < BOTTOM_PADDING)) {
    int currentOutputHeight = ((top - cursor + TERMINAL_HEIGHT) & TERMINAL_ROTATE_MASK);
    if (HEIGHT - currentOutputHeight < BOTTOM_PADDING) {
        //cursor must shift down as well or else the top wouldnt be in frame + the padding
        cursor = (cursor+1) & TERMINAL_ROTATE_MASK;
    }

    // clear the line
    for (int j = 0; j <= WIDTH *2; j++) {
        terminal[top][j] = ' ';
    }
}

void SHELL_Print(char *str) {
    updateDone = false;
    
    int i = 0;
    int entrySize = 0;
    int screenSize = 0;
    while (str[i] != '\0') {
        terminal[top][entrySize] = str[i];
        if (str[i] == (char) 5) {   // color escape code
            i++;
            entrySize++;

            if (str[i] == '\0')
                i--;
            else {
                terminal[top][entrySize] = str[i];
                i++;
                entrySize++;
            }
        } else if (str[i] == '\n') {
            terminal[top][entrySize] = ' ';
            _nextLine();
            entrySize = 0;
            screenSize = 0;
            i++;
        } else {
            i++;
            entrySize++;
            screenSize++;
        }

        if (screenSize >= WIDTH || entrySize >= WIDTH*2-2) {
            terminal[top][entrySize] = '\0';
            _nextLine();
            entrySize = 0;
            screenSize = 0;
        }
    }
    
    modifiedSinceLastRefresh = true;
    updateDone = true;
}

void SHELL_OnInput(uint16_t key) {

}