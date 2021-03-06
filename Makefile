CC=clang
DEBUGOBJ=debug-obj
OBJ=obj
SRC=src

SRCS=$(shell find $(SRC) -type f -name '*.c')
OBJS=$(foreach obj,$(SRCS:src/%=%),$(OBJ)/$(basename $(obj)).o)

CFLAGS=-Iinclude/ -Wall
DEBUGCFLAGS=$(CFLAGS) -g3
RELEASECFLAGS=$(CFLAGS) -Ofast

TARGET=bin/printf_test

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

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(shell dirname $@)
	@$(CC) $(RELEASECFLAGS) -c $< -o $@

$(DEBUGOBJ)/%.d.o: $(SRC)/%.c
	@mkdir -p $(shell dirname $@)
	@$(CC) $(DEBUGCFLAGS) -c $< -o $@

compile_commands: clean
	@compiledb make

print_info:
	@echo $(OBJS)
	@echo $(SRCS)
