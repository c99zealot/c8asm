#ifndef EXIT_CODES_H_INCLUDED
        #define EXIT_CODES_H_INCLUDED 1

        typedef enum {
                SUCCESS,
                FAILURE,

                ERR_TOO_FEW_ARGS, 
                ERR_FOPEN_FAIL,
                ERR_INT_TOO_LARGE,
                ERR_MALLOC_FAIL,
                ERR_EMPTY_FILE
        } ExitCode;
#endif
