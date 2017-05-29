## Know Problems
1. When using PlatformIO upload button, K303K8 will not work, as upload will choose to upload `firmware.elf`, while the drive has not enough space for that.
Need to manully upload `firmware.bin` into the drive.
2. If printf cannot print floating number, add the following lines in `platformio.ini`.
```ini
build_flags = -Wl,-u,_printf_float,-u,_scanf_float
```
