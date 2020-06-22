#ifndef PANIC_H_INCLUDED
        #define PANIC_H_INCLUDED 1
        
        #include <stdlib.h>
        #include <stddef.h>
        #include <stdio.h>        

        #include "parser.h"
        #include "lexer.h"
        #include "exitcodes.h"

        extern FILE *infile;
        extern ptrdiff_t label_defs_len, label_refs_len;

        //
        // frees resources and calls exit with an ExitCode
        //
        inline void panic(ExitCode err) {
                if (label_defs_len > 0)
                        for (int i = 0; i < label_defs_len; ++i)
                                free(label_defs[i].label_text);

                if (label_refs_len > 0)
                        for (int i = 0; i < label_refs_len; ++i)
                                free(label_refs[i].label_text);

                if (infile_buffer)
                        free(infile_buffer);
                if (tkn_stream)
                        free(tkn_stream);
                if (label_defs)
                        free(label_defs);
                if (label_refs)
                        free(label_refs);
                if (outfile_buffer)
                        free(outfile_buffer);
                if (infile)
                        fclose(infile);

                exit(err);
        }
#endif
