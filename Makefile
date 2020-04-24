prefix ?= /usr/local

CFLAGS ?= -O3
CFLAGS += -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS += -I"$(PWD)/src"

define mkbuild
	@mkdir -p "$(dir $@)"
endef

define compile_c
	$(CC) $(CPPFLAGS) $(CFLAGS) -x c -c
endef

define link_c
	$(CC) $(LDFLAGS) $^ -o $@
endef

build/bin2c: build/bin2c.o build/help.o build/lookuptable.o
	$(mkbuild)
	$(link_c)

build/bin2c_bootstrap: build/bin2c_bootstrap.o build/help_dummy.o
	$(mkbuild)
	$(link_c)

build/genbytes: build/genbytes.o
	$(mkbuild)
	$(link_c)

build/genlookup: build/genlookup.o
	$(mkbuild)
	$(link_c)

build/%.o: src/%.txt build/bin2c_bootstrap
	$(mkbuild)
	build/bin2c_bootstrap blob_$* < $< | $(compile_c) - -o $@

build/%.o: src/%.c
	$(mkbuild)
	$(compile_c) $< -o $@

build/genlookup.o build/bin2c_bootstrap.o: CPPFLAGS+=-DBIN2C_BOOTSTRAP=1
build/genlookup.o build/bin2c_bootstrap.o build/bin2c.o: src/bin2c.h
build/bin2c_bootstrap.o: src/bin2c.c
	$(mkbuild)
	$(compile_c) $< -o $@

build/lookuptable.o: build/bin2c_bootstrap build/genlookup
	build/genlookup \
		| build/bin2c_bootstrap bin2c_lookup_table_ \
		| $(compile_c) - -o $@

# Housekeeping

.PHONY: install clean

install: build/bin2c
	cp $< $(prefix)/bin

clean:
	rm -rf build

# Testing

.PHONY: test test_bin2c test_bootstrap

test: test_bin2c test_bootstrap

test_bin2c: build/bin2c build/genbytes build/print_myfile.o
	bash ./test.sh "$<"

test_bootstrap: build/bin2c_bootstrap build/genbytes build/print_myfile.o
	bash ./test.sh "$<"

# Benchmarking

.PHONY: bench bench_opt bench_full

previous_versions = \
	be5110f32aca3fcaa8a45e738efc27ba310bd6b4:0001_initial \
	fe09dd180121290f6eddbc2e981cc32295caa668:0002_manual_buffer \
	3c9a344fbd321625f6c0a9c4639bfedbf7092cc7:0003_inline_octal \
	2441ea27f836d44d7d3a9e2612ada202d080662d:0004_inline_isprint

previous_version_binaries = $(shell echo "$(previous_versions)" \
	| tr ' ' '\n' \
	| awk -F : '{ print("build/previous/" $$2 "/build/bin2c") }')

bench: build/bin2c
	bash ./benchmark.sh benchmark "benchmark-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

bench_full: build/bin2c $(previous_version_binaries)
	BENCH_PREVIOUS_VERSIONS=1 \
		bash ./benchmark.sh benchmark "benchmark-full-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

bench_opt: build/bin2c $(previous_version_binaries)
	BENCH_PREVIOUS_VERSIONS=1 BENCH_NO_XXD=1 BENCH_NO_LD=1 \
		bash ./benchmark.sh benchmark "benchmark-opt-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

build/previous/%/build/bin2c:
	$(eval prev_revision := $(shell echo "$(previous_versions)" \
			| tr ' ' '\n' \
			| awk -F : -v name="$*" '$$2 == name { print($$1) }'))
	mkdir -p "$(dir $@)/.."
	git archive "$(prev_revision)" | tar -C "$(dir $@)/.." -x
	$(MAKE) -C "$(dir $@)/.."
