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

.PHONY: all

all: build/bin2c build/libbin2c.a

build/bin2c: build/bin2c.o build/help.o build/lookuptable.o
	$(mkbuild)
	$(link_c)

build/libbin2c.a: build/libbin2c.o build/lookuptable.o
	$(AR) rcs $@ $^

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

build/bin2c.o: CPPFLAGS+=-DBIN2C_INLINE=1
build/genlookup.o build/bin2c_bootstrap.o: CPPFLAGS+=-DBIN2C_HEADER_ONLY=1
build/genlookup.o build/bin2c_bootstrap.o build/bin2c.o: src/bin2c.h
build/bin2c_bootstrap.o: src/bin2c.c
	$(mkbuild)
	$(compile_c) $< -o $@

build/lookuptable.o: build/bin2c_bootstrap build/genlookup
	build/genlookup \
		| build/bin2c_bootstrap bin2c_lookup_table_ \
		| $(compile_c) - -o $@

# Housekeeping

.PHONY: install uninstall clean

install: build/bin2c build/libbin2c.a
	mkdir -p "$(prefix)/bin" "$(prefix)/lib" "$(prefix)/include"
	cp build/bin2c "$(prefix)/bin"
	cp build/libbin2c.a "$(prefix)/lib"
	cp src/bin2c.h "$(prefix)/include"

uninstall:
	rm "$(prefix)/bin/bin2c" "$(prefix)/lib/libbin2c.a" "$(prefix)/include/bin2c.h"

clean:
	rm -rf build

# Testing

.PHONY: test test_bin2c test_bootstrap test_lib
.PHONY: test_lib_headeronly test_lib_inline test_lib_linked

test: test_lib test_bin2c test_bootstrap

test_bin2c: build/bin2c build/genbytes build/print_myfile.o
	bash ./test.sh "$<"

test_bootstrap: build/bin2c_bootstrap build/genbytes build/print_myfile.o
	bash ./test.sh "$<"

test_lib: test_lib_headeronly test_lib_inline test_lib_linked

test_lib_headeronly: build/test_headeronly
	$<
build/test_headeronly: build/test_headeronly.o
	$(mkbuild)
	$(link_c)
build/test_headeronly.o: CPPFLAGS+=-DBIN2C_HEADER_ONLY=1
build/test_headeronly.o: test/test.c
	$(mkbuild)
	$(compile_c) $< -o $@

test_lib_inline: build/test_inline
	$<
build/test_inline: build/test_inline.o build/libbin2c.a
	$(mkbuild)
	$(link_c)
build/test_inline.o: CPPFLAGS+=-DBIN2C_HEADER_ONLY=1
build/test_inline.o: test/test.c
	$(mkbuild)
	$(compile_c) $< -o $@

test_lib_linked: build/test_linked
	$<
build/test_linked: build/test_linked.o build/libbin2c.a
	$(mkbuild)
	$(link_c)
build/test_linked.o: test/test.c
	$(mkbuild)
	$(compile_c) $< -o $@

# Benchmarking

.PHONY: bench bench_opt bench_full

previous_versions = \
	be5110f32aca3fcaa8a45e738efc27ba310bd6b4:0001_initial \
	fe09dd180121290f6eddbc2e981cc32295caa668:0002_manual_buffer \
	3c9a344fbd321625f6c0a9c4639bfedbf7092cc7:0003_inline_octal \
	2441ea27f836d44d7d3a9e2612ada202d080662d:0004_inline_isprint \
	fab73316621efc556ac2e66ba0a3db49771c7a29:0005_lookup \
	ad8689ec37d316e07687bde66f27abeec36d17d9:0006_branchless_length \
	61bf0b40638d461935d4b1b05ea269e85abc104c:0007_lookup_length \
	3a727fa92f746126ddf4bc53d892b81e6f02d6b2:0008_32bit_lookup_access

previous_version_names = $(shell echo "$(previous_versions)" \
	| tr ' ' '\n' \
	| awk -F : '{print($$2)}')
previous_version_binaries = $(shell echo "$(previous_version_names)" \
	| tr ' ' '\n' \
	| awk '{ print("build/previous/" $$0 "/build/bin2c") }')
latest_previous_version_name = $(shell echo "$(previous_version_names)" \
	| tr ' ' '\n' \
	| tail -n 1)
latest_previous_version_binary = $(shell echo "$(previous_version_binaries)" \
	| tr ' ' '\n' \
	| tail -n 1)

bench: build/bin2c
	bash ./benchmark.sh benchmark "benchmark-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

bench_full: build/bin2c $(previous_version_binaries)
	BENCH_PREVIOUS_VERSIONS="$(previous_version_names)" \
		bash ./benchmark.sh benchmark "benchmark-full-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

bench_opt: build/bin2c $(previous_version_binaries)
	BENCH_NO_COMPILE=1 BENCH_NO_XXD=1 BENCH_NO_LD=1 \
		BENCH_PREVIOUS_VERSIONS="$(previous_version_names)" \
		bash ./benchmark.sh benchmark "benchmark-opt-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"

bench_quickopt: build/bin2c $(latest_previous_version_binary)
	BENCH_NO_XXD=1 BENCH_NO_LD=1 BENCH_NO_COMPILE=1 \
		BENCH_PREVIOUS_VERSIONS="$(latest_previous_version_name)" \
		bash ./benchmark.sh benchmark "benchmark-quckopt-$(shell date +"%Y-%m-%d-%H:%M:%S").txt"


build/previous/%/build/bin2c:
	$(eval prev_revision := $(shell echo "$(previous_versions)" \
			| tr ' ' '\n' \
			| awk -F : -v name="$*" '$$2 == name { print($$1) }'))
	mkdir -p "$(dir $@)/.."
	git archive "$(prev_revision)" | tar -C "$(dir $@)/.." -x
	$(MAKE) -C "$(dir $@)/.."
