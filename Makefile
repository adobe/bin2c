prefix ?= /usr/local

CFLAGS ?= -O3
CFLAGS += -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS += -I"$(PWD)/src"

define mkbuild
	mkdir -p "$(dir $@)"
endef

define compile_c
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -c
endef

define link_c
	$(CC) $(LDFLAGS) $^ -o $@
endef

build/bin2c: build/bin2c.o build/help.o
	$(mkbuild)
	$(link_c)

build/bin2c_bootstrap: build/bin2c.o build/help_dummy.o
	$(mkbuild)
	$(link_c)

build/genrandom: build/genrandom.o
	$(mkbuild)
	$(link_c)

build/genbytes: build/genbytes.o
	$(mkbuild)
	$(link_c)

build/%.o: src/%.txt build/bin2c_bootstrap
	$(mkbuild)
	build/bin2c_bootstrap blob_$* < $< | $(compile_c) - -o $@

build/%.o: src/%.c
	$(mkbuild)
	$(compile_c) $< -o $@

.PHONY: install test benchmark clean

install: build/bin2c
	cp $< $(prefix)/bin

test: build/bin2c build/genbytes build/genrandom build/print_myfile.o
	bash ./test.sh

bench: build/bin2c build/genrandom
	bash ./benchmark.sh benchmark "benchmark-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

clean:
	rm -rf build

build/shader_%.o: shaders/%.glsl
	bin2c shader_$* < $< \
		| $(CPP) -P -I "shaders/" \
		| $(CC) $(CPPFLAGS) $(CFLAGS) -x c -c - -o $@
