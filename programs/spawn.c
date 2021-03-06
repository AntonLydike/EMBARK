#define TEXT_IO_ADDR 0xff0000
#define TEXT_IO_BUFLEN 64

void dbgln(char*, int);
char* itoa(int value, char* str, int base);

int thread(void* args);

int main()
{
    dbgln("main", 4);

    int arg = 144;

    // manually invoke syscall to spawn thread
    // syscall code (a7): 1
    // args: target function, arg ptr
    __asm__ (
         "mv a0, %0\n"
         "mv a1, %1\n"
         "li a7, 1\n"
         "ecall" :: "r"(thread), "r"(&arg)
    );

    // wait for child thread to modify value
    while (arg == 144) {
    }

    dbgln("child exited!", 13);

    return 0;
}

int thread(void* args)
{
    // read value
    int arg = *((int*) args);
    char buff[32] = "the magic number is: ";
    char* end = itoa(arg, &buff[21], 10);

    // print given number
    dbgln(buff, (int) (end - buff));

    // set value to free parent thread
    *((int*) args) = 0;

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
    __asm__ __volatile__ (
         "mv     a0, %0\n"
         "li     a7, 5\n"
         "ecall\n" :: "r"(main())
    );

}
