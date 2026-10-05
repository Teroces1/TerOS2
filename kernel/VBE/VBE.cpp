#include "FONT.h"
#include "VBE.hpp"

constexpr int CHARACTER_WIDTH = 8;
constexpr int CHARACTER_HEIGHT = 16;

using namespace VBE;

// Display::Display(int resX, int resY, int bpp, int bytesPerScanLine, uint8_t *frameBuffer, int redPos, int greenPos, int bluePos) : 
//     ResX(resX), ResY(resY), BPP(bpp), BytesPerScanLine(bytesPerScanLine), FrameBuffer(frameBuffer), RPos(redPos), GPos(greenPos), BPos(bluePos)
//     {}

Display::Display(const volatile EntryTypes::VBEModeInfo &ModeInfo, volatile uint8_t *frameBufferVirtual) :
    ResX(ModeInfo.x_resolution),
    ResY(ModeInfo.y_resolution),
    ColorEnabled((ModeInfo.mode_attributes & 0x08) != 0),
    FrameBuffer(frameBufferVirtual),
    BytesPerScanLine(ModeInfo.bytes_per_scanline),
    BPP(ModeInfo.bits_per_pixel),
    // FrameBuffer(ModeInfo.physical_base_ptr),
    RPos(ModeInfo.red_field_position),
    GPos(ModeInfo.green_field_position),
    BPos(ModeInfo.blue_field_position),
    _totalBytes(ModeInfo.bytes_per_scanline * ModeInfo.y_resolution),
    _totalAllPixels(_totalBytes / ((BPP+7) / 8)),
    _pixelsPerScanLine(BytesPerScanLine / ((BPP+7) / 8))
    {}

void Display::SetFrameBuffer(volatile uint8_t *frameBufferVirtual) {
    FrameBuffer = frameBufferVirtual;
}

void Display::ClearFront(Color color) {
    if (BPP == 32) {
        volatile uint32_t* raw_fb = reinterpret_cast<volatile uint32_t*>(FrameBuffer);
        
        for (int i = 0; i < _totalAllPixels; i++) {
            raw_fb[i] = color;
        }
    // others not implemented yet
    }
}



void Display::PutPixelFront(int x, int y, Color color) {
    if (BPP == 32) {
        volatile uint32_t* raw_fb = reinterpret_cast<volatile uint32_t*>(FrameBuffer);

        raw_fb[y*_pixelsPerScanLine + x] = color;
    }
}

void Display::DrawRectangleFront(int x, int y, int w, int h, Color color) {
    if (BPP == 32) {
        volatile uint32_t* raw_fb = reinterpret_cast<volatile uint32_t*>(FrameBuffer);
        for (int row = y; row < y+h; row++) {
            int buffrow = row*_pixelsPerScanLine;
            for (int col = x; col < x+w; col++) {
                raw_fb[buffrow + col] = color;
            }
        }; 
    }
}

void Display::PutCharFront(char c, int size, int x, int y, Color color, Color backColor) {
    unsigned int charIndex = static_cast<unsigned char>(c);
    if (charIndex >= 128) charIndex = ' ';

    const uint8_t *map = FONT[fontID][static_cast<uint8_t>(c)];

    if (BPP == 32) {
        volatile uint32_t* raw_fb = reinterpret_cast<volatile uint32_t*>(FrameBuffer);

        if (size == 1) {
            for (int row = 0; row < 16; row++) {
                int buffrow = (row + y)*_pixelsPerScanLine;
                for (int col = 0; col < 8; col++) {
                    if ((map[row] & (0x80 >> col)) > 0) {
                        raw_fb[buffrow + col + x] = color;
                    } else {
                        raw_fb[buffrow + col + x] = backColor;
                    }
                }
            }
        } else {
            for (int row = 0; row < 16; row++) {
                for (int realRow = row*size; realRow <size*(row+1); realRow++) {
                    int buffrow = (realRow + y)*_pixelsPerScanLine;
                    for (int col = 0; col < 8; col++) {
                        if ((map[row] & (0x80 >> col)) > 0) {
                            for (int realCol = size*col; realCol < size*(col+1); realCol++) {
                                raw_fb[buffrow + realCol + x] = color;
                            }
                        } else {
                            for (int realCol = size*col; realCol < size*(col+1); realCol++) {
                                raw_fb[buffrow + realCol + x] = backColor;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Display::PutStringFront(const char *c, int size, int x, int y, Color color, Color backColor) {
    int i = 0;
    while (c[i] != '\0') {
        PutCharFront(c[i], size, x + i*8*size, y, color, backColor);
        i++;
    }
}

void Display::PutULLFront(uint64_t num, uint64_t base, int x, int y, Color color, Color backColor) {
    char digits[40];
    STR_64int2str(num, digits, base);
    PutStringFront(digits, x, y, color, backColor);
}

char digits[] = "0123456789ABCDEF";

char* Display::STR_64int2str(uint64_t value, char *buffer, const uint64_t base) {
    if (!buffer) return nullptr;
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return buffer;
    }

    int i = 0;
    do {
        buffer[i++] = digits[value % base];
        value /= base;
    } while (value > 0);


    buffer[i] = '\0';

    // reverse
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = buffer[j];
        buffer[j] = buffer[k];
        buffer[k] = temp;
    }

    return buffer;
}