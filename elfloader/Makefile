include ../starlet.mk

CFLAGS += -fpic
LDSCRIPT = stub.ld
LIBS = -lgcc

TARGET = elfloader.elf
TARGET_BIN = elfloader.bin
OBJS = start.o stub.o string.o utils.o

include ../common.mk

all: $(TARGET_BIN)

$(TARGET_BIN): $(TARGET)
	@echo  "  OBJCPY    $@"
	@$(OBJCOPY) -O binary $< $@

clean: myclean

myclean:
	-rm -f $(TARGET_BIN)

