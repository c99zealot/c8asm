#ifndef ANSI_CODES_H_INCLUDED
        #define ANSI_CODES_H_INCLUDED 1

        #define RESET           "\x1b[0m"

        #define RED(text)       "\x1b[31m" text RESET
        #define BLUE(text)      "\x1b[34m" text RESET
        #define CYAN(text)      "\x1b[36m" text RESET
        #define GREEN(text)     "\x1b[32m" text RESET
        #define YELLOW(text)    "\x1b[33m" text RESET
        #define MAGENTA(text)   "\x1b[35m" text RESET
        #define BOLD(text)      "\x1b[1m"  text RESET
        #define UNDERLINE(text) "\x1b[4m"  text RESET
#endif 
