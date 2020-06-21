#ifndef LEXER_H_INCLUDED
        #define LEXER_H_INCLUDED 1

        #include <stdlib.h>
        #include <stdint.h>
        #include <string.h>

        #define ISBIN(c)       ((c) == '0' || (c) == '1')
        #define ISOCT(c)       ((unsigned)(c) - '0' <= 7)
        #define ISDEC(c)       ((unsigned)(c) - '0' <= 9)
        #define ISUPPER(c)     ((unsigned)(c) - 'A' < 26)
        #define ISLOWER(c)     ((unsigned)(c) - 'a' < 26)
        #define ISLABELCHAR(c) (ISLOWER(c) || ISUPPER(c) || ISDEC(c) || (c) == '_')

        // these values correspond to indexes of their string representations in the keyword array in lexer.c
        // and this is used during lexing to find TokenType values for keywords in the character stream
        typedef enum {
                INSTR_CLS,
                INSTR_JMP,
                INSTR_VJMP,
                INSTR_CALL,
                INSTR_RET,
                INSTR_SNE,
                INSTR_SE,
                INSTR_MOV,
                INSTR_OR,
                INSTR_AND,
                INSTR_XOR,
                INSTR_ADD,
                INSTR_SUB,
                INSTR_SUBN,
                INSTR_SHR,
                INSTR_SHL,
                INSTR_RND,
                INSTR_DRW,
                INSTR_WKP,
                INSTR_SKD,
                INSTR_SKU,
                INSTR_LDF,
                INSTR_BCD,
                INSTR_LOD,
                INSTR_STR,

                NAME_ST,
                NAME_DT,
                NAME_REG,
                NAME_LBLREF,
                NAME_LBLDEF,
                NAME_I = 'I',

                SYM_COMMA = ',',
                SYM_COLON = ':',

                CONST_INT,

                STREAM_END
        } TokenType;

        typedef struct {
                TokenType type;
                uint16_t line, col;

                union {
                        int num;
                        char *text;
                } value;
        } Token;

        extern long infile_len;
        extern char *infile_name, *infile_buffer, *infile_buffer_ptr;
        extern int current_char;
        extern uint16_t col_count, line_count;

        extern int next_char(void);
        extern Token lex_name(void);
        extern Token lex_int(void);
#endif
