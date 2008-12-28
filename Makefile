CC = arm-eabi-gcc
AS = arm-eabi-as
LD = arm-eabi-gcc
OBJCOPY = arm-eabi-objcopy
CFLAGS = -mbig-endian -fomit-frame-pointer -Os -fpic -Wall -I.
ASFLAGS = -mbig-endian
LDFLAGS = -nostartfiles -mbig-endian -Wl,-T,stub.ld

TARGET = iosboot.bin
ELF = iosboot.elf
OBJECTS = start.o main.o vsprintf.o string.o gecko.o memory.o memory_asm.o \
	utils_asm.o utils.o ff.o diskio.o sdhc.o powerpc_elf.o powerpc.o panic.o


$(TARGET) : $(ELF)
	@echo  "OBJCPY	$@"
	@$(OBJCOPY) -O binary $< $@

$(ELF) : stub.ld $(OBJECTS)
	@echo  "LD	$@"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.S
	@echo  "AS	$@"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o : %.c
	@echo  "CC	$@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	@echo  "DEP	$@"
	@set -e; $(CC) -M $(CFLAGS) $< \
		| sed 's?\($*\)\.o[ :]*?\1.o $@ : ?g' > $@; \
		[ -s $@ ] || rm -f $@

%.d: %.S
	@echo	"DEP	$@"
	@touch $@

-include $(OBJECTS:.o=.d)

clean:
	-rm -f *.elf *.o *.bin *.d

