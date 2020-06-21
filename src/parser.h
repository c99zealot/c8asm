#ifndef PARSER_H_INCLUDED
        #define PARSER_H_INCLUDED 1

        #include <stdint.h>

        #include "lexer.h"

        enum {LABEL_BUFFER_INIT_LEN = 32};

        typedef uint16_t Instruction;

        typedef struct {
                char *label_text;
                uint16_t c8_addr;

                uint16_t line, col;
        } LabelDef;

        typedef struct {
                char *label_text;
                Instruction *output_pos;

                uint16_t line, col;
        } LabelRef;

        extern Token current_tkn;
        extern Token *tkn_stream, *tkn_stream_ptr;

        extern Instruction *outfile_buffer, *outfile_buffer_ptr;

        extern LabelDef *label_defs, *label_defs_ptr;
        extern LabelRef *label_refs, *label_refs_ptr;

        extern int error_count, warning_count;

        extern void parse_tkn_stream(void);
        extern void parser_error(char *errmsg);

        //
        // next_tkn - get the next token from the token stream
        //
        inline Token next_tkn(void) {
                return (current_tkn = *tkn_stream_ptr++);
        }
#endif
