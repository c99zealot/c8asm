#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "exitcodes.h"
#include "ansicodes.h"
#include "lexer.h"
#include "print_msg.h"
#include "panic.h"

static const char *keywords[] = {
        "cls",
        "jmp",
        "vjmp",
        "call",
        "ret",
        "sne",
        "se",
        "mov",
        "or",
        "and",
        "xor",
        "add",
        "sub",
        "subn",
        "shr",
        "shl",
        "rnd",
        "drw",
        "wkp",
        "skd",
        "sku",
        "ldf",
        "bcd",
        "lod",
        "str",
        "stimer",
        "dtimer"
};

//
// next_char - sets current_char to and returns the next char from the character stream, sets line_count and col_count
//
int next_char(void) {
        if (infile_buffer_ptr - infile_buffer == infile_len)
                return (current_char = EOF);

        current_char = *infile_buffer_ptr++;

        if (current_char == '\n') {
                ++line_count;
                col_count = 0;
        } else
                ++col_count;

        return current_char;
}

//
// lex_name - lexes a name (NAME_*) and returns it as a token
//
Token lex_name(void) {
        int i;
        int lexeme_startline = line_count;
        int lexeme_startcol = col_count;

        // lex NAME_REG
        if ((current_char == 'V' || current_char == 'v') && isxdigit(infile_buffer_ptr[0])) {
                int reg;

                next_char();
                if (ISLOWER(current_char))
                        reg = current_char - 87;
                else if (ISUPPER(current_char))
                        reg = current_char - 55;
                else                               
                        reg = current_char - '0';

                next_char();
                return (Token){
                        .type = NAME_REG,
                        .line = lexeme_startline,
                        .col  = lexeme_startcol,
                        .value.num = reg
                };
        }

        // assume name is a label definition
        char *name;
        if (!(name = calloc(32, 1)))
                goto alloc_fail;

        for (i = 0; i < 32 && ISLABELCHAR(current_char); ++i) {
                name[i] = current_char;
                next_char();
        }

        if (i == 32 && (isalnum(current_char) || current_char == '_')) {
                print_msg(ERROR, lexeme_startline, lexeme_startcol, "label name is too long (>32 characters)");
                ++error_count;
        }

        if (!(name = realloc(name, i + 1)))
                goto alloc_fail;
        name[i] = '\0';

        if (current_char == SYM_COLON) {
                next_char();
                return (Token){
                        .type = NAME_LBLDEF,
                        .line = lexeme_startline,
                        .col  = lexeme_startcol,
                        .value.text = name
                };
        }

        // no colon so not a label definition, search for name in keywords list
        for (i = 0; i < NAME_REG && strcmp(name, keywords[i]); ++i);

        // if i == NAME_REG then name was not found in keywords list, assume it's label reference
        if (i == NAME_REG)
                i = NAME_LBLREF;
        else {
                free(name);
                name = NULL;
        }

        return (Token){
                .type = i,
                .line = lexeme_startline,
                .col  = lexeme_startcol,
                .value.text = name
        };

alloc_fail:
        print_msg(ERROR, lexeme_startline, lexeme_startcol, "failed to allocate buffer for name");
        ++error_count;
}

//
// lex_int - lexes an integer constant and returns it as a token
//
Token lex_int(void) {
        int integer_value = 0;
        int decimal_value = current_char - '0';
        int lexeme_startline = line_count;
        int lexeme_startcol = col_count;

        if (current_char == '0' && isalpha(next_char())) {
                // parse 0x 0b 0o
                switch (current_char) {
                        case 'X':        // FALLTHROUGH
                        case 'x':
                                next_char();
                                if (!isxdigit(current_char)) {
                                        print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                                "invalid digits supplied to 0x integer constant");
                                        ++error_count;
                                }

                                while (!(isspace(current_char) || current_char == EOF)) {
                                        if (isxdigit(current_char)) {
                                                integer_value <<= 4;
                                                if (ISUPPER(current_char))
                                                        integer_value += (current_char - 55);
                                                else if (ISLOWER(current_char))
                                                        integer_value += (current_char - 87);
                                                else
                                                        integer_value += (current_char - '0');

                                                next_char();
                                        } else {
                                                print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                                        "invalid digits supplied to 0x integer constant");
                                                ++error_count;
                                        }
                                }

                                if (integer_value > 4095)
                                        goto int_too_large;

                                break;
                        case 'B':        // FALLTHROUGH
                        case 'b':
                                next_char();
                                if (!ISBIN(current_char)) {
                                        print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                                "invalid digits supplied to 0b integer constant");
                                        ++error_count;
                                }

                                while (!(isspace(current_char) || current_char == EOF)) {
                                        if (ISBIN(current_char)) {
                                                integer_value <<= 1;
                                                integer_value += (current_char - '0');
                                                next_char();
                                        } else {
                                                print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                                        "invalid digits supplied to 0b integer constant");
                                                ++error_count;
                                        }
                                }

                                if (integer_value > 4095)
                                        goto int_too_large;

                                break;
                        case 'O':        // FALLTHROUGH
                        case 'o':
                                next_char();
                                if (!ISOCT(current_char)) {
                                        print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                                "invalid digits supplied to 0o integer constant");
                                        ++error_count;
                                }

                                while (!(isspace(current_char) || current_char == EOF)) {
                                        if (ISOCT(current_char)) {
                                                integer_value <<= 3;
                                                integer_value += (current_char - '0');
                                                next_char();
                                        } else {
                                                print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                                        "invalid digits supplied to 0o integer constant");
                                                ++error_count;
                                        }
                                }

                                if (integer_value > 4095)
                                        goto int_too_large;

                                break;

                        default:
                                print_msg(ERROR, lexeme_startline, lexeme_startcol, 
                                        "expecting 0x, 0b or 0o prefixed integer constant");
                                ++error_count;
                }
        } else {
                // parse decimal
                next_char();
                integer_value = decimal_value;
                while (!(isspace(current_char) || current_char == EOF))
                        if (ISDEC(current_char)) {
                                integer_value *= 10;
                                integer_value += (current_char - '0');
                                next_char();
                        } else {
                                print_msg(ERROR, lexeme_startline, lexeme_startcol,
                                        "invalid digits in decimal integer constant");
                                ++error_count;
                        }

                if (integer_value > 4095)
                        goto int_too_large;
        }

        return (Token){
                .type = CONST_INT,
                .line = lexeme_startline,
                .col  = lexeme_startcol,
                .value.num = integer_value
        };

int_too_large:
        print_msg(ERROR, lexeme_startline, lexeme_startcol, "integer constant is too large (>4095)");
        ++error_count;
}
