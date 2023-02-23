
GDB_PATH="/home/lomas/.espressif/tools/xtensa-esp32s3-elf/esp-2021r2-patch5-8.4.0/xtensa-esp32s3-elf/bin/xtensa-esp32s3-elf-gdb"
PROGRAM_PATH="$( pwd; )/build"
PROJECT="${PWD##*/}"

$GDB_PATH -x gdbinit $PROGRAM_PATH/$PROJECT.elf