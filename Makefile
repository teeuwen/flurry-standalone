#
# Makefile
#

CFLAGS		:= -Wall -Wextra -fdiagnostics-color=auto -std=gnu89 -g
LDLIBS		:= -lglfw -lGL -lGLU -lalut -lm -lX11

flurry-o	= src/flurry.o src/flurry-smoke.o src/flurry-spark.o src/flurry-star.o src/flurry-texture.o
flurry-i	= -I src/include

all: flurry run

PHONY += clean
clean:
	@echo -e "\033[1m> Removing binaries...\033[0m"
	@find src -type f -name '*.o' -exec rm {} \;
	@rm -f bin/flurry

%.o: %.c
	@echo -e "\033[1;37m> Compiling \033[0;32m$<\033[1m...\033[0m"
	@$(CC) $(CFLAGS) -c $< $(flurry-i) -o $@

PHONY += flurry
flurry: bin/flurry
bin/flurry: $(flurry-o)
	@echo -e "\033[1m> Linking \033[0;32m$@\033[1m...\033[0m"
	@$(CC) $(LDLIBS) -o bin/flurry $(flurry-o)

PHONY += run
run: bin/flurry
	@echo -e "\033[1m> Running flurry...\033[0m"
	@./bin/flurry

.PHONY: $(PHONY)
