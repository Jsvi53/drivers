## linux-nano-5.2-tf
This is a linux driver project, which based on [linux-nano-5.2-tf](https://github.com/Lichee-Pi/linux/tree/nano-5.2-tf).

### Build tools
- arm-linux-gnueabi-gcc

### Build steps
- **step 1:** download the [linux-nano-5.2-tf](https://github.com/Lichee-Pi/linux/tree/nano-5.2-tf) to the local path;

- **step 2:** dclone this repository to the local path;
- **step 3:** replace corresponding files of the `linux-nano-5.2-tf` wite this repository;
- **step 4:** run `cd path/to/linux-nano-5.2-th` and run `make -j4`. The compiled kernel file `zImage` at `path/to/linux-nano-5.2-th/arch/arm/boot/`.

