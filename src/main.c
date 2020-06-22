#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "exitcodes.h"
#include "ansicodes.h"
#include "lexer.h"
#include "parser.h"
#include "print_msg.h"
#include "panic.h"

#define FMT_ERRMSG(msg) (BOLD(RED("error")) ": " msg)

FILE *infile, *outfile;

long infile_len;
char *outfile_name, *infile_name, *infile_buffer, *infile_buffer_ptr;
int current_char;
uint16_t col_count, line_count = 1;

Token current_tkn;
Token *tkn_stream, *tkn_stream_ptr;

Instruction *outfile_buffer, *outfile_buffer_ptr;

LabelRef *label_refs, *label_refs_ptr;
LabelDef *label_defs, *label_defs_ptr;

ptrdiff_t label_defs_len, label_refs_len;

int error_count, warning_count;

// defined in panic.h
extern inline void panic(ExitCode err);

int main(int argc, char **argv) {
        if (argc < 2) {
                fprintf(stderr, FMT_ERRMSG("too few arguments\nusage: %s <chip8 asm source file> <output file name>\n"),
                        argv[0]);
                return ERR_TOO_FEW_ARGS;
        }

        infile_name = argv[1];

        if (!(infile = fopen(argv[1], "rb"))) {
                fprintf(stderr, FMT_ERRMSG("failed to open file `%s`\n"), infile_name);
                return ERR_FOPEN_FAIL;
        }

        fseek(infile, 0, SEEK_END);
        infile_len = ftell(infile);
        rewind(infile);

        if (infile_len == 0) {
                fprintf(stderr, FMT_ERRMSG("input file `%s` is empty\n"), infile_name);
                panic(ERR_EMPTY_FILE);
        }

        if (!(infile_buffer_ptr = infile_buffer = malloc(infile_len))) {
                fprintf(stderr, FMT_ERRMSG("failed to allocate memory for source file `%s`\n"), infile_name);
                panic(ERR_MALLOC_FAIL);
        }

        if (!fread(infile_buffer, 1, infile_len, infile)) {
                fprintf(stderr, FMT_ERRMSG("failed to load source file `%s`\n"), infile_name);
                panic(ERR_MALLOC_FAIL);
        }
        fclose(infile);
        infile = NULL;

        if (!(tkn_stream_ptr = tkn_stream = malloc(sizeof(Token) * infile_len))) {
                fputs(FMT_ERRMSG("failed to allocate buffer for token stream\n"), stderr);
                panic(ERR_MALLOC_FAIL);
        }
        
        // start lexing
        next_char();
        while (current_char != EOF) {
                if (ISDEC(current_char))
                        *tkn_stream_ptr++ = lex_int();
                else if (current_char == SYM_COMMA || current_char == NAME_I) {
                        *tkn_stream_ptr++ = (Token){
                                .line = line_count,
                                .col = col_count,
                                .type = current_char
                        };
                        next_char();
                } else if (isalpha(current_char) || current_char == '_')
                        *tkn_stream_ptr++ = lex_name();
                else
                        next_char();
        }
        (tkn_stream_ptr++)->type = STREAM_END; // finish lexing

        ptrdiff_t tkns_generated = tkn_stream_ptr - tkn_stream;
        if (!(tkn_stream = realloc(tkn_stream, sizeof(Token) * tkns_generated))) {
                fputs(FMT_ERRMSG("failed to resize the token stream\n"), stderr);
                panic(ERR_MALLOC_FAIL);
        }
        tkn_stream_ptr = tkn_stream + tkns_generated;

        if (!(outfile_buffer_ptr = outfile_buffer = malloc(tkns_generated * sizeof(Instruction)))) {
                fputs(FMT_ERRMSG("failed to allocate buffer for output\n"), stderr);
                panic(ERR_MALLOC_FAIL);
        }

        if (!((label_defs_ptr = label_defs = malloc(sizeof(LabelDef) * LABEL_BUFFER_INIT_LEN)) &&
                        (label_refs_ptr = label_refs = malloc(sizeof(LabelRef) * LABEL_BUFFER_INIT_LEN)))) {
                fputs(FMT_ERRMSG("failed to allocate buffer for label table(s)\n"), stderr);
                panic(ERR_MALLOC_FAIL);
        }

        // start parsing
        tkn_stream_ptr = tkn_stream;
        parse_tkn_stream(); // finish parsing

        // check that label definitions are unique
        label_defs_len = label_defs_ptr - label_defs;
        for (int i = 0; i < label_defs_len; ++i)
                for (int j = i + 1; j < label_defs_len; ++j)
                        if (!strcmp(label_defs[i].label_text, label_defs[j].label_text)) {
                                print_msg(ERROR, label_defs[j].line, label_defs[j].col,
                                        "multiple definition of label `%s`", label_defs[j].label_text);
                                ++error_count;
                        }

        // resolve label references
        uint8_t *instr_ptr;
        _Bool found_ref;

        label_refs_len = label_refs_ptr - label_refs;
        for (int i = 0; i < label_refs_len; ++i) {
                found_ref = 0;
                for (int j = 0; j < label_defs_len; ++j)
                        if (!strcmp(label_refs[i].label_text, label_defs[j].label_text)) {
                                found_ref = 1;

                                instr_ptr = (uint8_t*)label_refs[i].output_pos;

                                instr_ptr[0] |= ((label_defs[j].c8_addr & 0xF00) >> 8);
                                instr_ptr[1] = label_defs[j].c8_addr & 0x0FF;
                        }

                if (!found_ref) {
                        print_msg(ERROR, label_refs[i].line, label_refs[i].col, "undefined reference to label `%s`",
                                label_refs[i].label_text);
                        ++error_count;
                }
        }

        if (warning_count > 0)
                fprintf(stderr, "%d warning(s) generated\n", warning_count);
        if (error_count > 0) {
                fprintf(stderr, "%d error(s) generated\n", error_count);
                panic(FAILURE);
        }

        outfile_name = (argc > 2) ? argv[2] : "out.ch8";
        if (!(outfile = fopen(outfile_name, "wb"))) {
                fprintf(stderr, FMT_ERRMSG("failed to open output file `%s` for writing\n"), outfile_name);
                panic(ERR_FOPEN_FAIL);
        }

        // write the assembled chip8 code to disk
        fwrite(outfile_buffer, sizeof(Instruction), outfile_buffer_ptr - outfile_buffer, outfile);

        // cleanup and exit
        for (int i = 0; i < label_defs_len; ++i)
                free(label_defs[i].label_text);

        for (int i = 0; i < label_refs_len; ++i)
                free(label_refs[i].label_text);

        free(tkn_stream);
        free(outfile_buffer);
        free(label_defs);
        free(label_refs);

        fclose(outfile);

        return SUCCESS;
}
