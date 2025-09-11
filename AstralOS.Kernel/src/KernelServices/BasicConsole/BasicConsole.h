#pragma once
#include <stddef.h>
#include <cstdint>
#include "font.h"

/*
* Originally Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-3-Graphics-Output-Protocol/kernel/src/kernel.c
*/
struct FrameBuffer {
	void* BaseAddress;
	size_t BufferSize;
	unsigned int Width;
	unsigned int Height;
	unsigned int PixelsPerScanLine;
};

typedef struct {
	unsigned int X;
	unsigned int Y;
} Point;

class BasicConsole {
public:
	BasicConsole(FrameBuffer fb);

	void Print(const char* str, unsigned int colour = 0xffffffff);
	void Println(const char* str, unsigned int colour = 0xffffffff);
	Point CursorPosition;
public:
	void putChar(unsigned int colour, char chr, unsigned int xOff, unsigned int yOff);
	void ClearLines(unsigned int lineCount);

	FrameBuffer pFramebuffer;
};

