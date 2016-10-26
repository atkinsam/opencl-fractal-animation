/*
 *
 *
 *
 */

#include "julia_set.hpp"

Julia_Set::Julia_Set(size_t size, 
                     cl::ImageFormat* format,
                     cl::Context* context)
{
    _size = size;
    _image = cl::Image2D(*context,
                         CL_MEM_READ_WRITE,
                         *format,
                         (size_t)_size,
                         (size_t)_size,
                         (size_t)0,
                         NULL, 
                         &_err);
    if (_err != CL_SUCCESS)
    {
        std::cerr << "Could not create OpenCL image" << std::endl;
    }
}

