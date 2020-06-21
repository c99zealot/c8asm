#ifndef SHOW_ERR_H_INCLUDED
        #define SHOW_ERR_H_INCLUDED 1

        typedef enum {
                ERROR,
                WARNING
        } MsgType;

        extern char *infile_name, *infile_buffer, *infile_buffer_ptr;

        extern void print_msg(MsgType msgtype, int line, int col, char *fmt, ...);
#endif
