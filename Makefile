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

build/genbytes: build/genbytes.o
	$(mkbuild)
	$(link_c)

build/%.o: src/%.txt build/bin2c_bootstrap
	$(mkbuild)
	build/bin2c_bootstrap blob_$* < $< | $(compile_c) - -o $@

build/%.o: src/%.c
	$(mkbuild)
	$(compile_c) $< -o $@

.PHONY: install test bench clean

install: build/bin2c
	cp $< $(prefix)/bin

clean:
	rm -rf build

test: build/bin2c build/genbytes build/print_myfile.o
	bash ./test.sh

bench: build/bin2c
	bash ./benchmark.sh benchmark "benchmark-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

previous_versions = \
	build/previous/0001_initial/build/bin2c

bench_full: build/bin2c $(previous_versions)
	BENCH_PREVIOUS_VERSIONS=1 \
		bash ./benchmark.sh benchmark "benchmark-full-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

bench_opt: build/bin2c $(previous_versions)
	BENCH_PREVIOUS_VERSIONS=1 BENCH_NO_XXD=1 BENCH_NO_LD=1 \
		bash ./benchmark.sh benchmark "benchmark-opt-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

build/previous/0001_initial/build/bin2c:
	mkdir -p "$(dir $@)/.."
	git archive be5110f32aca3fcaa8a45e738efc27ba310bd6b4 \
		| tar -C "$(dir $@)/.." -x
	$(MAKE) -C "$(dir $@)/.."
