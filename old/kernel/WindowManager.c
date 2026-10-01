#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Shell/Shell.h"
#include "drivers/VBE/VBE.h"
#include "WindowManager.h"

typedef struct Window_ {
    bool isKernel;
    struct Window_ *next;

    void (*init)(void);
    void (*render)(void);
    void (*onInput)(uint16_t key);
} Window;


// the root window is always the direct kernel shell
Window Root = {
    .isKernel = true,
    .next = NULL,
    .init = SHELL_Init,
    .render = SHELL_Render,
    .onInput = SHELL_OnInput
};

int WINDOW_INDEX = 0;

void WINDOW_Init() {
    Root.init();

    // nothing else to init for now
}

void WINDOW_Render() {
    int i = 0;
    Window *current = &Root;
    while (i < WINDOW_INDEX && current != NULL) {
        current = current -> next;
        i++;
    }

    if (current == NULL) {
        VBE_ClearScreen(VBEC_PURPLE);
        return;
    }

    current->render();
}

void WINDOW_OnInput(uint16_t code) {
    int i = 0;
    Window *current = &Root;
    while (i < WINDOW_INDEX && current != NULL) {
        current = current -> next;
        i++;
    }

    if (current == NULL) {
        return;
    }

    current->onInput(code);
}

// TODO: later, user programs (apps) can run in seperate windows with their own graphics and stuff
// Eventually, kernel shell will be disabled (its too dangerous to let the regular user use it)
// at some point, i will allow windows to be draggable/ resizeable. but for now, they are all forced fullscreen