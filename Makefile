include ../../starlet.mk

ASFLAGS += -D_LANGUAGE_ASSEMBLY
CFLAGS += -DCAN_HAZ_IRQ -DCAN_HAZ_IPC -mthumb-interwork
THUMBFLAGS = -mthumb
LDSCRIPT = mini.ld
LIBS = -lgcc

ELFLOADER = ../elfloader/elfloader.bin
MAKEBIN = python ../makebin.py

TARGET = armboot.elf
TARGET_BIN = armboot.bin
OBJS = start.o memory.o memory_asm.o utils_asm.o panic.o irq_asm.o ipc.o \
	exception_asm.o nand.o exception.o ldhack.o
THUMB_OBJS = sdmmc.o sdhc.o boot2.o powerpc.o powerpc_elf.o diskio.o ff.o \
	     crypto.o seeprom.o utils.o main.o vsprintf.o string.o \
	     gecko.o irq.o

include ../../common.mk

all: $(TARGET_BIN)

main.o: main.c git_version.h

$(THUMB_OBJS): %.o : %.c
	@echo "  COMPILE[T]   $<"
	@mkdir -p $(DEPDIR)
	@$(CC) $(CFLAGS) $(THUMBFLAGS) $(DEFINES) -Wp,-MMD,$(DEPDIR)/$(*F).d,-MQ,"$@",-MP -c $< -o $@

$(TARGET_BIN): $(TARGET) $(ELFLOADER) 
	@echo  "MAKEBIN	$@"
	@$(MAKEBIN) $(ELFLOADER) $< $@

upload: $(TARGET_BIN)
	@$(WIIDEV)/bin/bootmii -a $<
	
git_version.h:
	@echo "  GITVER    $@"
	@echo 'const char git_version[] = "'`./describesimple.sh`'";' > git_version.h

clean: myclean

myclean:
	-rm -f $(TARGET_BIN) git_version.h

