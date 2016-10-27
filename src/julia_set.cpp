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
    _origin[0] = 0;
    _origin[1] = 0;
    _origin[2] = 0;
    _region[0] = _size;
    _region[1] = _size;
    _region[2] = 1;
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


void Julia_Set::fill_white(cl::CommandQueue* queue)
{
    cl_uint4 color_init = {{255, 255, 255, 255}};
    cl_int err = queue->enqueueFillImage(_image, color_init, _origin, _region);
    if (err != CL_SUCCESS)
        std::cerr << "Could not fill image" << std::endl;
}


void Julia_Set::create_kernel(cl::Program* program,
                              std::string function_name,
                              cl::Buffer* buffer_re,
                              cl::Buffer* buffer_im,
                              cl::Buffer* cmap_buf,
                              unsigned int cmap_size,
                              float c_re,
                              float c_im)
{
    _render_kernel = cl::Kernel(*program, function_name.c_str());
    _render_kernel.setArg(0, _image);
    _render_kernel.setArg(1, *buffer_re);
    _render_kernel.setArg(2, *buffer_im);
    _render_kernel.setArg(3, *cmap_buf);
    _render_kernel.setArg(4, cmap_size);
    _render_kernel.setArg(5, c_re);
    _render_kernel.setArg(6, c_im);
}


void Julia_Set::queue_kernel(cl::CommandQueue* queue)
{
    cl_int err = queue->enqueueNDRangeKernel(_render_kernel,
                                             cl::NullRange,
                                             cl::NDRange(_size, _size),
                                             cl::NullRange,
                                             NULL,
                                             NULL);
    if (err != CL_SUCCESS)
        std::cerr << "Could not add kernel to queue" << std::endl;
}
