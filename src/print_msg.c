#include <stdio.h>
#include <stdarg.h>

#include "print_msg.h"
#include "ansicodes.h"

//
// print_msg - prints formatted error/warning messages
//
void print_msg(MsgType msgtype, int line, int col, char *fmt, ...) {
        char errmsg[128];

        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(errmsg, 127, fmt, arglist);
        va_end(arglist);

        char *infile_buffer_alias = infile_buffer;

        int lines_found = 0;
        while (lines_found < line - 1 && infile_buffer_alias <= infile_buffer_ptr)
                if (*infile_buffer_alias++ == '\n')
                        ++lines_found;

        if (msgtype == ERROR)
                fprintf(stderr, BOLD("%s:%d:%d: " RED("error")) BOLD(": %s") "\n", infile_name, line, col, errmsg);
        else
                fprintf(stderr, BOLD("%s:%d:%d: " MAGENTA("warning")) BOLD(": %s") "\n", infile_name, line, col, errmsg);

        while (*infile_buffer_alias != '\n')
                putchar(*infile_buffer_alias++);

        putchar('\n');
        while (--col > 0)
                putchar(' ');
        fputs(BOLD(YELLOW("^")) "\n\n", stderr);
}
