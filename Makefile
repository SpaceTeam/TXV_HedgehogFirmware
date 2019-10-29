# name of the project, the binaries will be generated with this name
PROJ_NAME = HedgehogFirmware

# hardware version, can be overwritten when calling make: make HW_VERSION=3
# complete rebuild needed after changing version
# 1: prototype v0.1 (not supported by this firmware)
# 2: prototype v0.2
# 3: prototype v0.3
# 4: prototype v0.4
# 10: production v1.0
HW_VERSION = 10

# remote flashing target
REMOTE = pi@raspberrypi.local
REMOTE_BUNDLE = /home/pi/HedgehogBundle/firmware/HedgehogFirmware

#compiler
export CC = arm-none-eabi-gcc
#linker
export LD = arm-none-eabi-gcc
#objcopy
export OC = arm-none-eabi-objcopy
#size
export SZ = arm-none-eabi-size

#linker flags
export LDFLAGS = -mcpu=cortex-m4 -DSTM32F401xC -mlittle-endian -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -static -Wl,--gc-sections
#compiler flags
export CFLAGS = -mcpu=cortex-m4 -DSTM32F401xC -mlittle-endian -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fno-common -ffunction-sections -fdata-sections -Wall -Og -g -c

###################################################

#root directory containing the makefile and user source files
ROOTDIR := $(CURDIR)

#directory containing system files
SYSTEMDIR := system

#directory containing user sources
SRCDIR := src

#directory containing HCP souce files
HCPDIR := src/hcp

#directory for binary output files
BUILDDIR := build

#directory for object files
OBJDIR := build/obj

#directory for openocd flash scripts
OPENOCDDIR := openocd

###################################################

#include directories
CFLAGS += -I$(SYSTEMDIR) -I$(SRCDIR) -I$(HCPDIR)

#hw version define
CFLAGS += -DHW_VERSION=$(HW_VERSION)

#linker file
LDFLAGS += -T$(SYSTEMDIR)/STM32F401XB_FLASH.ld

#libraries
LDFLAGS += -lm #math
#LDFLAGS += -lc -lnosys #newlib (e.g. for printf)

#system source files
SRC = startup_stm32f401xc.s system.c

#user source files
SRC += main.c gpio.c output.c power.c digitalIO.c servo.c adc.c motor.c uart.c ringbuffer.c systick.c imu.c

#user hcp source files
SRC += hcp.c hcp_commands.c hcp_handler_analogReq.c hcp_handler_digitalReq.c hcp_handler_ioConfig.c hcp_handler_motor.c hcp_handler_servo.c hcp_handler_uart.c hcp_handler_version.c hcp_handler_emergency.c hcp_handler_imu.c hcp_handler_speaker.c hcp_handler_st.c


#object files (with object directory --> $(OBJDIR)/name.o)
OBJS = $(addprefix $(OBJDIR)/,$(subst .c,.o,$(subst .s,.o,$(SRC))))

###################################################

.PHONY: all buildAll flash remote-flash flash-stlink size clean

#build and show size
all: buildAll size

#build all output formats
buildAll: $(BUILDDIR)/$(PROJ_NAME).elf $(BUILDDIR)/$(PROJ_NAME).hex $(BUILDDIR)/$(PROJ_NAME).bin
	@echo
	@echo build finished with HW_VERSION = $(HW_VERSION)

#flash locally via HedgehogFirmwareBundle
flash:
	openocd/flash_hex.sh $(BUILDDIR)/$(PROJ_NAME).hex

#flash remotely via HedgehogFirmwareBundle
flash-remote:
	rsync -avz $(BUILDDIR)/$(PROJ_NAME).hex $(REMOTE):/tmp/
	ssh $(REMOTE) "cd $(REMOTE_BUNDLE) && make flash-tmp"
	
flash-tmp:
	$(OPENOCDDIR)/flash_hex.sh /tmp/$(PROJ_NAME).hex

#flash using github.com/texane/stlink
flash-stlink:
	st-flash --reset write $(BUILDDIR)/$(PROJ_NAME).bin 0x8000000
	@echo flash finished

#flash using openocd
flash-stlink-openocd:
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg -c "program $(BUILDDIR)/$(PROJ_NAME).hex 0x00 verify; reset run; exit"
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg -c "init" -c "mdw 0x1FFF7A10 3" -c "exit"
	@echo flash finished


#shows size of .elf
size: $(BUILDDIR)/$(PROJ_NAME).elf
	@echo size:
	@$(SZ) $<


#create .hex from .elf
$(BUILDDIR)/$(PROJ_NAME).hex: $(BUILDDIR)/$(PROJ_NAME).elf | $(BUILDDIR)
	@echo creating $@ from $<
	@$(OC) -Oihex $< $@

#create .bin from .elf
$(BUILDDIR)/$(PROJ_NAME).bin: $(BUILDDIR)/$(PROJ_NAME).elf | $(BUILDDIR)
	@echo creating $@ from $<
	@$(OC) -Obinary $< $@

#link objects to .elf
$(BUILDDIR)/$(PROJ_NAME).elf: $(OBJS) | $(OBJDIR) $(BUILDDIR)
	@echo
	@echo linking $@ from $(OBJS)
	@$(LD) -o $@ $(OBJS) $(LDFLAGS)
	@echo


#compile user .c file to .o
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo compiling $@ from $<
	@$(CC) -o $@ $< $(CFLAGS)

#compile user hcp .c file to .o
$(OBJDIR)/%.o: $(HCPDIR)/%.c | $(OBJDIR)
	@echo compiling $@ from $<
	@$(CC) -o $@ $< $(CFLAGS)

#compile system .c file to .o
$(OBJDIR)/%.o: $(SYSTEMDIR)/%.c | $(OBJDIR)
	@echo compiling $@ from $<
	@$(CC) -o $@ $< $(CFLAGS)

#compile system .s file to .o
$(OBJDIR)/%.o: $(SYSTEMDIR)/%.s | $(OBJDIR)
	@echo compiling $@ from $<
	@$(CC) -o $@ $< $(CFLAGS)


#create directory for objects
$(OBJDIR):
	mkdir -p $@

#create directory for output files
$(BUILDDIR):
	mkdir -p $@


#delete all build products
clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BUILDDIR)/$(PROJ_NAME).*
	@echo
