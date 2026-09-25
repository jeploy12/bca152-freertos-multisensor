import os
import subprocess

Import("env")

build_dir = env.subst("$BUILD_DIR")
python = env.subst("$PYTHONEXE")
esptool = os.path.join(
    env.subst("$PROJECT_PACKAGES_DIR"), "tool-esptoolpy", "esptool.py"
)
output = os.path.join(build_dir, "merged.bin")

command = [
    python, esptool, "merge_bin",
    "-o", output,
    "--flash_mode", "dio",
    "--flash_freq", "40m",
    "--flash_size", "4MB",
    "0x1000", os.path.join(build_dir, "bootloader", "bootloader.bin"),
    "0x8000", os.path.join(build_dir, "partition_table", "partition-table.bin"),
    "0x10000", os.path.join(build_dir, "bca152-freertos-multisensor.bin"),
]
subprocess.check_call(command)
print("Wokwi firmware:", output)
