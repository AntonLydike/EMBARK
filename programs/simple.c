#define TEXT_IO_ADDR 0xff0000
#define TEXT_IO_BUFLEN 64

void dbgln(char*, int);
char* itoa(int value, char* str, int base);


char text[48] = "the number is       ";

int main()
{
    dbgln("main", 4);
    int a = 144;

    while (1) {
        for (int i = 0; i < 10000; i++) {
            for (int j = 0; j < 10000; j++) {
                a ^= (((a << 16) ^ a) & i) << 4;
                a ^= ((a & (j << 4)) >> 3) ^ (i * j);
            }
            itoa(a, &text[14], 16);
            dbgln(text, 32);
        }
        __asm__ ("ebreak");
    }

    return a;
}

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

char alpha[16] = "0123456789abcdef";
char* itoa(int value, char* str, int base)
{
    if (base > 16 || base < 2) {
        *str++ = '?';
        return str;
    }

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

    value = num;
    do {
        num = value % base;
        value = value / base;
        *str++ = alpha[num];
        digits--;
    }while (digits > 0);

    return str;
}

void _start()
{
    __asm__ (
         ".option push\n"
         ".option norelax\n"
         "            la      gp, _gp\n"
         ".option pop\n"
    );

    dbgln("start", 5);
    int exit_code = main();

    dbgln("end", 3);

    __asm__ (
         "mv     a0, %0\n"
         "li     a7, 5\n"
         "ecall" :: "r"(exit_code)
    );
}
