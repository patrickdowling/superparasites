Port of Clouds/Parasites/Beat Repeat to Grayscale [Supercell](https://grayscale.info/supercell/) and [Microcell](https://grayscale.info/microcell/) platforms.

Note that this version has platform-specific drivers and a custom linker script and is **not** compatible with other hardware versions of Clouds.

It is based off the [Parasites codebase](https://github.com/mqtthiqs/parasites) and merges the two modes from [Beat Repeat](https://github.com/jkammerl/eurorack) for a total of eight modes:
- Granular
- Pitch shifter/time stretch
- Looping delay
- Spectral madness
- Oliverb
- Resonestor
- Beat Repeat
- Spectral clouds

As far as possible, the commit history has been preserved, for better for worse...

## Hardware Variants
Since there is a small hardware difference (inverted pots) there are two possible build targets: Supercell and Microcell
- To build for Supercell hardware, use `make -f supercell/makefile VARIANT=SUPERCELL ...`. The resulting files are in `build/supercell/`.
- To build for Microcell hardware, use `make -f supercell/makefile VARIANT=MICROCELL ...`. The resulting files are in `build/microcell/`.

This doesn't change anything for the bootloader (1), which should be common to both(famous last words...)

At the risk of repeating myself: This code is not compatible with other hardware versions of Clouds!

## Notes
- (1) The bootloader is the least tested part of this project.
- Released versions have been compiled using `gcc-arm-none-eabi-5_4-2016q3`
- The stmlib submodule still uses mqtthiqs/stmlib which has diverged somewhat from pichenettes'. This allowed setting of an external linker script out-of-the-box.
- However, that does mean that the scripts for programming appear to require openocd 0.9.0 so the command line calls may be different to a current version of pichenettes' repo.
- The submodule `stm_audio_bootloader` has been updated to the latest master to work with newer python versions.
- The custom linker script adds sections for relevant C++ features.

## License
Code (STM32F projects): MIT license.
