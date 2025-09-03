#include "utils.h"
#include "../KernelServices/Paging/MemoryAlloc/Heap.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }
    return (unsigned char)(*s1) - (unsigned char)(*s2);
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;

    for (size_t i = 0; i < n; ++i) {
        if (p1[i] != p2[i])
            return p1[i] - p2[i];
    }
    return 0;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];

    return dest;
}

char* strncpy(char* dest, const char* src, size_t maxLen) {
    if (maxLen == 0) return dest;

    size_t i = 0;
    while (i < maxLen - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
    return dest;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

char* strchr(const char* str, int ch) {
    while (*str) {
        if (*str == (char)ch) {
            return (char*)str;
        }
        ++str;
    }
    return nullptr;
}

char* strdup(const char* src) {
    if (!src) return nullptr;

    size_t len = 0;
    while (src[len] != '\0') len++;

    char* dst = (char*)malloc(len + 1);
    if (!dst) return nullptr;

    for (size_t i = 0; i <= len; i++) {
        dst[i] = src[i];
    }

    return dst;
}

char *
strtok_r(char *s, const char *delim, char **last)
{
	char *spanp;
	int c, sc;
	char *tok;
	if (s == NULL && (s = *last) == NULL)
		return (NULL);
	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}
	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;
	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

char *
strtok(char *s, const char *delim)
{
	static char *last;
	return strtok_r(s, delim, &last);
}

char *
strcat(char *s, const char *append)
{
	char *save = s;
	for (; *s; ++s);
	while ((*s++ = *append++) != '\0');
	return(save);
}

char *
strcpy(char *to, const char *from)
{
	char *save = to;
	for (; (*to = *from) != '\0'; ++from, ++to);
	return(save);
}

void* operator new(size_t size) { 
    return malloc(size); 
}

void operator delete(void* ptr) noexcept { 
    free(ptr); 
}

void* operator new[](size_t size) { 
    return malloc(size); 
}

void operator delete[](void* ptr) noexcept { 
    free(ptr); 
}