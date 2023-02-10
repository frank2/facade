```
 ▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▀▀▀▀▀▀▄▄▀▀▀▀▀▀▀▀▀▀▄
█ ▀▀▀███████ ▀▀▀███████ ▀▀▀███████ ▀▀▀███████ ▀▀▀██████▄ ▀▀▀███████ █
█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀    █▓█ ▀▀▀ ▄▄▄▄▄▄▄▀
█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀        ▀▀▀    █▓█ ▀▀▀    █▓█ ▀▀▀ █
█ ▀▀▀        ▀▀▀    █▒█ ▀▀▀        ▀▀▀    █▒█ ▀▀▀    █▒█ ▀▀▀ ▀▀▀▀▀▀▄
█ ▀▀▀█████▓░ ▀▀▀██▓░█▒█ ▀▀▀        ▀▀▀██▓░█▒█ ▀▀▀    █▒█ ▀▀▀█████▓░ █
█ ▀▀▀ ▄▄▄▄▄▄ ▀▀▀ ▄▄ █░█ ▀▀▀        ▀▀▀ ▄▄ █░█ ▀▀▀    █░█ ▀▀▀ ▄▄▄▄▄▄▀
█ ▀▀▀ █    █ ▀▀▀ ██ █░█ ▀▀▀        ▀▀▀ ██ █░█ ▀▀▀    █░█ ▀▀▀ █▄▄▄▄▄▄
█ ▀▀▀ █    █ ▀▀▀ ██ █ █ ▀▀▀▄▄▄▄▄▄▄ ▀▀▀ ██ █ █ ▀▀▀▄▄▄▄█ █ ▀▀▀▄▄▄▄▄▄▄ █
▀▄▀▀▀▄▀    ▀▄▀▀▀▄▀▀▄▀▀▀▄▀▀▀▀▀▀▀▀▀▀▄▀▀▀▄▀▀▄▀▀▀▄▀▀▀▀▀▀▀▀▀▄▄▀▀▀▀▀▀▀▀▀▀▄▀
  ▀▀▀        ▀▀▀    ▀▀▀ ▀▀▀▀▀▀▀▀▀▀ ▀▀▀    ▀▀▀ ▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀
```

**Facade** is a tool and library for embedding, extracting and detecting arbitrary payloads in PNG files. It is currently capable of adding arbitrary payloads to PNG files in the following ways:

* *Concatenation*: Data can be arbitrarily appended to the end of a PNG file without disrupting the image data. This is the "quick and dirty" solution to adding payload data to a given PNG image, although extraction of the data is just as easy as adding it without the use of this tool.
* *`tEXt` sections*: A feature of PNG files is **`tEXt` sections**, which is PNG metadata to add arbitrary text to images. Typically, this metadata includes information about, for example, the software used to create the image. *Facade* base64 encodes payloads with a given keyword into these sections in order to still meet the text requirement of these sections. This is ideal if you wish to stick the payload in the PNG chunk data, but can be obvious when viewed in a hex editor.
* *`zTXt` sections*: On top of a `tEXt` section, PNG images also feature **`zTXt` sections**, which are zlib-compressed `tEXt` sections. These, too, are base64-encoded before compression in order to conform to the text standard. This technique is a little less obvious, as the data is compressed and looks like any other binary data featured in the image (such as an `IDAT` section).
* *Steganography*: The final technique employed by *facade* is [steganography](https://en.wikipedia.org/wiki/Steganography) in the image data itself. Specifically, it uses the least-significant-bit technique across 4 bits of the color channels to encode arbitrary data into the image. This is much less obvious than the other techniques from a binary image standpoint, but might produce visible noise within your target image. Additionally, unlike the previous techniques, it's limited by the size of the image in pixels, and requires a specific pixel format to work. Luckily, RGB and RGBA are very standard pixel configurations for most PNG images!

On top of being able to embed these payloads, the console application is also capable of extracting and detecting these payloads within images.

This readme specifically covers the **binary** release of *facade*. For the **library** portion, see the [library readme](https://github.com/frank2/facade/blob/main/libfacade/README.md) or, better yet, the [library documentation](https://frank2.github.io/docs/libfacade).

## Acquiring

*Facade* uses the [argparse](https://github.com/p-ranav/argparse) library as a Git submodule, so it's not quite as straight-forward as cloning the repository, but it's still easy:

```
$ git clone https://github.com/frank2/facade.git
$ cd facade
$ git submodule update --init
```

This should be all you need to do to initialize the repository for the build step.

## Building

*Facade* has been compiled and tested against the following compilers:

* Microsoft Visual Studio 2019 (aka MSVC)
* gcc/g++ 10.2.1
* clang 9.0.1

Once you've completed the *acquire* step, install [CMake](https://cmake.org) for your platform, go to the root directory of the project and run the following commands:

```
$ mkdir build
$ cd build
$ cmake ../
$ cmake --build ./
```

You will most likely want to compile with optimizations to speed up the processing of PNG files. PNG data on large images is very processor-intensive when encoding images. For Windows, you can build a release build this way:

```
$ cmake --build --config Release
```

For Unix-based systems, it's a little more complicated, as you have to build the make system with the build type. Do it this way:

```
$ cmake -DCMAKE_BUILD_TYPE=Release ../
$ cmake --build ./
```

For Unix-based systems, you can add a final step once the tool has been built:

```
$ cmake --install ./
```

This will install *zlib* (if not present on the system already), *libfacade* and the *facade* binary. If you don't want it in the default directories, build the project with a prefix in mind. The default is `/usr/local`.

```
$ cmake -DCMAKE_INSTALL_PREFIX="/your/local/directory" ../
$ cmake --build ./
$ cmake --install ./
```

This will install the library and binary to `/your/local/directory/lib` and `/your/local/directory/bin` respectively.

For both operating systems, you can configure the binary and the library to used shared objects, which are DLLs on Windows.

```
$ cmake -DLIBFACADE_BUILD_SHARED=ON ../
$ cmake --build ./
```

## Using

Using the *facade* tool is pretty straight-forward. There are three main modules:

* create
* detect
* extract

For example, to create a steganographic payload within a PNG image, you can do this:

```
$ facade create -i image.png -o stego.png -s payload.bin
```

To see whether or not this stego payload is in the image, you can do this:

```
$ facade detect -s stego.png
```

And finally, to extract the stego payload from the image, you can do this:

```
$ facade extract -i stego.png -o ./extract-path -s
```

More detailed usage can be found by issuing the `--help` argument on each subcommand.
