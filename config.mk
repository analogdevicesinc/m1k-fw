PRJ_PATH = .
ARCH = cortex-m3
PART = sam3u2c

# Application target name. Given with suffix .a for library and .elf for a
# standalone application.
TARGET_FLASH = m1000.elf

BUILD_DIR = build

# List of C source files.
CSRCS = \
	src/main.c                                                     \
	asf/common/boards/user_board/init.c                            \
	asf/common/services/clock/sam3u/sysclk.c                       \
	asf/common/services/delay/sam/cycle_counter.c                  \
	asf/common/services/sleepmgr/sam/sleepmgr.c                    \
	asf/common/services/usb/class/vendor/device/udi_vendor.c       \
	asf/common/services/usb/class/vendor/device/udi_vendor_desc.c  \
	asf/common/services/usb/udc/udc.c                              \
	asf/common/utils/interrupt/interrupt_sam_nvic.c                \
	asf/sam/drivers/pio/pio.c                                      \
	asf/sam/drivers/efc/efc.c                                      \
	asf/sam/drivers/pio/pio_handler.c                              \
	asf/sam/drivers/tc/tc.c                                        \
	asf/sam/drivers/pwm/pwm.c                                      \
	asf/sam/drivers/twi/twi.c                                      \
	asf/sam/drivers/wdt/wdt.c                                      \
	asf/sam/drivers/pmc/pmc.c                                      \
	asf/sam/drivers/pmc/sleep.c                                    \
	asf/sam/drivers/usart/usart.c                                  \
	asf/sam/drivers/udphs/udphs_device.c                           \
	asf/sam/services/flash_efc/flash_efc.c                         \
	asf/sam/utils/cmsis/sam3u/source/templates/exceptions.c        \
	asf/sam/utils/cmsis/sam3u/source/templates/gcc/startup_sam3u.c \
	asf/sam/utils/cmsis/sam3u/source/templates/system_sam3u.c      \
	asf/sam/utils/syscalls/gcc/syscalls.c

# List of assembler source files.
ASSRCS =

# List of include paths.
INC_PATH = \
	src                                         \
	asf/common/boards                           \
	asf/common/boards/user_board                \
	asf/common/services/delay                   \
	asf/common/services/clock                   \
	asf/common/services/gpio                    \
	asf/common/services/ioport                  \
	asf/common/services/sleepmgr                \
	asf/common/services/usb/udc                 \
	asf/common/services/usb                     \
	asf/common/services/usb/class/vendor        \
	asf/common/services/usb/class/vendor/device \
	asf/common/utils                            \
	asf/sam/drivers/pio                         \
	asf/sam/services/flash_efc                  \
	asf/sam/drivers/efc                         \
	asf/sam/drivers/twi                         \
	asf/sam/drivers/pmc                         \
	asf/sam/drivers/wdt                         \
	asf/sam/drivers/tc                          \
	asf/sam/drivers/pwm                         \
	asf/sam/drivers/udphs                       \
	asf/sam/drivers/usart                       \
	asf/sam/utils                               \
	asf/sam/utils/cmsis/sam3u/include           \
	asf/sam/utils/cmsis/sam3u/source/templates  \
	asf/sam/utils/header_files                  \
	asf/sam/utils/preprocessor                  \
	asf/thirdparty/CMSIS/Include                \
	asf/thirdparty/CMSIS/Lib/GCC

# Additional search paths for libraries.
LIB_PATH = \
       asf/thirdparty/CMSIS/Lib/GCC

# List of libraries to use during linking.
LIBS =  \
       arm_cortexM3l_math \
       m

# Path relative to top level directory pointing to a linker script.
LINKER_SCRIPT_FLASH = scripts/flash.ld

# Path relative to top level directory pointing to a linker script.
DEBUG_SCRIPT_FLASH =

# Project type parameter: all, sram or flash
PROJECT_TYPE = flash

# Additional options for debugging. By default the common Makefile.in will
# add -g3.
DBGFLAGS =

# Application optimization used during compilation and linking:
# -O0, -O1, -O2, -O3 or -Os
OPTIMIZATION = -O2

# Extra flags to use when archiving.
ARFLAGS =

# Extra flags to use when assembling.
ASFLAGS =

# Extra flags to use when compiling.
CFLAGS = -fstack-usage -Wno-attributes

# Extra flags to use when preprocessing.
#
# Preprocessor symbol definitions
#   To add a definition use the format "-D name[=definition]".
#   To cancel a definition use the format "-U name".
#
# The most relevant symbols to define for the preprocessor are:
#   BOARD      Target board in use, see boards/board.h for a list.
#   EXT_BOARD  Optional extension board in use, see boards/board.h for a list.
CPPFLAGS = \
       -D ARM_MATH_CM3=true \
       -D BOARD=USER_BOARD  \
       -D UDD_ENABLE        \
       -D __SAM3U2C__       \
       -D printf=iprintf

# Extra flags to use when linking
LDFLAGS =
