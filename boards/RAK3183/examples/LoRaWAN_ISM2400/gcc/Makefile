TARGET := rak3183
COMPILERNAME := gcc
PROJECT := rak3183
CONFIG := bin

SHELL:=/bin/bash

# Enable printing explicit commands with 'make VERBOSE=1'
ifneq ($(VERBOSE),1)
Q:=@
endif

#### Setup ####

TOOLCHAIN ?= arm-none-eabi
PART = apollo3
CPU = cortex-m4
FPU = fpv4-sp-d16
# Default to FPU hardware calling convention.  However, some customers and/or
# applications may need the software calling convention.
#FABI = softfp
FABI = hard

LINKER_FILE := ./rak3183.ld
STARTUP_FILE := ./startup_$(COMPILERNAME).c

#### Required Executables ####
CC = $(TOOLCHAIN)-gcc
GCC = $(TOOLCHAIN)-gcc
CPP = $(TOOLCHAIN)-cpp
LD = $(TOOLCHAIN)-ld
CP = $(TOOLCHAIN)-objcopy
OD = $(TOOLCHAIN)-objdump
RD = $(TOOLCHAIN)-readelf
AR = $(TOOLCHAIN)-ar
SIZE = $(TOOLCHAIN)-size
RM = $(shell which rm 2>/dev/null)

EXECUTABLES = CC LD CP OD AR RD SIZE GCC
K := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $($(exec)) 2>/dev/null),,\
        $(info $(exec) not found on PATH ($($(exec))).)$(exec)))
$(if $(strip $(value K)),$(info Required Program(s) $(strip $(value K)) not found))

ifneq ($(strip $(value K)),)
all clean:
	$(info Tools $(TOOLCHAIN)-$(COMPILERNAME) not installed.)
	$(RM) -rf bin
else

DEFINES = -DPART_$(PART)
DEFINES+= -DAM_PACKAGE_BGA
DEFINES+= -DAM_PRINTF_BUFSIZE=256
DEFINES+= -DAM_PART_APOLLO3
DEFINES+= -Dgcc
DEFINES+= -DSX128X
DEFINES+= -DRP2_101
DEFINES+= -DREGION_WW2G4
DEFINES+= -DWW2G4_SINGLE_DATARATE
DEFINES+= -DAM_UTIL_FAULTISR_PRINT

INCLUDES = -I../../../../../devices
INCLUDES+= -I../../../../../utils
INCLUDES+= -I../../../../../CMSIS/ARM/Include
INCLUDES+= -I../../../../..
INCLUDES+= -I../src
INCLUDES+= -I../../../bsp
INCLUDES+= -I../../../../../CMSIS/AmbiqMicro/Include
INCLUDES+= -I../../../../../mcu/apollo3
INCLUDES+= -I../smtc_modem_core/radio_drivers/sx128x_driver/src
INCLUDES+= -I../smtc_modem_core/smtc_ral/src
INCLUDES+= -I../radio_hal
INCLUDES+= -I../smtc_modem_api
INCLUDES+= -I../smtc_modem_hal
INCLUDES+= -I../smtc_modem_core/radio_drivers/sx128x_driver/src
INCLUDES+= -I../smtc_modem_core/smtc_modem_crypto/soft_secure_element
INCLUDES+= -I../smtc_modem_core
INCLUDES+= -I../smtc_modem_core/modem_config
INCLUDES+= -I../smtc_modem_core/modem_core
INCLUDES+= -I../smtc_modem_core/modem_supervisor
INCLUDES+= -I../smtc_modem_core/device_management
INCLUDES+= -I../smtc_modem_core/modem_services
INCLUDES+= -I../smtc_modem_core/lorawan_api
INCLUDES+= -I../smtc_modem_core/smtc_modem_services/headers
INCLUDES+= -I../smtc_modem_core/smtc_modem_services/src
INCLUDES+= -I../smtc_modem_core/smtc_modem_services/src/stream
INCLUDES+= -I../smtc_modem_core/smtc_modem_services/src/file_upload
INCLUDES+= -I../smtc_modem_core/smtc_modem_services/src/alc_sync
INCLUDES+= -I../smtc_modem_core/smtc_modem_services
INCLUDES+= -I../smtc_modem_core/smtc_ral/src
INCLUDES+= -I../smtc_modem_core/smtc_ralf/src
INCLUDES+= -I../smtc_modem_core/lr1mac
INCLUDES+= -I../smtc_modem_core/lr1mac/src
INCLUDES+= -I../smtc_modem_core/lr1mac/src/services
INCLUDES+= -I../smtc_modem_core/lr1mac/src/lr1mac_class_c
INCLUDES+= -I../smtc_modem_core/lr1mac/src/lr1mac_class_b
INCLUDES+= -I../smtc_modem_core/radio_planner/src
INCLUDES+= -I../smtc_modem_core/smtc_modem_crypto
INCLUDES+= -I../smtc_modem_core/smtc_modem_crypto/smtc_secure_element
INCLUDES+= -I../smtc_modem_core/lorawan_api
INCLUDES+= -I../smtc_modem_hal
INCLUDES+= -I../smtc_modem_core/lr1mac/src/smtc_real/src

VPATH = ../../../../../devices
VPATH+=:../src
VPATH+=:../../../../../utils
VPATH+=:../smtc_modem_hal
VPATH+=:../smtc_modem_core
VPATH+=:../smtc_modem_core/device_management
VPATH+=:../smtc_modem_core/lorawan_api
VPATH+=:../smtc_modem_core/lr1mac
VPATH+=:../smtc_modem_core/lr1mac/src
VPATH+=:../smtc_modem_core/lr1mac/src/lr1mac_class_b
VPATH+=:../smtc_modem_core/lr1mac/src/lr1mac_class_c
VPATH+=:../smtc_modem_core/lr1mac/src/services
VPATH+=:../smtc_modem_core/lr1mac/src/smtc_real/src
VPATH+=:../smtc_modem_core/modem_config
VPATH+=:../smtc_modem_core/modem_core
VPATH+=:../smtc_modem_core/modem_services
VPATH+=:../smtc_modem_core/modem_services/fragmentation
VPATH+=:../smtc_modem_core/modem_supervisor
VPATH+=:../smtc_modem_core/radio_drivers/sx128x_driver/src
VPATH+=:../smtc_modem_core/radio_planner/src
VPATH+=:../smtc_modem_core/smtc_modem_crypto
VPATH+=:../smtc_modem_core/smtc_modem_crypto/smtc_secure_element
VPATH+=:../smtc_modem_core/smtc_modem_crypto/soft_secure_element
VPATH+=:../smtc_modem_core/smtc_modem_services
VPATH+=:../smtc_modem_core/smtc_modem_services/src
VPATH+=:../smtc_modem_core/smtc_ral/src
VPATH+=:../smtc_modem_core/smtc_ralf/src
VPATH+=:../smtc_modem_api
VPATH+=:../radio_hal

SRC = main.c
SRC += am_util_faultisr.c
SRC += am_util_stdio.c
SRC += am_util_delay.c
SRC += am_util_id.c
SRC += startup_gcc.c


SRC += smtc_hal_spi.c
SRC += smtc_hal_gpio.c
SRC += smtc_hal_lp_timer.c
SRC += smtc_hal_rtc.c
SRC += smtc_hal_trace.c
SRC += smtc_hal_rng.c
SRC += smtc_hal_flash.c
SRC += smtc_hal_mcu.c
SRC += at_cmd.c
SRC += lorawan_app.c
SRC += i2c.c
SRC += rak1904.c
SRC += rak1901.c

SRC += ral_sx128x_bsp.c
SRC += sx128x_hal.c
SRC += smtc_modem_hal.c

SRC += lorawan_api.c
SRC += ralf_sx128x.c
SRC += sx128x.c
SRC += dm_downlink.c
SRC += region_ww2g4.c
SRC += cmac.c
SRC += modem_context.c
SRC += soft_se.c
SRC += smtc_modem.c
SRC += smtc_modem_test.c
SRC += aes.c
SRC += modem_utilities.c
SRC += smtc_modem_services_hal.c
SRC += lorawan_certification.c
SRC += modem_supervisor.c
SRC += fifo_ctrl.c
SRC += lr1_stack_mac_layer.c
SRC += lr1mac_utilities.c
SRC += lr1mac_core.c
SRC += smtc_real.c
SRC += smtc_duty_cycle.c
SRC += lr1mac_class_c.c
SRC += smtc_lbt.c
SRC += smtc_ping_slot.c
SRC += smtc_beacon_sniff.c
SRC += smtc_modem_crypto.c
SRC += radio_planner.c
SRC += radio_planner_hal.c
SRC += ral_sx128x.c

CSRC = $(filter %.c,$(SRC))
ASRC = $(filter %.s,$(SRC))

OBJS = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS+= $(ASRC:%.s=$(CONFIG)/%.o)

DEPS = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS+= $(ASRC:%.s=$(CONFIG)/%.d)

LIBS = ../../../../../mcu/apollo3/hal/gcc/bin/libam_hal.a

CFLAGS = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
CFLAGS+= -ffunction-sections -fdata-sections -fomit-frame-pointer
CFLAGS+= -MMD -MP -std=c99 
CFLAGS+= -O0 
CFLAGS+= $(DEFINES)
CFLAGS+= $(INCLUDES)
#CFLAGS+= -Wall
#CFLAGS+= -specs=nosys.specs
CFLAGS+= -w

LFLAGS = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
LFLAGS+= -nostartfiles -static
LFLAGS+= -Wl,--gc-sections,--entry,Reset_Handler,-Map,$(CONFIG)/$(TARGET).map
LFLAGS+= -Wl,--start-group -lm -lc -lgcc $(LIBS) -Wl,--end-group
LFLAGS+= -specs=nosys.specs

# Additional user specified CFLAGS
CFLAGS+=$(EXTRA_CFLAGS)

CPFLAGS = -Obinary

ODFLAGS = -S

#### Rules ####
all: directories $(CONFIG)/$(TARGET).bin

directories: $(CONFIG)

$(CONFIG):
	@mkdir -p $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d
	@echo " Compiling $(COMPILERNAME) $<"
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d
	@echo " Assembling $(COMPILERNAME) $<"
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET).axf: $(OBJS) $(LIBS)
	@echo " Linking $(COMPILERNAME) $@"
	$(Q) $(CC) -Wl,-T,$(LINKER_FILE) -o $@ $(OBJS) $(LFLAGS)

$(CONFIG)/$(TARGET).bin: $(CONFIG)/$(TARGET).axf
	@echo " Copying $(COMPILERNAME) $@..."
	$(Q) $(CP) $(CPFLAGS) $< $@
	$(Q) $(OD) $(ODFLAGS) $< > $(CONFIG)/$(TARGET).lst
	$(Q) $(SIZE) $(OBJS) $(LIBS) $(CONFIG)/$(TARGET).axf >$(CONFIG)/$(TARGET).size
	@echo " Copying $(COMPILERNAME) $(CONFIG)/$(TARGET).hex"
	$(Q) $(CP) -O ihex $< $(CONFIG)/$(TARGET).hex

clean:
	@echo "Cleaning..."
#	$(Q) $(RM) -f $(OBJS) $(DEPS) \
#	    $(CONFIG)/$(TARGET).bin $(CONFIG)/$(TARGET).axf \
#	    $(CONFIG)/$(TARGET).lst $(CONFIG)/$(TARGET).map
	rm -rf bin
$(CONFIG)/%.d: ;

../../../../../mcu/apollo3/hal/gcc/bin/libam_hal.a:
	$(MAKE) -C ../../../../../mcu/apollo3/hal


# Automatically include any generated dependencies
-include $(DEPS)
endif
.PHONY: all clean directories
