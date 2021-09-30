#include "io.h"
// this file should only be used for debugging purposes. Most functions here
// could easily corrupt kernel memory if used with untrusted input.
// ALWAYS check your buffer lengths!

#ifdef TEXT_IO_ADDR

// this function writes a string to the TEXT_IO buffer
// it adds a newline at the end and splits the passed string into smaller chunks
// if it is larger than TEXT_IO_BUFLEN.
void dbgln(char* text, int len)
{
    // if the passed text is longer than TEXT_IO_BUFLEN, print it in chunks
    while (len > TEXT_IO_BUFLEN) {
        dbgln(text, TEXT_IO_BUFLEN);
        text += TEXT_IO_BUFLEN;
        len -= TEXT_IO_BUFLEN;
    }

    // this is the address of the textIO
    char* ioaddr = (char*) TEXT_IO_ADDR + 4;

    // write message bytewise to buffer (this could be implemented faster)
    for (int i = 0; i < len; i++) {
        if (*text == 0)
            break;
        *ioaddr++ = *text++;
    }

    // add a newline
    if (len < TEXT_IO_BUFLEN)
        *ioaddr = '\n';

    // write a 1 to the start of the textIO to signal a buffer flush
    *((char*) TEXT_IO_ADDR) = 1;
}


/* alphabet for itoa */
char alpha[16] = "0123456789abcdef";
// convert int to str
char* itoa(int value, char* str, int base)
{
    // fail on unknown base
    if (base > 16 || base < 2) {
        *str++ = '?';
        return str;
    }

    // handle negative numbers
    if (value < 0) {
        *str++ = '-';
        value *= -1;
    }

    int digits = 0;
    int num = 0;

    // reverse number
    do {
        num = num * base;
        num += value % base;
        value = value / base;
        digits++;
    } while (value > 0);

    // write reversed number to the buffer
    value = num;
    do {
        num = value % base;
        value = value / base;
        *str++ = alpha[num];
        digits--;
    }while (digits > 0);

    return str;
}

#else

// this is included to prevent "error: ISO C forbids an empty translation unit [-Wpedantic]"
typedef int make_iso_compilers_happy;

#endif
