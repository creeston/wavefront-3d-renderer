# Overview

Simple GUI app for visualing 3d objects in .obj format.

# Running

## Dependencies

### Ubuntu
```sh
sudo apt-get install libgtk-3-dev
sudo apt-get install glade
```

### Mac (Arm64)

brew install cmake
brew instal pkg-config
brew install gtk+3

## Build

1. Using make

make all

2. Using cmake

mkdir build
cd build
cmake ..
make

## Project structure

### gtk_gui.c 
> Contains logic for initialization gtk window and callbacks. Rotating, scaling, button clicking - all implemented there.

### canvas_drawing.c
> Implemenets drawing functions (line, single pixel and etc.), as a drawing surface uses GdkPixBuff
> After drawing on buffer, it will be placed at the cavnas, using draw_cb callback from gtk_gui.c

### renderer.c
> Implements interactions with .obj files: reading, drawing, rotating, scaling. 
> It is better NOT TO MODIFY anything there. 

### renderer.glade
> xml file, generated by glade tool. Gtk uses it as markup for it's gui.
> Use glade to modify it and add new widgets.
