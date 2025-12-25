#CONFIG_PROFILE=y
#CONFIG_X86_32=y
#CONFIG_ARM32=y
#CONFIG_WIN32=y
#CONFIG_SOFTFLOAT=y
CONFIG_ASAN=y
#CONFIG_GPROF=y
CONFIG_SMALL=y
# consider warnings as errors (for development)
#CONFIG_WERROR=y

ifdef CONFIG_ARM32
CROSS_PREFIX=arm-linux-gnu-
endif

ifdef CONFIG_WIN32
  ifdef CONFIG_X86_32
    CROSS_PREFIX?=i686-w64-mingw32-
  else
    CROSS_PREFIX?=x86_64-w64-mingw32-
  endif
  EXE=.exe
else
  CROSS_PREFIX?=
  EXE=
endif

HOST_CC=gcc
CC=$(CROSS_PREFIX)gcc
CFLAGS=-Wall -g -MMD -D_GNU_SOURCE -fno-math-errno -fno-trapping-math -Isrc/core -Isrc/libc -Isrc/utils
HOST_CFLAGS=-Wall -g -MMD -D_GNU_SOURCE -fno-math-errno -fno-trapping-math -lm
ifdef CONFIG_WERROR
CFLAGS+=-Werror
HOST_CFLAGS+=-Werror
endif
ifdef CONFIG_ARM32
CFLAGS+=-mthumb
endif
ifdef CONFIG_SMALL
CFLAGS+=-Os
else
CFLAGS+=-O2
endif
#CFLAGS+=-fstack-usage
ifdef CONFIG_SOFTFLOAT
CFLAGS+=-msoft-float
CFLAGS+=-DUSE_SOFTFLOAT
endif # CONFIG_SOFTFLOAT
HOST_CFLAGS+=-O2
LDFLAGS=-g
HOST_LDFLAGS=-g
ifdef CONFIG_GPROF
CFLAGS+=-p
LDFLAGS+=-p
endif
ifdef CONFIG_ASAN
CFLAGS+=-fsanitize=address -fno-omit-frame-pointer
LDFLAGS+=-fsanitize=address -fno-omit-frame-pointer
endif
ifdef CONFIG_X86_32
CFLAGS+=-m32
LDFLAGS+=-m32
endif
ifdef CONFIG_PROFILE
CFLAGS+=-p
LDFLAGS+=-p
endif

# when cross compiling from a 64 bit system to a 32 bit system, force
# a 32 bit output
ifdef CONFIG_X86_32
MQJS_BUILD_FLAGS=-m32
endif
ifdef CONFIG_ARM32
MQJS_BUILD_FLAGS=-m32
endif

PROGS=mqjs$(EXE) example$(EXE)
TEST_PROGS=dtoa_test libm_test 

all: $(PROGS)

MQJS_OBJS=src/repl/mqjs.o src/utils/readline_tty.o src/utils/readline.o src/core/mquickjs.o src/libc/dtoa.o src/libc/libm.o src/utils/cutils.o
LIBS=-lm

mqjs$(EXE): $(MQJS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

src/core/mquickjs.o: src/core/mquickjs_atom.h

# Build tools for generating headers
mqjs_stdlib: tools/mquickjs_build.host.o src/repl/mqjs_stdlib.host.o
	$(HOST_CC) $(HOST_LDFLAGS) -o $@ $^

tools/mquickjs_build.host.o: tools/mquickjs_build.c
	$(HOST_CC) $(HOST_CFLAGS) -Isrc/utils -Isrc/core -c -o $@ $<

src/repl/mqjs_stdlib.host.o: src/repl/mqjs_stdlib.c
	$(HOST_CC) $(HOST_CFLAGS) -Isrc/utils -Isrc/core -Itools -c -o $@ $<

src/core/mquickjs_atom.h: mqjs_stdlib
	./mqjs_stdlib -a $(MQJS_BUILD_FLAGS) > $@

src/repl/mqjs_stdlib.h: mqjs_stdlib
	./mqjs_stdlib $(MQJS_BUILD_FLAGS) > $@

src/repl/mqjs.o: src/repl/mqjs_stdlib.h

# C API example
examples/example.o: examples/example_stdlib.h

example$(EXE): examples/example.o src/core/mquickjs.o src/libc/dtoa.o src/libc/libm.o src/utils/cutils.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

examples/example_stdlib: examples/example_stdlib.host.o tools/mquickjs_build.host.o
	$(HOST_CC) $(HOST_LDFLAGS) -o $@ $^

examples/example_stdlib.host.o: examples/example_stdlib.c
	$(HOST_CC) $(HOST_CFLAGS) -Isrc/utils -Isrc/core -Iexamples -Isrc/repl -Itools -c -o $@ $<

examples/example_stdlib.h: examples/example_stdlib
	./examples/example_stdlib $(MQJS_BUILD_FLAGS) > $@

# 编译规则
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/utils/%.o: src/utils/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/core/%.o: src/core/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/libc/%.o: src/libc/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/repl/%.o: src/repl/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

examples/%.o: examples/%.c
	$(CC) $(CFLAGS) -Iexamples -c -o $@ $<

tools/%.o: tools/%.c
	$(CC) $(CFLAGS) -Isrc/utils -Isrc/core -Iexamples -Isrc/libc -c -o $@ $<

%.host.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -Isrc/utils -Isrc/core -Iexamples -Isrc/libc -Itools -c -o $@ $<

test: mqjs example
	./mqjs tests/test_closure.js
	./mqjs tests/test_language.js
	./mqjs tests/test_loop.js
	./mqjs tests/test_builtin.js
# test bytecode generation and loading
	./mqjs -o test_builtin.bin tests/test_builtin.js
#	@sha256sum -c test_builtin.sha256
	./mqjs test_builtin.bin
	./example tests/test_rect.js

microbench: mqjs
	./mqjs tests/microbench.js

octane: mqjs
	./mqjs --memory-limit 256M tests/octane/run.js

size: mqjs
	size mqjs src/repl/mqjs.o src/utils/readline.o src/utils/cutils.o src/libc/dtoa.o src/libc/libm.o src/core/mquickjs.o

dtoa_test: tests/dtoa_test.o src/libc/dtoa.o src/utils/cutils.o tests/gay-fixed.o tests/gay-precision.o tests/gay-shortest.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

libm_test: tests/libm_test.o src/libc/libm.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rempio2_test: tests/rempio2_test.o src/libc/libm.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f *.o *.d *~ tests/*.o tests/*.d tests/*~ test_builtin.bin mqjs_stdlib mqjs_stdlib.h mquickjs_build_atoms mquickjs_atom.h mqjs_example example_stdlib example_stdlib.h $(PROGS) $(TEST_PROGS) tools/mqjs_js_module_gen examples/example_stdlib examples/example_stdlib.h
	find . \( -name "*.o" -o -name "*.d" -o -name "*~" \) -type f -not -path "./build/*" -delete

-include $(wildcard *.d)
-include $(wildcard src/*/*.d)
-include $(wildcard examples/*.d)