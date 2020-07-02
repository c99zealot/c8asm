#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "exitcodes.h"
#include "ansicodes.h"
#include "lexer.h"
#include "parser.h"
#include "print_msg.h"
#include "panic.h"

#define FMT_ERRMSG(msg) (BOLD(RED("error")) ": " msg)

#define INT_TOO_LARGE_255   "integer constant is too large for this instruction (>255)"
#define INT_TOO_LARGE_15    "integer constant is too large for this instruction (>15)"
#define ADDR_LT_512_WARNING "most CHIP8 implementations use addresses below 0x200 for sprite " \
                            "storage, jumping to any of them probably isn't a good idea"

enum {C8_INSTR_SIZE = 2, C8_CODE_START_ADDR = 0x200};

// used throughout to access a byte in the output stream, this makes writes endian-agnostic
uint8_t *byte_ptr;

// defined in parser.h
extern inline Token next_tkn(void);

//
// push_label_ref - pushes a LabelRef to the label reference table, grows table if needed
//
static inline void push_label_ref(Token *label) {
        ptrdiff_t label_refs_pushed = label_refs_ptr - label_refs;

        if (label_refs_pushed >= LABEL_BUFFER_INIT_LEN) {
                if (!(label_refs = realloc(label_refs, label_refs_pushed * sizeof(LabelRef) + sizeof(LabelRef)))) {
                        fputs(FMT_ERRMSG("failed to resize buffer for label reference table\n"), stderr);
                        panic(ERR_MALLOC_FAIL);
                }
                label_refs_ptr = label_refs + label_refs_pushed;
        }

        *label_refs_ptr++ = (LabelRef){
                .label_text = label->value.text,
                .output_pos = outfile_buffer_ptr,
                .line = label->line,
                .col = label->col
        };
}

//
// push_label_def - pushes a LabelDef to the label definition table, grows table if needed
//
static inline void push_label_def(Token *label) {
        ptrdiff_t label_defs_pushed = label_defs_ptr - label_defs;

        if (label_defs_pushed >= LABEL_BUFFER_INIT_LEN) {
                if (!(label_defs = realloc(label_defs, label_defs_pushed * sizeof(LabelDef) + sizeof(LabelDef)))) {
                        fputs(FMT_ERRMSG("failed to resize buffer for label definition table\n"), stderr);
                        panic(ERR_MALLOC_FAIL);
                }
                label_defs_ptr = label_defs + label_defs_pushed;
        }

        *label_defs_ptr++ = (LabelDef){
                .label_text = label->value.text,
                .c8_addr = C8_CODE_START_ADDR + ((outfile_buffer_ptr - outfile_buffer) * C8_INSTR_SIZE),
                .line = label->line,
                .col = label->col
        };
}

//
// parse_jmp - parses a jmp instruction, writes output to the output stream
//
static inline void parse_jmp(void) {
        next_tkn();
        if (current_tkn.type == NAME_LBLREF) {
                byte_ptr[0] = 0x10;
                push_label_ref(&current_tkn);
        } else if (current_tkn.type == CONST_INT) {
                if (current_tkn.value.num < 0x200) {
                        print_msg(WARNING, current_tkn.line, current_tkn.col, ADDR_LT_512_WARNING);
                        ++warning_count;
                }
                byte_ptr[0] = 0x10 | ((current_tkn.value.num & 0xF00) >> 8);
                byte_ptr[1] = current_tkn.value.num & 0xFF;
        } else {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected an integer constant or a label reference");
                ++error_count;
        }
}

//
// parse_vjmp - parses a vjmp instruction, writes output to the output stream
//
static inline void parse_vjmp(void) {
        next_tkn();
        if (current_tkn.type == NAME_LBLREF) {
                push_label_ref(&current_tkn);
                byte_ptr[0] = 0xB0;
        } else if (current_tkn.type == CONST_INT) {
                if (current_tkn.value.num < 0x200) {
                        print_msg(WARNING, current_tkn.line, current_tkn.col, ADDR_LT_512_WARNING);
                        ++warning_count;
                }
                byte_ptr[0] = 0xB0 | ((current_tkn.value.num & 0xF00) >> 8);
                byte_ptr[1] = current_tkn.value.num & 0xFF;
        } else {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected an integer constant or a label reference");
                ++error_count;
        }
}

//
// parse_call - parses a call instruction, writes output to the output stream
//
static inline void parse_call(void) {
        next_tkn();
        if (current_tkn.type == NAME_LBLREF) {
                byte_ptr[0] = 0x20;
                push_label_ref(&current_tkn);
        } else if (current_tkn.type == CONST_INT) {
                if (current_tkn.value.num < 0x200) {
                        print_msg(WARNING, current_tkn.line, current_tkn.col, ADDR_LT_512_WARNING);
                        ++warning_count;
                }
                byte_ptr[0] = 0x20 | ((current_tkn.value.num & 0xF00) >> 8);
                byte_ptr[1] = current_tkn.value.num & 0xFF;
        } else {
                print_msg(ERROR, current_tkn.line, current_tkn.col,
                        "expected an integer constant or a label reference");
                ++error_count;
        }
}

//
// parse_sne - parses an sne instruction, writes output to the output stream
//
static inline void parse_sne(void) {
        int xreg;

        if (next_tkn().type != NAME_REG) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
                ++error_count;
        }

        xreg = current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        next_tkn();
        if (current_tkn.type == CONST_INT) {
                if (current_tkn.value.num > 0xFF)
                        print_msg(ERROR, current_tkn.line, current_tkn.col, INT_TOO_LARGE_255);

                byte_ptr[0] = 0x40 | xreg;
                byte_ptr[1] = current_tkn.value.num;
        } else if (current_tkn.type == NAME_REG) {
                byte_ptr[0] = 0x90 | xreg;
                byte_ptr[1] = current_tkn.value.num << 4;
        } else {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register or an integer constant");
                ++error_count;
        }
}

//
// parse_se - parses an se instruction, writes output to the output stream
//
static inline void parse_se(void) {
        int xreg;

        if (next_tkn().type != NAME_REG) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
                ++error_count;
        }

        xreg = current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        next_tkn();
        if (current_tkn.type == CONST_INT) {
                if (current_tkn.value.num > 0xFF)
                        print_msg(ERROR, current_tkn.line, current_tkn.col, INT_TOO_LARGE_255);

                byte_ptr[0] = 0x30 | xreg;
                byte_ptr[1] = current_tkn.value.num;
        } else if (current_tkn.type == NAME_REG) {
                byte_ptr[0] = 0x50 | xreg;
                byte_ptr[1] = current_tkn.value.num << 4;
        } else {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register or an integer constant");
                ++error_count;
        }
}

//
// parse_mov - parses a mov instruction, writes output to the output stream
//
static void parse_mov(void) {
        int xreg;

        switch (next_tkn().type) {
                case NAME_REG:
                        xreg = current_tkn.value.num;

                        if (next_tkn().type != SYM_COMMA)
                                goto comma_not_found;
                        
                        switch (next_tkn().type) {
                                case CONST_INT:
                                        if (current_tkn.value.num > 0xFF) {
                                                print_msg(ERROR, current_tkn.line, current_tkn.col, INT_TOO_LARGE_255);
                                                ++error_count;
                                                return;
                                        }

                                        byte_ptr[0] = 0x60 | xreg;
                                        byte_ptr[1] = current_tkn.value.num;

                                        break;
                                case NAME_REG:
                                        byte_ptr[0] = 0x80 | xreg;
                                        byte_ptr[1] = current_tkn.value.num << 4;

                                        break;
                                case NAME_DT:
                                        byte_ptr[0] = 0xF0 | xreg;
                                        byte_ptr[1] = 0x07;

                                        break;

                                default:
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected an integer constant or a name");
                                        ++error_count;
                                        return;
                        }

                        break;
                case NAME_ST:
                        if (next_tkn().type != SYM_COMMA)
                                goto comma_not_found;
                        
                        if (next_tkn().type != NAME_REG)
                                goto reg_not_found;

                        byte_ptr[0] = 0xF0 | current_tkn.value.num;
                        byte_ptr[1] = 0x18;

                        break;                        
                case NAME_DT:
                        if (next_tkn().type != SYM_COMMA)
                                goto comma_not_found;
                        
                        if (next_tkn().type != NAME_REG)
                                goto reg_not_found;

                        byte_ptr[0] = 0xF0 | current_tkn.value.num;
                        byte_ptr[1] = 0x15;

                        break;                        
                case NAME_I:
                        if (next_tkn().type != SYM_COMMA)
                                goto comma_not_found;

                        if (next_tkn().type != CONST_INT) {
                                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected an integer constant");
                                ++error_count;
                                return;
                        }

                        byte_ptr[0] = 0xA0 | ((current_tkn.value.num & 0xF00) >> 8);
                        byte_ptr[1] = current_tkn.value.num & 0xFF;

                        break;                        

                default:
                        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a name");
                        ++error_count;
                        return;
        }

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register");
        ++error_count;
        return;
comma_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
        ++error_count;
}

//
// parse_or - parses an or instruction, writes output to the output stream
//
static inline void parse_or(void) {
        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[0] = 0x80 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[1] = 0x01 | (current_tkn.value.num << 4);

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
        ++error_count;
}

//
// parse_and - parses an and instruction, writes output to the output stream
//
static inline void parse_and(void) {
        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[0] = 0x80 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[1] = 0x02 | (current_tkn.value.num << 4);

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
        ++error_count;
}

//
// parse_xor - parses an xor instruction, writes output to the output stream
//
static inline void parse_xor(void) {
        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[0] = 0x80 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[1] = 0x03 | (current_tkn.value.num << 4);

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
        ++error_count;
}

//
// parse_sub - parses a sub instruction, writes output to the output stream
//
static inline void parse_sub(void) {
        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[0] = 0x80 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[1] = 0x05 | (current_tkn.value.num << 4);

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
        ++error_count;
}

//
// parse_subn - parses a subn instruction, writes output to the output stream
//
static inline void parse_subn(void) {
        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[0] = 0x80 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[1] = 0x07 | (current_tkn.value.num << 4);

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
        ++error_count;
}

//
// parse_rnd - parses a rnd instruction, writes output to the output stream
//
static inline void parse_rnd(void) {
        if (next_tkn().type != NAME_REG) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
                ++error_count;
        }

        byte_ptr[0] = 0xC0 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
                ++error_count;
        }

        if (next_tkn().type != CONST_INT) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected an integer constant");
                ++error_count;
        }

        if (current_tkn.value.num > 0xFF) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, INT_TOO_LARGE_255);
                ++error_count;
        }

        byte_ptr[1] = current_tkn.value.num;
}

//
// parse_drw - parses a drw instruction, writes output to the output stream
//
static void parse_drw(void) {
        if (next_tkn().type != NAME_REG)
                goto reg_not_found;
        
        byte_ptr[0] = 0xD0 | current_tkn.value.num;

        if (next_tkn().type != SYM_COMMA)
                goto comma_not_found;

        if (next_tkn().type != NAME_REG)
                goto reg_not_found;

        byte_ptr[1] = current_tkn.value.num << 4;

        if (next_tkn().type != SYM_COMMA)
                goto comma_not_found;

        if (next_tkn().type != CONST_INT) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected an integer constant");
                ++error_count;
        }

        if (current_tkn.value.num > 0xF) {
                print_msg(ERROR, current_tkn.line, current_tkn.col, INT_TOO_LARGE_15);
                ++error_count;
        }

        byte_ptr[1] |= current_tkn.value.num;

        return;

reg_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
        ++error_count;
        return;
comma_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
        ++error_count;
}

//
// parse_add - parses an add instruction, writes output to the output stream
//
static void parse_add(void) {
        int xreg;

        next_tkn();
        if (current_tkn.type == NAME_REG) {
                xreg = current_tkn.value.num;

                if (next_tkn().type != SYM_COMMA)
                        goto comma_not_found;

                next_tkn();
                if (current_tkn.type == NAME_REG) {
                        byte_ptr[0] = 0x80 | xreg;
                        byte_ptr[1] = 0x04 | (current_tkn.value.num << 4);
                } else if (current_tkn.type == CONST_INT) {
                        if (current_tkn.value.num > 0xFF) {
                                print_msg(ERROR, current_tkn.line, current_tkn.col, INT_TOO_LARGE_255);
                                ++error_count;
                        }

                        byte_ptr[0] = 0x70 | xreg;
                        byte_ptr[1] = current_tkn.value.num;
                } else {
                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                "expected a register or an integer constant");
                        ++error_count;
                }
        } else if (current_tkn.type == NAME_I) {
                if (next_tkn().type != SYM_COMMA)
                        goto comma_not_found;

                if (next_tkn().type != NAME_REG) {
                        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register name");
                        ++error_count;
                }

                byte_ptr[0] = 0xF0 | current_tkn.value.num;
                byte_ptr[1] = 0x1E;
        } else {
                print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a register or a name");
                ++error_count;
        }

        return;

comma_not_found:
        print_msg(ERROR, current_tkn.line, current_tkn.col, "expected a comma");
        ++error_count;
}

//
// parse_tkn_stream - examines the token stream and performs a procedure accordingly
//
void parse_tkn_stream(void) {
        while (next_tkn().type != STREAM_END) {
                byte_ptr = (uint8_t*)outfile_buffer_ptr;

                switch (current_tkn.type) {
                        case INSTR_SE:   parse_se();   break;
                        case INSTR_OR:   parse_or();   break;
                        case INSTR_JMP:  parse_jmp();  break;
                        case INSTR_SNE:  parse_sne();  break;
                        case INSTR_MOV:  parse_mov();  break;
                        case INSTR_AND:  parse_and();  break;
                        case INSTR_XOR:  parse_xor();  break;
                        case INSTR_ADD:  parse_add();  break;
                        case INSTR_SUB:  parse_sub();  break;
                        case INSTR_RND:  parse_rnd();  break;
                        case INSTR_DRW:  parse_drw();  break;
                        case INSTR_SUBN: parse_subn(); break;
                        case INSTR_VJMP: parse_vjmp(); break;
                        case INSTR_CALL: parse_call(); break;

                        case INSTR_SHL: 
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0x80 | current_tkn.value.num;
                                byte_ptr[1] = 0x0E;

                                break;
                        case INSTR_SHR:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0x80 | current_tkn.value.num;
                                byte_ptr[1] = 0x06;

                                break;
                        case INSTR_WKP:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xF0 | current_tkn.value.num;
                                byte_ptr[1] = 0x0A;

                                break;
                        case INSTR_SKD:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xE0 | current_tkn.value.num;
                                byte_ptr[1] = 0x9E;

                                break;
                        case INSTR_SKU:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xE0 | current_tkn.value.num;
                                byte_ptr[1] = 0xA1;

                                break;
                        case INSTR_LDF:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xF0 | current_tkn.value.num;
                                byte_ptr[1] = 0x29;

                                break;
                        case INSTR_BCD:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xF0 | current_tkn.value.num;
                                byte_ptr[1] = 0x33;

                                break;
                        case INSTR_LOD:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xF0 | current_tkn.value.num;
                                byte_ptr[1] = 0x65;

                                break;
                        case INSTR_STR:
                                if (next_tkn().type != NAME_REG) {
                                        print_msg(ERROR, current_tkn.line, current_tkn.col,
                                                "expected a register name");
                                        ++error_count;
                                }

                                byte_ptr[0] = 0xF0 | current_tkn.value.num;
                                byte_ptr[1] = 0x55;

                                break;
                        case INSTR_CLS:
                                byte_ptr[0] = 0x00;
                                byte_ptr[1] = 0xE0;

                                break;
                        case INSTR_RET:
                                byte_ptr[0] = 0x00;
                                byte_ptr[1] = 0xEE;

                                break;
                        case NAME_LBLDEF:
                                push_label_def(&current_tkn);
                                continue;

                        default:
                                print_msg(ERROR, current_tkn.line, current_tkn.col,
                                        "expected a label definition or a mnemonic");
                                ++error_count;
                }

                ++outfile_buffer_ptr;
        }
}
