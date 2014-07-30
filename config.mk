# Path to top level ASF directory relative to this project directory.
PRJ_PATH = asf

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m3

# Target part: none, sam3n4 or sam4l4aa
PART = sam3u1c

# Application target name. Given with suffix .a for library and .elf for a
# standalone application.
TARGET_FLASH = helium.elf
TARGET_SRAM = helium_sram.elf

# List of C source files.
CSRCS = \
       ../src/main.c \
       common/services/clock/sam3u/sysclk.c               \
       common/services/delay/sam/cycle_counter.c          \
       common/services/sleepmgr/sam/sleepmgr.c            \
       common/services/usb/class/vendor/device/udi_vendor.c \
       common/services/usb/class/vendor/device/udi_vendor_desc.c \
       common/services/usb/udc/udc.c                      \
       common/utils/interrupt/interrupt_sam_nvic.c        \
       sam/drivers/pio/pio.c                              \
       sam/drivers/efc/efc.c                              \
       sam/drivers/pio/pio_handler.c                      \
       sam/drivers/tc/tc.c                                \
       sam/drivers/twi/twi.c                              \
       sam/drivers/wdt/wdt.c                              \
       sam/drivers/pmc/pmc.c                              \
       sam/drivers/pmc/sleep.c                            \
       sam/drivers/usart/usart.c                          \
       sam/drivers/udphs/udphs_device.c                   \
       sam/services/flash_efc/flash_efc.c                  \
       sam/utils/cmsis/sam3u/source/templates/exceptions.c \
       sam/utils/cmsis/sam3u/source/templates/gcc/startup_sam3u.c \
       sam/utils/cmsis/sam3u/source/templates/system_sam3u.c \
       sam/utils/syscalls/gcc/syscalls.c

# List of assembler source files.
ASSRCS =

# List of include paths.
INC_PATH = \
       ../src/ \
       common/boards                                      \
       common/services/delay                              \
       common/services/clock                              \
       common/services/gpio                               \
       common/services/ioport                             \
       common/services/sleepmgr                           \
       common/services/usb/udc                            \
       common/services/usb                                \
       common/services/usb/class/vendor                   \
       common/services/usb/class/vendor/device            \
       common/utils                                       \
       sam/boards                                         \
       sam/boards/sam3u_ek                                \
       sam/drivers/pio                                    \
       sam/services/flash_efc                             \
       sam/drivers/efc                                    \
       sam/drivers/twi                                    \
       sam/drivers/pmc                                    \
       sam/drivers/wdt                                    \
       sam/drivers/tc                                     \
       sam/drivers/udphs                                  \
       sam/drivers/usart                                  \
       sam/utils                                          \
       sam/utils/cmsis/sam3u/include                      \
       sam/utils/cmsis/sam3u/source/templates             \
       sam/utils/header_files                             \
       sam/utils/preprocessor                             \
       thirdparty/CMSIS/Include                           \
       thirdparty/CMSIS/Lib/GCC \

# Additional search paths for libraries.
LIB_PATH =  \
       thirdparty/CMSIS/Lib/GCC

# List of libraries to use during linking.
LIBS =  \
       arm_cortexM3l_math                                 \
       m

# Path relative to top level directory pointing to a linker script.
LINKER_SCRIPT_FLASH = sam/utils/linker_scripts/sam3u/sam3u1/gcc/flash.ld
LINKER_SCRIPT_SRAM  = sam/utils/linker_scripts/sam3u/sam3u1/gcc/sram.ld

# Path relative to top level directory pointing to a linker script.
DEBUG_SCRIPT_FLASH = sam/boards/sam3u_ek/debug_scripts/gcc/sam3u_ek_flash.gdb
DEBUG_SCRIPT_SRAM  = sam/boards/sam3u_ek/debug_scripts/gcc/sam3u_ek_sram.gdb

# Project type parameter: all, sram or flash
PROJECT_TYPE        = flash

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
CFLAGS =

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
       -D ARM_MATH_CM3=true                               \
       -D BOARD=SAM3U_EK                                  \
       -D UDD_ENABLE                                      \
       -D __SAM3U1C__                                     \
       -D printf=iprintf

# Extra flags to use when linking
LDFLAGS = \
