# SpriteExtractor
When I'm prototyping games, I avoid spending time on art. I do this by grabbing temporary art assets from the internet.
However, many of them have inconsistent spacing/padding and are a pain to work with in my engine. This tool fixes that.

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

## Usage
```
sprite_extractor (path to input image) (path to output image) (desired frame width) (desired frame height) (edge distance threshold in pixels)
```

The edge distance threshold is used to determine whether disconnected pixels still belong to a frame. If the distance from these pixels
to the nearest edge is less than or equal to the threshold, then they're incorporated.

## Examples

I turned this

![Alt text](example_images/enemies_zelda_src.png?raw=true "Zelda Enemies")

into this

![Alt text](example_images/enemies_zelda.png?raw=true "Zelda Enemies Cleaned Up")

by doing

```
sprite_extractor example_images/enemies_zelda_src.png example_images/enemies_zelda.png 32 32 5
```

And this

![Alt text](example_images/player_zelda_src.png?raw=true "Zelda Player")

into this

![Alt text](example_images/player_zelda.png?raw=true "Zelda Player Cleaned Up")

by doing

```
sprite_extractor.exe example_images\player_zelda_src.png example_images\player_zelda.png 64 64 2
```

Notice how the "wave arcs" are placed into single frames despite them being separated from each other by a certain amount.
This occurs because of the edge distance threshold.
