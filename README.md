# swe1r-patcher


swe1r-patcher enhances your "Star Wars Episode 1: Racer" (SWE1R) experience:

- Higher resolution fonts
- Allow upgraded podracers in multiplayer
- Disable player collisions in multiplayer
- Enable new debug features
- ...


**If you like my software, you can make a donation to me; more information:**

http://jannikvogel.de/


## Compatible game versions

This software has been tested with all variants of the 1.1 US release.

This includes the GOG.com and Steam re-releases.


## Installation instructions for Windows users

There are different methods to use swe1r-patcher, each has it's own benefits and drawbacks.

### DLL method (recommended)

This method sits between DirectX and the game to wait until the game is about to begin.
This makes it compatible with the Steam release, as it happens after DRM checks.
However, it might be incompatible with other software that uses the same trick.

- Copy "dinput.dll" and "textures" folder into your game directory.
- Run `swep1rcr.exe` to start the game.

### Loader method

This manually modifies the game in memory.
Due to security restrictions, this does not work for everyone.
Also, because the modifications happen very early, it's incompatible with the Steam release.

- Copy "swe1r-loader.exe" and "textures" folder into your game directory (must contain "swep1rcr.exe").
- Run `swe1r-loader.exe` to start the game.

### Patcher method

This changes your "swep1rcr.exe" file permanently.
Because this method does not run the game directly, the patcher also works on macOS and Linux.
However, it is compatible with other hacks, but incompatible with the Steam release because of DRM checks.

- Make a backup copy of your "swep1rcr.exe".
- Run `swe1r-patcher.exe <path-to-your-swep1rcr.exe>`.
- Run `swep1rcr.exe` to start the game.


## Build instructions for software developers

**The source code can be found on GitHub:**

https://github.com/JayFoxRox/swe1r-patcher


### Windows (MSYS2)

Run the following in a MinGW32 Shell:

```
pacman -S git mingw32/mingw-w64-i686-cmake mingw32/mingw-w64-i686-gcc
git clone https://github.com/JayFoxRox/swe1r-patcher.git
cd swe1r-patcher
mkdir build
cd build
cmake .. -G "MSYS Makefiles"
make
```

### macOS / Linux

It is assumed you have git, cmake and a compatible compiler installed.

```
git clone https://github.com/JayFoxRox/swe1r-patcher.git
cd swe1r-patcher
mkdir build
cd build
cmake ..
make
```


## License

### Code and documentation

**(c)2019 Jannik Vogel**


No commercial use allowed; all rights reserved.

You are only allowed to download, compile and run this software.

Further **distribution is not allowed**.

The code which is patched into the **output file is licensed under the same conditions**.

**Therefore, you have no right to restribute the softwares output**.


### Font artwork

**(c)2018 Jordan Blackburn**


All font artwork is licensed under a [Creative Commons Attribution-NonCommercial 4.0 International License](http://creativecommons.org/licenses/by-nc/4.0/).

In particular, the font artwork is found in the files with filenames matching the pattern `font*.png` and `font*.data`.
