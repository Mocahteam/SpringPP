# Spring RTS game engine with Prog&Play

## README

Spring (formerly TASpring) is an Open Source Real Time Strategy game engine.
Visit the [project homepage](http://springrts.com/) for help, suggestions,
bugs, community forum and everything spring related.

The version available in this repository is a fork of the original Spring engine with Prog&Play functionalities. Visit the [Prog&Play homepage](http://progandplay.lip6.fr/index_en.php) for details.

Binaries are available at: <http://progandplay.lip6.fr/download.php?LANG=en>

### Compiling on Windows

Softwares Required:

* CMake (version 2.6)
* Java jdk

Download following archive ([MinGW-gcc4.4.zip](http://progandplay.lip6.fr/ressources/MinGW-gcc4.4.zip)), it contains g++ compiler based on MinGW. Unzip this archive at the root of the drive "C:\". Update your environment variables:

* Create a new variable named MINGW with "C:\MinGW-gcc4.4" as value;
* Update your PATH by adding in first value: "%MINGW%\bin".
	
Run "CMake (cmake-gui)":

* Set path to game engine source code;
* Set path where to build the binaries;
* Click on "configure" (perhaps you will click again to resolve all errors);
* Then Click on "Generate".

Open a console (cmd.exe), cd into your build directory and compile with:

	mingw32-make install-spring

When compiling ends, the game is installed into "C:\Program Files\Spring" (default). Into this directory create two subdirectories "maps" and "mods". Donwload following archive ([GamesAndMaps_3.zip](http://progandplay.lip6.fr/ressources/GamesAndMaps_3.zip)) and move all files included into "mods" directory into your "mods" directory and all files included into "maps" directory into your "maps" directory. Now you can play the game...

### Compiling on Linux and MacOSX

#### Get Dependencies

* Programs necessary to build
    * Xcode (Only for MacOSX)
    * cmake (tested with version 3.5.1)
    * 7zip (aka p7zip or 7z)
    * The usual build toolchain
        * gcc (MacOSX: comes with Xcode tools)
        * make (MacOSX: comes with Xcode tools)
* Libraries (install development packages)
    * SDL (tested with version 1.2)
    * Boost (tested with version 1.58)
    * DevIL (IL, ILU)
    * OpenAL
    * OpenGL headers (mesa, GLEW, etc. - MacOSX: comes with Xcode tools)
    * zlib
    * freetype (2)
    * ogg, vorbis and vorbisfile
    * Rapidxml (tested with v1.13)
    * Rapidjson (tested with v0.12)
* For IAs
    * python (2.5+)
    * jdk (1.5+ - pre-installed on OSX)

#### Build and install

	Last successful build on Linux: Ubuntu 16.04
	Last successful build on MacOSX: never tested

Default compiling process with CMake (2.6 or newer)

* Configure:
    * cmake .
* Install:
    * sudo make install-spring
* Default install paths are:
    * Spring executable: /usr/local/bin/spring
    * Read-only data: /usr/local/share/games/spring
    * If you want /usr prefix instead of /usr/local, configure like this:
        * cmake -DCMAKE_INSTALL_PREFIX=/usr .

Into your read-only data directory create two subdirectories "maps" and "mods". Donwload following archive ([GamesAndMaps_3.zip](http://progandplay.lip6.fr/ressources/GamesAndMaps_3.zip)) and move all files included into "mods" directory into your "mods" directory and all files included into "maps" directory into your "maps" directory. Now you can play the game...

