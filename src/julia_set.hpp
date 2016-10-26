/*  julia_set.hpp
 *
 *  Author: Sam Atkinson
 *  Date modified: Oct. 26 2016
 */

#ifndef JULIA_SET_H
#define JULIA_SET_H

#include <iostream>
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

class Julia_Set
{
    public:
        Julia_Set(size_t size, 
                  cl::ImageFormat* format,
                  cl::Context* context);
    private:
        cl_int _err;
        size_t _size;
        float _c_re;
        float _c_im;
        cl::Kernel* _render_kernel;
        cl::Buffer* _buffer_re;
        cl::Buffer* _buffer_im;
        cl::Buffer* _cmap_buf;
        unsigned int _cmap_size;
        cl::Image2D _image;
        cl::Context* context;
};


#endif /* JULIA_SET_H */
