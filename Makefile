all: default

default: netfileserver.c netfileserver.h
	gcc -Wall -Werror -fsanitize=address -std=c99 -lpthread netfileserver.c -o server
	
clang: netfileserver.c netfileserver.h
	clang -g -fsanitize=address -std=c99 -fno-omit-frame-pointer -lpthread netfileserver.c -o server

clean: 
	rm -f server
