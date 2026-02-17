#include "BasicConsole.h"
#include "../../Utils/cpu.h"
#include "../KernelServices.h"

BasicConsole::BasicConsole(FrameBuffer fb) : pFramebuffer(fb) {
    CursorPosition.X = 10;
    CursorPosition.Y = 10;
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

int is_transmit_empty() {
    return inb(0x3F8 + 5) & 0x20;
}

void serial_write_char(char a) {
    while (is_transmit_empty() == 0);
    outb(0x3F8, a);
}

void serial_write(const char* str) {
    while (*str) {
        if (*str == '\n')
            serial_write_char('\r'); // so newlines look right
        serial_write_char(*str++);
    }
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
    serial_write(chr);
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
    serial_write(chr);
    serial_write("\n");
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

char* BasicConsole::Input() {
    inputActive = true;
    finished = false;
    input = strdup("");
    while (!finished);
    inputActive = false;
    return input;
}
void BasicConsole::FinishInput() {
    finished = true;
}
void BasicConsole::SubmitText(char* text) {
    if (inputActive) {
        char* oldInp = input;

        input = (char*)malloc(strlen(oldInp) + strlen(text) + 1);
        if (oldInp) {
            memcpy(input, oldInp, strlen(oldInp));
            free(oldInp);
        }
        memcpy(input + strlen(oldInp), text, strlen(text));
        input[strlen(oldInp) + strlen(text)] = 0;
    }
}
void BasicConsole::Backspace() {
    if (inputActive) {
        size_t len = strlen(input);
        char ol = input[len - 1];
        if (len == 0) return;
        input[len - 1] = '\0';
        CursorPosition.X -= 8;
        putChar(0, ol, CursorPosition.X, CursorPosition.Y);
    }
}