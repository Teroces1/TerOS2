#include <stdint.h>
#include <stdbool.h>
#include "../drivers/VBE/VBE.h"
#include "../drivers/Keyboard/keyboard.h"
#include "../../lib/string.h"
#include "Debug.h"


#define WIDTH 128
#define HEIGHT 48
#define TERMINAL_HEIGHT 256
#define TERMINAL_ROTATE_MASK 0xFF
#define BOTTOM_PADDING 5    // leave 5 lines at the bottom clear
#define BOTTOM_PADDING_WHEN_FOCUS 40    // always leave majority white space when CTRL+F
#define MAX_COMMAND_LENGTH (WIDTH*4+2)

char commandBuffer[MAX_COMMAND_LENGTH];    // make it big just in case
int commandLength = 0;
int commandCursor = 0;
char terminal[TERMINAL_HEIGHT][WIDTH*2+1];  // larger width just in case the entire line is filled with color symbols
bool snapToNewLine = false;
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

    for (int i = 0; i < MAX_COMMAND_LENGTH; i++) {
        commandBuffer[i] = ' ';
    }
    commandBuffer[MAX_COMMAND_LENGTH-1] = '\0';
}

char shellPrefix[] = "0g$KERNEL$> 0d";

void SHELL_Init() {
    shellPrefix[0] = 5; // color assignment code
    shellPrefix[12] = 5;

    clear();

    acceptCommand();

    // char digits[10];
    // STR_int2str(GetScreenSize(top, shellCursorX), digits, 10);
    // SHELL_Print(digits);
}


void acceptCommand() {
    SHELL_Print(shellPrefix);
    for (int i = 0; i < MAX_COMMAND_LENGTH; i++) {
        commandBuffer[i] = ' ';
    }
    commandBuffer[MAX_COMMAND_LENGTH-1] = '\0';
    commandCursor = 0;

    canType = true;
}



// will be run from a timer interrupt, so no other interrupts can happen while this is running. is this right?
void SHELL_Render() {
    if (runningCommandFlag) {
        RunCommand();
        modifiedSinceLastRefresh = true;
    }

    if (modifiedSinceLastRefresh && updateDone) {
        SHELL_hidden_print(commandBuffer);
        int screenSize = GetScreenSize(shellCursorY, shellCursorX);
        int virtualCursorX = (screenSize + commandCursor) % WIDTH;
        if (virtualCursorX == (screenSize + commandCursor))
            virtualCursorX = shellCursorX + commandCursor;

        int virtualCursorY = (shellCursorY + (screenSize + commandCursor) / WIDTH) & TERMINAL_ROTATE_MASK;

        modifiedSinceLastRefresh = false;

        VBE_ClearScreen(VBEC_BLACK);

        for (int i = 0; i < HEIGHT; i++) {
            int index = (first + i) & TERMINAL_ROTATE_MASK;
            // draw entry to screen
            int cursor = index == virtualCursorY ? virtualCursorX : -1;
            

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
    if (shellCursorY == top) {
        top = (top+1) & TERMINAL_ROTATE_MASK;

        // clear the line
        for (int j = 0; j <= WIDTH *2; j++) {
            terminal[top][j] = ' ';
        }
    }
    shellCursorY = (shellCursorY+1) & TERMINAL_ROTATE_MASK;
    shellCursorX = 0;
    topScreenSize = 0;

    int currentOutputHeight = ((top - first + TERMINAL_HEIGHT) & TERMINAL_ROTATE_MASK);

    int cursorToFirstHeight = ((shellCursorY - first + TERMINAL_HEIGHT) & TERMINAL_ROTATE_MASK);

    // if (cursorToFirstHeight > currentOutputHeight) {
    //     top = (top+1) & TERMINAL_ROTATE_MASK;

    //     // clear the line
    //     for (int j = 0; j <= WIDTH *2; j++) {
    //         terminal[top][j] = ' ';
    //     }
    // }
    

    // // if (((top >= first) && (HEIGHT - (top - first) < BOTTOM_PADDING)) || (HEIGHT - (TERMINAL_HEIGHT - (first - top) - 1) < BOTTOM_PADDING)) {
    if (snapToNewLine && HEIGHT - cursorToFirstHeight < BOTTOM_PADDING) {
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

int GetScreenSize(int index, int cursor) {
    int size = 0;
    for (int i=0; i < cursor; i++) {
        if (terminal[index][i] == 5) {
            i++;
        } else {
            size++;
        }
    }

    return size;
}

void SHELL_Print(char *str) {
    updateDone = false;

    int currentScreenSize = GetScreenSize(shellCursorY, shellCursorX);
    
    
    int i = 0;
    while (str[i] != '\0') {
        terminal[shellCursorY][shellCursorX] = str[i];
        if (str[i] == (char) 5) {   // color escape code
            i++;
            shellCursorX++;

            if (str[i] == '\0')
                i--;
            else {
                terminal[shellCursorY][shellCursorX] = str[i];
                i++;
                shellCursorX++;
            }
        } else if (str[i] == '\n') {
            terminal[shellCursorY][shellCursorX] = ' ';
            _nextLine();
            i++;
            currentScreenSize = 0;
        } else {
            i++;
            shellCursorX++;
            currentScreenSize++;
        }

        if (currentScreenSize >= WIDTH || shellCursorX >= WIDTH*2-2) {
            terminal[shellCursorY][shellCursorX] = '\0';
            _nextLine();
            currentScreenSize = 0;
        }
    }
    
    modifiedSinceLastRefresh = true;
    updateDone = true;
}

void SHELL_hidden_print(char *str) {
    int savedCursorX = shellCursorX;
    int savedCursorY = shellCursorY;
    snapToNewLine = false;

    SHELL_Print(str);

    snapToNewLine = true;
    shellCursorX = savedCursorX;
    shellCursorY = savedCursorY;
}

void EnterCommand() {
    updateDone = false;

    canType = false;

    // just to make sure command doesnt take up more space than it needs to on the console
    for (int i = MAX_COMMAND_LENGTH-2; i>=0; i--) {
        if (commandBuffer[i] == ' ' || commandBuffer[i] == '\n' || commandBuffer[i] == '\0') {
            commandBuffer[i] = '\0';
        } else {
            break;
        }
    }
    SHELL_Print(commandBuffer);

    SHELL_putChar('\n');

    
    modifiedSinceLastRefresh = true;
    updateDone = true;


    runningCommandFlag = true;
}

void RunCommand() {
    // match and run command
    if (STR_strcmp(commandBuffer, "hello") == 0) {
        SHELL_Print("world\n");
    } else if (STR_strcmp(commandBuffer, "bootinfo") == 0) {
        DEBUG_print_entry();
    } else if (STR_strcmp(commandBuffer, "cli") == 0) {
        clear();
    } else if (STR_strcmp(commandBuffer, "help") == 0) {
        SHELL_Print(STR_CEncode(
            "  \\5yhello    \\5d- Returns 'world' for testing.\n"
            "  \\5ybootinfo \\5d- Prints most of the information in the entry packet.\n"
            "  \\5ycli      \\5d- Clears the screen.\n"
            "  \\5yhelp     \\5d- Prints out a list of commands.\n"
            "  \\5yvbe      \\5d- Prints out the VBE information.\n"
            "  \\5yvbemode [mode index] \\5d- Prints out the information on a given VBE mode. If no mode given, prints out a list of vbe modes.\n"
        ));
    } else if (STR_strcmp(commandBuffer, "vbe") == 0) {
        DEBUG_print_VBE();
    } else if (STR_strncmp(commandBuffer, "vbemode", 7) == 0) {
        if (commandBuffer[8] >= '0' && commandBuffer[8] <= '9') {
            int arg = STR_str2int(commandBuffer+8);
            DEBUG_print_mode_info(arg);
        } else {
            DEBUG_print_modes();
        }        
    }


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

void backspaceOnCommand() {
    if (commandCursor <= 0) return;

    for (int i = commandCursor; i <= MAX_COMMAND_LENGTH-2; i++) {
        commandBuffer[i-1] = commandBuffer[i];
    }
    commandBuffer[MAX_COMMAND_LENGTH-2] = ' ';

    commandCursor --;
}

void insertCharacterOnCommand(char c) {
    if (commandCursor > MAX_COMMAND_LENGTH-2) {
        commandCursor = MAX_COMMAND_LENGTH-1;
        return;
    }

    for (int i = MAX_COMMAND_LENGTH-3; i >= commandCursor; i--) {
        commandBuffer[i+1] = commandBuffer[i];
    }

    commandBuffer[commandCursor] = c;
    commandCursor++;
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

        if (canType && commandCursor < WIDTH*4) {
            if (secondary) {
                key = KEYBOARD_SECONDARY[key];
            }
            
            insertCharacterOnCommand(key);
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
                backspaceOnCommand();
                break;
            case 'L':
                if (commandCursor > 0)
                    commandCursor --;
                break;
            case 'R':
                if (commandCursor < MAX_COMMAND_LENGTH -1)
                    commandCursor ++;
                break;
            // case '\b':
            // case 'L':   // left
            //     if (shellCursorX == 0) {
            //         // needs to decrement cursor y, but cant if its going to be the top index
            //         if (shellCursorY-1 == top)
            //             break;

            //         shellCursorY = (shellCursorY - 1) & TERMINAL_ROTATE_MASK;
            //         shellCursorX = getLastCharacter(shellCursorY);
            //         if (shellCursorX < WIDTH*2)
            //             shellCursorX ++;    // normally the cursor would go after the last character
            //     } else {
            //         shellCursorX --;
            //         if (key == '\b')
            //             terminal[shellCursorY][shellCursorX] = ' ';
            //     }
            //     break;
            // case 'R':   // right
            //     int lastChar = getLastCharacter(shellCursorY);
            //     if (shellCursorX > lastChar) {
            //         // needs to incremement cursor y, but cant if its at the top
            //         if (shellCursorY == top)
            //             break;

            //         shellCursorY = (shellCursorY + 1) & TERMINAL_ROTATE_MASK;
            //         shellCursorX = 0;
            //     } else {
            //         shellCursorX ++;
            //     }
            //     break;
        }
    }

    modifiedSinceLastRefresh = true;
    updateDone = true;
}