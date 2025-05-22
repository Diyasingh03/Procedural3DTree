# A tree generator in OpenGL using Lindenmayer systems and turtle graphics

Created by Denny Lindberg for the course TNM084 and modified by Diya Singh for CS334


# Features

- C++17
- For Windows (Visual Studio)
- OpenGL 3.3

- Full scene with camera turntable
- Generates trees with continuous branches
- Plenty of leaves
- Flowers
- Customisation to the tree elements
- Only procedural content, no existing textures
- Screenshot the tree at any moment

# Building
Solution placed in builds/

# Third party libraries used

**glad** for OpenGL bindings - https://github.com/Dav1dde/glad

**SDL2** for creating a cross-platform OpenGL window with input - https://www.libsdl.org

**glm** for vector and matrix data types compatible with OpenGL - https://glm.g-truc.net/0.9.9/index.html

**lodepng** for loading and saving PNG files - https://lodev.org/lodepng/

**tangrams glmTool** for generating hull around vertices - https://github.com/tangrams/glmTools

**imgui** for user interaction - 


# Algorithms copied or referenced
**Various GLSL noise algorithms** - Copied: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83

**Bresenham line drawing algorithm** - Referenced: https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C.2B.2B

**Evan's grid shader** - Slightly modified: http://madebyevan.com/shaders/grid/

**Denny Lindbergg's Tree Generator** - Modified: https://github.com/DennyLindberg/Tree-Generator


## Folder structure

**binaries/** - contains compiled executable, dlls, images, configs or audio. (screenshots end up here)

**content/** - shader files.

**include/** - thirdparty includes.

**libs/** - windows specific libs.

**source/** - main folder for source code.

**builds/** - this folder contains the solution. This folder can be deleted at any time.
