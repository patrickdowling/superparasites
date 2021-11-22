# Docker container for `supercell` build

In theory (from root directory)
```
make -C docker
docker run --rm -it -v $(pwd):/build pld/supercell make -f supercell/makefile VARIANT=MICROCELL all wav
```

- Experimental. "I have no idea what I'm doing"
- The recommended compiler version (`gcc-arm-none-eabi-5_4-2016q3`) and `make` live in the container...
- ...while the source and artifacts remain on the host.

## Vague TODOs
- Yeah, the dependency errors during (first?) compilation are annoying.
- Smaller image size (alpine?)
- Seems like `openocd`-in-a-container might not work everywhere and it's better to keep that native. 
