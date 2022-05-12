ccflags-y := \
  -DDEBUG \
  -ggdb3 \
  -std=gnu99 \
  -Werror \
  -Wframe-larger-than=1000000000 \
  -Wno-declaration-after-statement \
  $(CCFLAGS)


INC_DIR = ./


us:
	gcc -Wall main.c helper.c blowfish.c -o us -lpthread

test:
	gcc -Wall test.c helper.c blowfish.c -o test -lpthread