# VoxelCraft - Minecraft clone on C++ with Vulkan

<img width="1199" height="899" alt="изображение" src="https://github.com/user-attachments/assets/787b0ebc-acf8-4f9a-af4d-d55c25ee8161" />

Now it's buggy some times, and have some issues with chunk generation (to generate, takes time about 2~3 ms).

## How to run:
Firstly, we need to compile program. For this, we need to install some Vulkan libraries:
(On Arch)
```
sudo pacman -S vulkan-validation-layers vulkan-devel vulkan-tools glm
```
(glm for math)

Also, we need GLFW for window
```
sudo pacman -S glfw
```

And, we need `clang` for compiling:
```
sudo pacman -S clang
```


And this all, now, we can compile VoxelCraft. In project root folder:
```
make run
```
This will automaticly compile and run the program.

By default, Makefile compile all in debug mode, but, you can change it. In Makefile
``` Makefile
all: release # insted of debug

debug: CXXFLAGS += -g
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)
```

This all for now! :D
