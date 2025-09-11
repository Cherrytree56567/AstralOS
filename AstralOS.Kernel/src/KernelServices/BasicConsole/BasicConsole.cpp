#include "BasicConsole.h"

BasicConsole::BasicConsole(FrameBuffer fb) : pFramebuffer(fb) {
    CursorPosition.X = 10;
    CursorPosition.Y = 10;
}

void BasicConsole::putChar(unsigned int colour, char chr, unsigned int xOff, unsigned int yOff) {
    unsigned int* pixPtr = (unsigned int*)pFramebuffer.BaseAddress;

    // Check if the character is within the font range
    if (chr < 0 || chr >= 256) return;  // Assuming 256 characters for simplicity

    const uint8_t* fontPtr = font + (chr * 16);  // Each character takes 16 bytes (8 rows)

    for (unsigned long y = 0; y < 16; y++) { // Iterate over 16 rows
        for (unsigned long x = 0; x < 8; x++) { // Iterate over 8 columns
            // Check if the bit is set
            if (fontPtr[y] & (0b10000000 >> x)) {
                // Calculate pixel position in framebuffer
                unsigned long pixelIndex = (x + xOff) + ((y + yOff) * pFramebuffer.PixelsPerScanLine);
                if (pixelIndex < (pFramebuffer.Width * pFramebuffer.Height)) { // Ensure we don't write out of bounds
                    pixPtr[pixelIndex] = colour; // Write the pixel colour
                }
            }
        }
    }
}

void BasicConsole::Print(const char* str, unsigned int colour) {
    const char* chr = str;
    while (*chr != 0) {
        if (*chr == '\n') {
            CursorPosition.X = 10;           // Reset X position for new line
            CursorPosition.Y += 16;         // Move cursor down by 16 pixels
        }
        else {
            putChar(colour, *chr, CursorPosition.X, CursorPosition.Y);
            CursorPosition.X += 8;          // Move cursor right by 8 pixels for the next character
            if (CursorPosition.X + 10 > pFramebuffer.Width) {
                CursorPosition.X = 10;       // Reset X position
                CursorPosition.Y += 16;     // Move cursor down by 16 pixels for a new line
            }
        }
        chr++;
    }
}

void BasicConsole::Println(const char* str, unsigned int colour) {
    const char* chr = str;
    while (*chr != 0) {
        if (*chr == '\n') {
            CursorPosition.X = 10;           // Reset X position for new line
            CursorPosition.Y += 16;         // Move cursor down by 16 pixels
        }
        else {
            putChar(colour, *chr, CursorPosition.X, CursorPosition.Y);
            CursorPosition.X += 8;          // Move cursor right by 8 pixels for the next character
            if (CursorPosition.X + 10 > pFramebuffer.Width) {
                CursorPosition.X = 10;       // Reset X position
                CursorPosition.Y += 16;     // Move cursor down by 16 pixels for a new line
            }
        }
        chr++;
    }

    CursorPosition.X = 10;           // Reset X position for new line
    CursorPosition.Y += 16;         // Move cursor down by 16 pixels
}

void BasicConsole::ClearLines(unsigned int lineCount) {
    unsigned int fontHeight = 16; // or whatever your font height is
    unsigned int clearHeight = fontHeight * lineCount;

    for (unsigned int y = 0; y < clearHeight; y++) {
        for (unsigned int x = 0; x < pFramebuffer.Width; x++) {
            ((uint32_t*)pFramebuffer.BaseAddress)[y * pFramebuffer.PixelsPerScanLine + x] = 0x000000;
        }
    }
}