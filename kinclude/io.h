#ifndef H_IO
#define H_IO

// if we have a textIO module
#ifdef TEXT_IO_ADDR

#define DEBUGGING 1


#ifndef TEXT_IO_BUFLEN
#error "When defining TEXT_IO_ADDR, please also provide TEXT_IO_BUFLEN, otherwise textIO won't work!"
#endif

/* print a line to the debug textIO module */
void dbgln(char* text, int len);

/* alphabet for itoa */
char* itoa(int value, char* str, int base);

// if we don't have a textIO module for debugging
#else

#define DEBUGGING 0


// if we don't have textio, dbgln becomes an empty macro to save on cycles
#define dbgln(a, b)
// itoa just evaluates to the passes pointer
#define itoa(a, b, c) b

#endif
#endif
