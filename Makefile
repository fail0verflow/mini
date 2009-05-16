include starlet.mk

ASFLAGS += -D_LANGUAGE_ASSEMBLY
CFLAGS += -DCAN_HAZ_IRQ -DCAN_HAZ_IPC
LDSCRIPT = mini.ld
LIBS = -lgcc

ELFLOADER = elfloader/elfloader.bin
MAKEBIN = python makebin.py

TARGET = armboot.elf
TARGET_BIN = armboot.bin
OBJS = start.o main.o ipc.o vsprintf.o string.o gecko.o memory.o memory_asm.o \
	utils_asm.o utils.o ff.o diskio.o sdhc.o powerpc_elf.o powerpc.o panic.o \
	irq.o irq_asm.o exception.o exception_asm.o seeprom.o crypto.o nand.o \
	boot2.o ldhack.o sdmmc.o

include common.mk

all: $(TARGET_BIN)

main.o: main.c git_version.h

$(TARGET_BIN): $(TARGET) $(ELFLOADER) 
	@echo  "MAKEBIN	$@"
	@$(MAKEBIN) $(ELFLOADER) $< $@

upload: $(TARGET_BIN)
	@$(WIIDEV)/bin/bootmii -a $<
	
git_version.h:
	@echo "  GITVER    $@"
	@echo 'const char git_version[] = "'`./describesimple.sh`'";' > git_version.h

clean: myclean

$(ELFLOADER):
	$(MAKE) -C elfloader

myclean:
	-rm -f $(TARGET_BIN) git_version.h
	$(MAKE) -C elfloader clean

