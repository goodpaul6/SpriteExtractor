# SpriteExtractor
When I'm prototyping games, I avoid spending time on art. I do this by grabbing temporary art assets from the internet.
However, many of them have inconsistent spacing/padding and are a pain to work with in my engine. This tool fixes that.

## Downloads
Pre-built binaries are located in the downloads folder.

## Features
* Optionally pack sprites tightly and generate appropriate metadata
* Label each frame with its index for easy visual lookup
* Process entire directories of images (recursively) all at once
* Absolutely no dependencies
* Only processes/generates 4-component (RGBA) png files. (Yes, this is a feature)

## Build
You can use CMake to build this with whatever you want, which means you'll need CMake.

If you're on windows and have Visual Studio 2017 installed:

```
mkdir bin
cd bin
cmake -G "Visual Studio 15 Win64" ..
```
will make a 64-bit project which you can open up in VS and build.

If you're somewhere else and you want a makefile for a debug build, for example:

```
mkdir bin
cd bin
mkdir Debug
cd Debug
cmake -G "Makefile" ../.. -DCMAKE_BUILD_TYPE=Debug
```

will make a makefile in that directory to build a debug version of the app.

## Usage
```
sprite_extractor -h
```
Should give you all the usage info.
Do note that it samples the top-left pixel of the image to determine the "background" color.

## Examples

I turned this

![Alt text](example_images/enemies_zelda_src.png?raw=true "Zelda Enemies")

into this

![Alt text](example_images/enemies_zelda.png?raw=true "Zelda Enemies Cleaned Up")

by doing

```
sprite_extractor example_images/enemies_zelda_src.png example_images/enemies_zelda.png --frame-width 32 --frame-height 32 -e 5 --dest-width 512 --row-thresh 16 --min-width 4 --min-height 4
```

And this

![Alt text](example_images/player_zelda_src.png?raw=true "Zelda Player")

into this

![Alt text](example_images/player_zelda_packed.png?raw=true "Zelda Player Cleaned Up")

by doing

```
sprite_extractor example_images/player_zelda_src.png example_images/player_zelda_packed.png --pack 256 128 --frame-width 64 --frame-height 64 -e 2 --min-width 4 --min-height 4 --label --row-thresh 4
```

Notice how the "wave arcs" are placed into single frames despite them being separated from each other by a certain amount.
This occurs because of the edge distance threshold.

I was also able to take these images (see `example_images/npcs`)

![Alt text](example_images/npcs/oldman/1.png?raw=true "Zelda NPC")
![Alt text](example_images/npcs/2.png?raw=true "Zelda NPC")
![Alt text](example_images/npcs/3.png?raw=true "Zelda NPC")
![Alt text](example_images/npcs/oldman/4.png?raw=true "Zelda NPC")
![Alt text](example_images/npcs/5.png?raw=true "Zelda NPC")

And make

![Alt text](example_images/npcs_zelda.png?raw=true "Zelda NPC")

by doing

```
sprite_extractor --dir example_images/npcs example_images/npcs_zelda.png --dest-width 128 --frame-width 32 --frame-height 32 -e 0 --min-width 4 --min-height 4
```

Notice how some images contained multiple frames and others just one. This is fine, it still detects them as multiple frames even if they're all in the same file, just like it did for the single image examples.
