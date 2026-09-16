#include <stdint.h>
#include <stdbool.h>
#include "../drivers/VBE/VBE.h"
#include "../drivers/Keyboard/keyboard.h"


#define WIDTH 128
#define HEIGHT 48
#define TERMINAL_HEIGHT 256
#define TERMINAL_ROTATE_MASK 0xFF
#define BOTTOM_PADDING 5    // leave 5 lines at the bottom clear
#define BOTTOM_PADDING_WHEN_FOCUS 40    // always leave majority white space when CTRL+F

char commandBuffer[WIDTH*4+1];    // make it big just in case
int commandLength = 0;
int commandCursor = 0;
char terminal[TERMINAL_HEIGHT][WIDTH*2+1];  // larger width just in case the entire line is filled with color symbols
volatile int top = 0;    // always points to the last entry to the terminal
volatile int topScreenSize = 0; // the screen size of the current entry
volatile int first = 0; // always points to the 1st entry visible on the screen
volatile int shellCursorX = 0;
volatile int shellCursorY = 0;
volatile bool modifiedSinceLastRefresh = false;
volatile bool updateDone = true;      // if false, then render will skip that refresh frame until its done
volatile bool canType = false;
volatile bool runningCommandFlag = false;

void clear() {
    updateDone = false;
    for (int i = 0; i < TERMINAL_HEIGHT; i++) {
        for (int j = 0; j <= WIDTH *2; j++) {
            terminal[i][j] = ' ';
        }
    }

    top = 0;
    first = 0;
    shellCursorX = 0;
    shellCursorY = 0;
    topScreenSize = 0;
    modifiedSinceLastRefresh = true;
    updateDone = true;
}

char shellPrefix[] = "0g$KERNEL$> 0d";

void SHELL_Init() {
    shellPrefix[0] = 5; // color assignment code
    shellPrefix[12] = 5;

    clear();

    acceptCommand();
}


void acceptCommand() {
    SHELL_Print(shellPrefix);
    canType = true;
}

// will be run from a timer interrupt, so no other interrupts can happen while this is running. is this right?
void SHELL_Render() {
    if (runningCommandFlag) {
        RunCommand();
    }

    if (modifiedSinceLastRefresh && updateDone) {
        modifiedSinceLastRefresh = false;

        VBE_ClearScreen(VBEC_BLACK);

        for (int i = 0; i < HEIGHT; i++) {
            int index = (first + i) & TERMINAL_ROTATE_MASK;
            // draw entry to screen
            int cursor = index == shellCursorY ? shellCursorX : -1;

            VBE_PutTerminalString(terminal[index], 0, i << 4, WIDTH*2+1, cursor, VBEC_WHITE, VBEC_BLACK);   // has a max string limit of WIDTH*2+1

            if (index == top) {
                // reached the last point in history; must exit
                break;
            }
        }

        VBE_Swap();
    }
}

void _nextLine() {
    shellCursorY = (shellCursorY+1) & TERMINAL_ROTATE_MASK;
    shellCursorX = 0;
    topScreenSize = 0;

    int currentOutputHeight = ((top - first + TERMINAL_HEIGHT) & TERMINAL_ROTATE_MASK);

    int cursorToFirstHeight = ((shellCursorY - first + TERMINAL_HEIGHT) & TERMINAL_ROTATE_MASK);

    if (cursorToFirstHeight > currentOutputHeight) {
        top = (top+1) & TERMINAL_ROTATE_MASK;

        // clear the line
        for (int j = 0; j <= WIDTH *2; j++) {
            terminal[top][j] = ' ';
        }
    }
    

    // if (((top >= first) && (HEIGHT - (top - first) < BOTTOM_PADDING)) || (HEIGHT - (TERMINAL_HEIGHT - (first - top) - 1) < BOTTOM_PADDING)) {
    if (HEIGHT - cursorToFirstHeight < BOTTOM_PADDING) {
        //first must shift down as well or else the top wouldnt be in frame + the padding
        first = (first + BOTTOM_PADDING - (HEIGHT - cursorToFirstHeight)) & TERMINAL_ROTATE_MASK;
    }
}

void _enforceVisibleCursor() {
    int cursorToFirstHeight = ((shellCursorY - first + TERMINAL_HEIGHT) & TERMINAL_ROTATE_MASK);


    if (HEIGHT - cursorToFirstHeight < 0) {
        first = (first + BOTTOM_PADDING_WHEN_FOCUS - (HEIGHT - cursorToFirstHeight)) & TERMINAL_ROTATE_MASK;
    } else if (HEIGHT - cursorToFirstHeight < BOTTOM_PADDING) {
        // if the cursor is on screen just in that padding area, only shift down to padding
        first = (first + BOTTOM_PADDING - (HEIGHT - cursorToFirstHeight)) & TERMINAL_ROTATE_MASK;
    }
}

void SHELL_putChar(char c) {
    terminal[shellCursorY][shellCursorX] = c;
    if (c == '\n') {
        terminal[shellCursorY][shellCursorX] = ' ';
        _nextLine();
    } else {
        shellCursorX++;
        topScreenSize++;
    }

    if (topScreenSize >= WIDTH || shellCursorX >= WIDTH*2-2) {
        terminal[shellCursorY][shellCursorX] = '\0';
        _nextLine();
    }
}

void SHELL_Print(char *str) {
    updateDone = false;
    
    int i = 0;
    while (str[i] != '\0') {
        terminal[top][shellCursorX] = str[i];
        if (str[i] == (char) 5) {   // color escape code
            i++;
            shellCursorX++;

            if (str[i] == '\0')
                i--;
            else {
                terminal[top][shellCursorX] = str[i];
                i++;
                shellCursorX++;
            }
        } else if (str[i] == '\n') {
            terminal[top][shellCursorX] = ' ';
            _nextLine();
            i++;
        } else {
            i++;
            shellCursorX++;
            topScreenSize++;
        }

        if (topScreenSize >= WIDTH || shellCursorX >= WIDTH*2-2) {
            terminal[top][shellCursorX] = '\0';
            _nextLine();
        }
    }
    
    modifiedSinceLastRefresh = true;
    updateDone = true;
}

void EnterCommand() {
    updateDone = false;

    canType = false;

    SHELL_putChar('\n');

    
    modifiedSinceLastRefresh = true;
    updateDone = true;


    runningCommandFlag = true;
    
    // match and run command
}

void RunCommand() {


    runningCommandFlag = false;
    acceptCommand();
}

int getLastCharacter(int entryIndex) {
    int i = WIDTH*2;
    while ((i >= 0) && (terminal[entryIndex][i] == '\0' || terminal[entryIndex][i] == ' ' || terminal[entryIndex][i] == '\n')) {
        i--;
    }
    // returns -1 if line is empty
    return i;
}

int backspaceOnCommand() {
    if (commandCursor == 0) return;

    // for (int i = commandCursor)

    commandCursor --;
}

void SHELL_OnInput(uint16_t keycode) {
    uint8_t control = (uint8_t) (keycode >> 8);
    uint8_t key = (uint8_t) (keycode & 0xFF);

    bool pressed = (bool) (control & 0x01);
    if (!pressed) return;

    bool other = (bool) ((control >> 1) & 0x01);    // shift, ctrl, alt, capslock
    bool special = (bool) ((control >> 2) & 0x01);
    bool secondary = (bool) ((control >> 3) & 0x01);

    bool ctrl = (bool) ((control >> 6) & 0x01);
    bool alt = (bool) ((control >> 7) & 0x01);


    

    updateDone = false;
    if (ctrl) {
        switch (key) {
            case 'f':
                _enforceVisibleCursor();
                break;
        }
    } else if (alt) {
        
    } else if (!(other || special)) {
        // regular key press

        if (canType && commandLength < WIDTH*4) {
            if (secondary) {
                key = KEYBOARD_SECONDARY[key];
            }

            SHELL_putChar(key);
            commandLength++;
        }
    } else if (special == 1) {
        // control key
        switch (key) {
            case '\n':  // enter
                EnterCommand();
                break;
            
            case 'U':   // up
                int nextFirst = (first - 1) & TERMINAL_ROTATE_MASK;
                if (nextFirst != top) {
                    first = nextFirst;
                }
                break;
            case 'D':   // down
                if (first != top) {
                    first = (first + 1) & TERMINAL_ROTATE_MASK;
                }
                break;
            case '\b':
            case 'L':   // left
                if (shellCursorX == 0) {
                    // needs to decrement cursor y, but cant if its going to be the top index
                    if (shellCursorY-1 == top)
                        break;

                    shellCursorY = (shellCursorY - 1) & TERMINAL_ROTATE_MASK;
                    shellCursorX = getLastCharacter(shellCursorY);
                    if (shellCursorX < WIDTH*2)
                        shellCursorX ++;    // normally the cursor would go after the last character
                } else {
                    shellCursorX --;
                    if (key == '\b')
                        terminal[shellCursorY][shellCursorX] = ' ';
                }
                break;
            case 'R':   // right
                int lastChar = getLastCharacter(shellCursorY);
                if (shellCursorX > lastChar) {
                    // needs to incremement cursor y, but cant if its at the top
                    if (shellCursorY == top)
                        break;

                    shellCursorY = (shellCursorY + 1) & TERMINAL_ROTATE_MASK;
                    shellCursorX = 0;
                } else {
                    shellCursorX ++;
                }
                break;
        }
    }

    modifiedSinceLastRefresh = true;
    updateDone = true;
}