# Screenshot
![Screenshot](/screenshots/screenshot-2023-04-09.png)

# Description
Camera rotating around a splatmap terrain rendered with OpenGL.

# How to run
```console
# clone repo with its submodules
$ git clone --recursive https://github.com/OpenGL-Graphics/terrain

# build & run
$ cd terrain
$ mkdir build && cd build
$ cmake .. && make -j && ./main

# to get new commits from submodules
$ git submodule update --remote
```
