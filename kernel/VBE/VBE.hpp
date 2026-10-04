#ifndef VBE_H
#define VBE_H

#include "../types.hpp"
using namespace Kernel;

namespace VBE {
    using Color = uint32_t;

    constexpr uint32_t RED = 0x00FF0000;
    constexpr uint32_t BLUE = 0x000000FF;
    constexpr uint32_t GREEN = 0x0000FF00;
    constexpr uint32_t BLACK = 0x00000000;
    constexpr uint32_t WHITE = 0x00FFFFFF;
    constexpr uint32_t YELLOW = 0x00FFFF00;
    constexpr uint32_t CYAN = 0x0000FFFF;
    constexpr uint32_t PURPLE = 0x00FF00FF;
    constexpr uint32_t CLEAR = 0xFF000000;

    class Display {
    public:
        const int ResX;
        const int ResY;
        const bool ColorEnabled {true};

        // Display (int resX, int resY, int bpp, int bytesPerScanLine, uint8_t *frameBuffer, int redPos, int greenPos, int bluePos);
        Display (const volatile EntryTypes::VBEModeInfo &ModeInfo, volatile uint8_t *frameBufferVirtual);
        
        void SetFrameBuffer(volatile uint8_t *frameBufferVirtual);

        void ClearFront(Color color);
        inline void ClearFront() {
            ClearFront(0);
        }
        inline Color GetRGB(int r, int g, int b) {
            return r << RPos | g << GPos | b << BPos;
        }

        void PutPixelFront(int x, int y, Color color);
        void DrawRectangleFront(int x, int y, int w, int h, Color color);
        void PutCharFront(char c, int x, int y, Color color, Color backColor);
        inline void PutCharFront(char c, int x, int y, Color color) {
            PutCharFront(c, x, y, color, 0);
        }
        inline void PutCharFront(char c, int x, int y) {
            PutCharFront(c, x, y, 0xFFFFFFFF, 0);
        }

        void PutStringFront(const char *c, int x, int y, Color color, Color backColor);
        inline void PutStringFront(const char *c, int x, int y, Color color) {
            PutStringFront(c, x, y, color, 0);
        }
        inline void PutStringFront(const char *c, int x, int y) {
            PutStringFront(c, x, y, 0xFFFFFFFF, 0);
        }

        void PutULLFront(uint64_t num, uint64_t base, int x, int y, Color color, Color backColor);

        inline void PutULLFront(uint64_t num, uint64_t base, int x, int y) {
            PutULLFront(num, base, x, y, 0xFFFFFFFF, 0);
        }

        void testPrint(const char* c, uint64_t data, uint64_t base) {
            PutStringFront(c, 16, testPrintLine*16);
            int len = 0;
            while (c[len] != '\0') {
                len++;
            }
            PutULLFront(data, base, 16+8*len, (testPrintLine++)*16);
        }
        void testPrint(const char* c) {
            PutStringFront(c, 16, (testPrintLine++)*16);
        }

    private:
        volatile uint8_t *FrameBuffer;
        const int BytesPerScanLine;
        const int BPP;
        const int RPos {0};
        const int GPos {0};
        const int BPos {0};

        const int _totalBytes;
        const int _totalAllPixels;
        const int _pixelsPerScanLine;

        int fontID {2};

        char* STR_64int2str(uint64_t value, char *buffer, const uint64_t base);

        int testPrintLine = 5;
    };
}

#endif