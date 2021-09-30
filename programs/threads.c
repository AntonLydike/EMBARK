#include "threads.h"

int main()
{
    dbgln("main", 4);

    int arg1 = 30;
    int arg2 = 50;
    struct optional_int t1 = spawn(thread, &arg1);

    if (has_error(t1)) {
        __asm__ ("ebreak");
        return 1;
    }
    struct optional_int t2 = spawn(thread, &arg2);

    if (has_error(t2)) {
        __asm__ ("ebreak");
        return 2;
    }

    t1 = join(t1.value, 1000);
    if (has_error(t1)) {
        return -1;
    }
    t2 = join(t2.value, 1000);
    if (has_error(t2)) {
        return -2;
    }

    dbgln("children exited!", 16);

    return 0;
}

int thread(void* args)
{
    // read value
    int arg = *((int*) args);

    //char buff[64] = "sleeping for ";
    //char* end = itoa(arg, &buff[13], 10);

    // print given number
    //dbgln(buff, 13);

    sleep(arg);

    // return value as exit code
    return arg;
}

/*
 * Additional functions
 */

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
    __asm__ __volatile__ (
          ".option push\n"
          ".option norelax\n"
          "            la      gp, _gp\n"
          ".option pop\n"
    );
    main();
    __asm__ __volatile__ (
         "li     a7, 5\n"
         "ecall\n"
    );
    __builtin_unreachable();
}
