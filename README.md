# opencl-fractal-animation
Fractal animation using C++ and the OpenCL framework

### Dependencies

* ffmpeg
* OpenCL framework
* OpenCL C++ wrapper (cl.hpp)
* make
* g++

### Compiling/running this program:
    $ git clone https://github.com/atkinsam/opencl-fractal-animation.git
    $ cd opencl-fractal-animation
    $ make -C src/
    $ ./render.o <video size in px> <colormap png file>

### Example
    $ ./render.o 500 colormaps/ocean.png
Outputs a 500x500-pixel 60-FPS .mp4 video colored using the colormaps/ocean.png image

### Changing animation parameters

See src/render.cpp lines 46-53:

| Parameter    | Property                                     |
|--------------|----------------------------------------------|
| `num_frames` | Changes number of frames in the 60 FPS video |
| `center_re`  | Pan video left/right                         |
| `center_im`  | Pan video up/down                            |
| `zoom`       | Zoom in on video                             |
| `c_re`       | Starting real part of complex number C       |
| `c_im`       | Starting imaginary part of complex number C  |
| `c_re_step`  | Step per frame for real part of C            |
| `c_im_step`  | Step per frame for imaginary part of C       |
