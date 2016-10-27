/*  julia_set.hpp
 *
 *  Author: Sam Atkinson
 *  Date modified: Oct. 26 2016
 */

#ifndef JULIA_SET_H
#define JULIA_SET_H

#include <iostream>
#include "lodepng.h"
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

class Julia_Set
{
    public:
        Julia_Set(void) {};
        Julia_Set(size_t size, 
                  cl::ImageFormat* format,
                  cl::Context* context);
        void fill_white(cl::CommandQueue* queue);
        void create_kernel(cl::Program* program, 
                           std::string function_name,
                           cl::Buffer* buffer_re,
                           cl::Buffer* buffer_im,
                           cl::Buffer* cmap_buf,
                           unsigned int cmap_size,
                           float c_re,
                           float c_im);
        void queue_kernel(cl::CommandQueue* queue);
        void read_image_to_host(cl::CommandQueue* queue);
        void export_to_png(std::string filename);
    private:
        cl_int _err;
        size_t _size;
        float _c_re;
        float _c_im;
        cl::Kernel _render_kernel;
        cl::Buffer* _buffer_re;
        cl::Buffer* _buffer_im;
        cl::Buffer* _cmap_buf;
        unsigned int _cmap_size;
        cl::Image2D _image;
        cl::Context* context;
        cl::size_t<3> _origin;
        cl::size_t<3> _region;
        uint8_t* _result;
};


#endif /* JULIA_SET_H */
