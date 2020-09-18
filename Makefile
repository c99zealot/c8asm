CC=cc
CFLAGS=-std=c99 -o c8asm

c8asm: src/main.c src/lexer.c src/parser.c src/print_msg.c
	@$(CC) $(CFLAGS) src/*.c

install:
	@install -s c8asm /bin/c8asm

clean:
	@rm c8asm

uninstall:
	@rm /bin/c8asm
