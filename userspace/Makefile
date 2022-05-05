ccflags-y := \
  -DDEBUG \
  -ggdb3 \
  -std=gnu99 \
  -Werror \
  -Wframe-larger-than=1000000000 \
  -Wno-declaration-after-statement \
  $(CCFLAGS)


INC_DIR = ./


c:
	gcc -Wall main.c helper.c blowfish.c -o us_app