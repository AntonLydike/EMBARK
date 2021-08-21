#include "io.h"

#ifdef TEXT_IO_ADDR

#ifndef TEXT_IO_BUFLEN
#error "When defining TEXT_IO_ADDR, please also provide TEXT_IO_BUFLEN, otherwise textIO won't work!"
#endif

void dbgln(char* text, int len)
{
    while (len > TEXT_IO_BUFLEN) {
        dbgln(text, TEXT_IO_BUFLEN);
        text += TEXT_IO_BUFLEN;
        len -= TEXT_IO_BUFLEN;
    }

    char* ioaddr = (char*) TEXT_IO_ADDR + 4;

    for (int i = 0; i < len; i++) {
        if (*text == 0)
            break;
        *ioaddr++ = *text++;
    }

    if (len < TEXT_IO_BUFLEN)
        *ioaddr = '\n';

    // write a 1 to the start of the textIO to signal a buffer flush
    *((char*) TEXT_IO_ADDR) = 1;
}


/* alphabet for itoa */
char alpha[16] = "0123456789abcdef";
char* itoa (int value, char* str, int base)
{
    if (base > 16 || base < 2) {
        *str++ = '?';
        return str;
    }

    if (value < 0) {
        *str++ = '-';
        value *= -1;
    }

    int num;
    do {
        num = value % base;
        value = value / base;
        *str++ = alpha[num];
    }
    while (value > 0);

    return str;
}

#else

/* if no textIO module loaded, dbgln is a noop :( */
void dbgln(char* text, int len){}
char* itoa (int value, char* str, int base) {
    return str;
}

#endif

