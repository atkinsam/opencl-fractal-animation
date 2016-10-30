//  julia_set.cpp
//
//  Author: Sam Atkinson
//  Date modified: Oct. 29 2016
//
//  Source code for julia set object functions


#include "julia_set.hpp"

Julia_Set::Julia_Set(size_t size, 
                     cl::ImageFormat* format,
                     cl::Context* context)
{
    // Initialize a few object variables
    _size = size;
    _origin[0] = 0;
    _origin[1] = 0;
    _origin[2] = 0;
    _region[0] = _size;
    _region[1] = _size;
    _region[2] = 1;
    // Allocated memory for resulting image after OpenCL kernel execution
    _result = new uint8_t[_size * _size * 4];
    // Create a blank OpenCL image
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


Julia_Set::~Julia_Set(void)
{
    // Deallocate memory for resulting image on host memeory
    delete _result;
}


void Julia_Set::fill_white(cl::CommandQueue* queue)
{
    // Fill image entirely with white
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
    // Set arguments for kernel specific to this julia set 
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
    // Add julia set kernel to queue to start computation
    cl_int err = queue->enqueueNDRangeKernel(_render_kernel,
                                             cl::NullRange,
                                             cl::NDRange(_size, _size),
                                             cl::NullRange,
                                             NULL,
                                             NULL);
    if (err != CL_SUCCESS)
        std::cerr << "Could not add kernel to queue" << std::endl;
}


void Julia_Set::read_image_to_host(cl::CommandQueue* queue)
{
    // Read OpenCL image from device memory to host memory, 
    //   into the array allocated in the constructor
    cl_int err = queue->enqueueReadImage(_image, CL_TRUE, _origin, _region,
                                         0, 0, _result, NULL, NULL);
    if (err != CL_SUCCESS)
        std::cerr << "Could not read image to host" << std::endl;
}


void Julia_Set::export_to_png(std::string filename)
{
    // Use the lodepng library to encode and export the resulting julia
    //   set image to a PNG file
    std::vector<unsigned char> output;
    output.resize(_size * _size * 4);
    for (unsigned int i = 0; i < _size * _size * 4; i++)
        output[i] = (unsigned char)_result[i];
    unsigned image_error = lodepng::encode(filename, output, _size, _size);
    if (image_error)
        std::cout << "Image encoding error: "
                  << lodepng_error_text(image_error) << std::endl;
}


void Julia_Set::export_to_ppm(std::string filename)
{
    // Write the raw RGB image to a PPM file and export to the disk
    // (this function executes significantly faster than export_to_png)
    std::ofstream ofs;
    ofs.open(filename.c_str(), std::ios::binary);
    ofs << "P6\n" << _size << " " << _size << "\n255\n";
    unsigned char r, g, b, a;
    for (unsigned int i = 0; i < _size * _size * 4; i+=4)
    {
        r = static_cast<unsigned char>(_result[i+0]);
        g = static_cast<unsigned char>(_result[i+1]);
        b = static_cast<unsigned char>(_result[i+2]);
        a = static_cast<unsigned char>(_result[i+3]);
        ofs << r << g << b;
    }
    ofs.close();
}
