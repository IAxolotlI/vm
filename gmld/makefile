
INCLUDE_FLAGS=-I ../gm/ -I ../interface -I ../common

FLAGS_COMPILE:=-x c -Wall
FLAGS_LINKING:=-lc

EXTRAFLAGS_COMPILE=
EXTRAFLAGS_LINKING=

goal : setup gmld

setup :
	@if [ -e ../build ]; then \
		echo 'build dir found'; \
	else \
		mkdir ../build; \
	fi

gmld :
	@make --no-print-directory ../build/gmld

../build/gmld_toolset.o : gmld_toolset.c gmld_toolset.h gmld_defs.h
	@gcc $(FLAGS_COMPILE) $(EXTRAFLAGS_COMPILE) $(INCLUDE_FLAGS) -c gmld_toolset.c -o ../build/gmld_toolset.o

../build/gmld.o : gmld.c gmld_toolset.h gmld_defs.h
	@gcc $(FLAGS_COMPILE) $(EXTRAFLAGS_COMPILE) $(INCLUDE_FLAGS) -c gmld.c -o ../build/gmld.o

../build/gmld : ../build/gmld_toolset.o ../build/gmld.o 
	@gcc $(FLAGS_LINKING) $(EXTRAFLAGS_LINKING) ../build/gmld.o ../build/gmld_toolset.o -o ../build/gmld
