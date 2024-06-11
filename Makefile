CC?=clang

SRCS=parse_printf.c example.c test.c
OBJS=$(SRCS:.c=.o)
DEBUG_OBJS=$(SRCS:.c=.d.o)

CFLAGS=-Iinclude/ -Wall -Wextra
DEBUG_CFLAGS=$(CFLAGS) -g3
RELEASE_CFLAGS=$(CFLAGS) -Ofast

TARGET=test
DEBUG_TARGET=test_debug

.PHONY: all clean debug compile_commands
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $(TARGET))
	@$(CC) $^ -o $@
	@strip $(TARGET)

clean:
	@find . -name '*.o' -type f -delete
	@$(RM) $(TARGET)
	@$(RM) $(DEBUG_TARGET)

debug_clean:
	@find . -name '*.d.o' -type f -delete
	@$(RM) $(DEBUG_TARGET)

debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	@mkdir -p $(dir $(DEBUG_TARGET))
	@$(CC) $^ -o $@

%.o: %.c
	@mkdir -p $(shell dirname $@)
	@$(CC) $(RELEASE_CFLAGS) -c $< -o $@

%.d.o: %.c
	@mkdir -p $(shell dirname $@)
	@$(CC) $(DEBUG_CFLAGS) -c $< -o $@

compile_commands: clean
	compiledb make

print_info:
	@echo $(OBJS)
	@echo $(SRCS)
