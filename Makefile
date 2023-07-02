CC=clang
DEBUGOBJ=debug-obj
OBJ=obj
SRC=src

SRCS=parse_printf.c example.c test.c
OBJS=$(foreach obj,$(SRCS:src/%=%),$(OBJ)/$(basename $(obj)).o)

CFLAGS=-Iinclude/ -Wall -Wextra
DEBUGCFLAGS=$(CFLAGS) -g3
RELEASECFLAGS=$(CFLAGS) -Ofast

TARGET=test

DEBUGTARGET=bin/printf_debug_test
DEBUGOBJS=$(foreach obj,$(SRCS:src/%=%),$(DEBUGOBJ)/$(basename $(obj)).d.o)

.PHONY: all clean debug compile_commands
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $(TARGET))
	@$(CC) $^ -o $@
	@strip $(TARGET)

clean:
	@find $(OBJ) -name '*.o' -type f -delete
	@$(RM) $(TARGET)

debug_clean:
	@find $(DEBUGOBJ) -name '*.d.o' -type f -delete
	@$(RM) $(TARGET)

debug: $(DEBUGTARGET)

$(DEBUGTARGET): $(DEBUGOBJS)
	@mkdir -p $(dir $(DEBUGTARGET))
	@$(CC) $^ -o $@

$(OBJ)/%.o: %.c
	@mkdir -p $(shell dirname $@)
	@$(CC) $(RELEASECFLAGS) -c $< -o $@

$(DEBUGOBJ)/%.d.o: %.c
	@mkdir -p $(shell dirname $@)
	@$(CC) $(DEBUGCFLAGS) -c $< -o $@

compile_commands: clean
	@bear -- make

print_info:
	@echo $(OBJS)
	@echo $(SRCS)
