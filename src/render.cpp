/*  render_mp4.cpp
 *
 *  Author: Sam Atkinson
 *  Date modified: 09-Oct-2016
 *
 *  Renders an animated mp4 of a julia fractal transformation
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "lodepng.h"
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif


int main(int argc, char* argv[])
{
    /* OpenCL variables */
    cl_int err = CL_SUCCESS;


    /* size of image */
    size_t size = 8194;

    /* get available OpenCL platforms */
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
    {
        std::cerr << "No OpenCL platforms found.";
        return EXIT_FAILURE;
    }
    else if (all_platforms.size() > 1)
    {
        std::cout << "Found " << all_platforms.size() << " available";
        std::cout << " OpenCL platforms" << std::endl;
    }
    else
    {
        std::cout << "Found 1 available OpenCL platform" << std::endl;
    }
    cl::Platform platform = all_platforms[0];
    std::string platform_name = platform.getInfo<CL_PLATFORM_NAME>();
    std::cout << "\tUsing platform " << platform_name << std::endl;

    /* get available OpenCL devices */
    std::vector<cl::Device> all_devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0)
    {
        std::cerr << "No OpenCL devices found.";
        return EXIT_FAILURE;
    }

    /* let user choose OpenCL device */
    int device_num = 0;
    if (all_devices.size() > 1)
    {
        std::cout << "Available OpenCL devices:" << std::endl;
        for (unsigned int i = 0; i < all_devices.size(); i++)
        {
            std::string device_name;
            device_name = all_devices[i].getInfo<CL_DEVICE_NAME>();
            std::cout << "\t[" << i << "] " << device_name << std::endl;
        }
        std::cout << "Device preference: ";
        std::cin >> device_num;
    }
    else
    {
        std::cout << "Found 1 available OpenCL device" << std::endl;
    }
    cl::Device device = all_devices[device_num];
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "\tUsing device " << device_name << std::endl;

    /* create OpenCL context on device */
    cl::Context context({device});

    /* make sure device has OpenCL image support */
    if (!device.getInfo<CL_DEVICE_IMAGE_SUPPORT>())
    {
        std::cerr << "OpenCL device does not have image support";
        return EXIT_FAILURE;
    }

    std::cout << "DEVICE INFO:" << std::endl;

    /* check maximum work item dimensions */
    cl_uint max_work_item_dims;
    max_work_item_dims = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
    std::cout << "\tMaximum work item dimensions: " << max_work_item_dims;
    std::cout << std::endl;

    /* check maximum work item sizes */
    std::vector<long unsigned int> max_work_item_sizes;
    max_work_item_sizes = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    for (cl_uint i = 0; i < max_work_item_dims; i++)
    {
        std::cout << "\t\tMaximum work item size for dimension " << i;
        std::cout << ": " << max_work_item_sizes[i] << std::endl;
    }

    /* check maximum work group size */
    cl_uint max_work_group_size;
    max_work_group_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    std::cout << "\tMaximum work group size: " << max_work_group_size;
    std::cout << std::endl;

    /* tell the user we are starting execution */
    std::cout << "STARTING EXECUTION" << std::endl;

    /* choose parameters for fractal */
    static float frac_center_re = 0.0;
    static float frac_center_im = 0.0;
    static float frac_zoom = 1.0;
    static float frac_c_re = 0.0;
    static float frac_c_im = 0.68;

    /* create buffer for real values */
    float min_re = frac_center_re - (float)frac_zoom / 2.0;
    float max_re = frac_center_re + (float)frac_zoom / 2.0;
    float interval_re = (max_re - min_re) / (float)size;
    float spaced_re[size];
    for (unsigned int i = 0; i < size; i++)
        spaced_re[i] = min_re + i * interval_re;
    cl::Buffer buffer_re(context, 
                         CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         (size_t)size * sizeof(float),
                         spaced_re,
                         &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Did not successfully fill real buffer" << std::endl;
        return EXIT_FAILURE;
    }

    /* create buffer for imaginary values */
    float min_im = frac_center_im - (float)frac_zoom / 2.0;
    float max_im = frac_center_im + (float)frac_zoom / 2.0;
    float interval_im = (max_im - min_im) / (float)size;
    float spaced_im[size];
    for (unsigned int i = 0; i < size; i++)
        spaced_im[i] = min_im + i * interval_im;
    cl::Buffer buffer_im(context, 
                         CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         (size_t)size * sizeof(float),
                         spaced_im,
                         &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Did not successfully fill imaginary buffer" << std::endl;
        return EXIT_FAILURE;
    }
  
    /* initialize image as type RGBA with each element having size 8 bytes */ 
    cl::ImageFormat image_format(CL_RGBA, CL_UNSIGNED_INT8);
    cl::Image2D image(context,
                      CL_MEM_READ_WRITE,
                      image_format,
                      (size_t)size,
                      (size_t)size,
                      (size_t)0,
                      NULL,
                      &err);

    if (err != CL_SUCCESS)
    {
        std::cerr << "Did not successfully create image: " << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        size_t height, width;
        err = image.getImageInfo(CL_IMAGE_HEIGHT, &height);
        err = image.getImageInfo(CL_IMAGE_WIDTH, &width);
        std::cout << "Successfully created image with size: ";
        std::cout << height << "x" << width << std::endl;
    }

    /* read kernel source code from file */
    cl::Program::Sources sources;
    std::ifstream kernel_file("src/kernel.cl");
    std::stringstream buffer;
    buffer << kernel_file.rdbuf();
    std::string kernel_source = buffer.str();
    sources.push_back({kernel_source.c_str(), kernel_source.length()});
   
    /* build kernel program and output build errors to file */ 
    cl::Program program(context, sources);
    if (program.build({device}) != CL_SUCCESS)
    {
        std::string err_str;
        err_str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::ofstream build_log;
        build_log.open("kernel_build_log.txt", std::ios::out);
        build_log << err_str;
        build_log.close();
        std::cerr << "Error building kernel. See file kernel_build_log.txt" <<
                     std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        std::cout << "Successfully built kernel" << std::endl;
    }

    cl_uint4 color_init = {{255, 255, 255, 255}};

    /* create kernel from program */
    cl::Kernel kernel(program, "render_image");
    kernel.setArg(0, image);
    kernel.setArg(1, buffer_re);
    kernel.setArg(2, buffer_im);
    kernel.setArg(3, frac_c_re);
    kernel.setArg(4, frac_c_im);

    /* create command queue */
    cl::CommandQueue queue(context, device);

    /* create origin and region locations */
    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = size;
    region[1] = size;
    region[2] = 1;

    /* write initial color to image */
    err = queue.enqueueFillImage(image, color_init, origin, region);

    /* add kernel to queue */
    err = queue.enqueueNDRangeKernel(kernel, 
                                     cl::NullRange, 
                                     cl::NDRange(size, size), 
                                     cl::NullRange,
                                     NULL,
                                     NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not execute kernel: " << std::endl;
        return EXIT_FAILURE;
    }
   
    /* make sure all queued operations are finished before reading image */ 
    err = queue.finish();
    if (err != CL_SUCCESS)
    {
        std::cerr << "Queue could not finish execution" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Finished rendering" << std::endl;
   
    /* read finished image from device memory */ 
    uint8_t* result = new uint8_t[size * size * 4];
    err = queue.enqueueReadImage(image, CL_TRUE, origin, region, 0, 0, result,
                           NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "OpenCL enqueueReadImage call failed: " << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        std::cout << "Successfully read image to host memory" << std::endl;
    }
    
    /* convert from array to vector for lodepng */
    std::vector<unsigned char> output;
    output.resize(size * size * 4);
    for (unsigned int i = 0; i < size * size * 4; i++)
    {
        output[i] = (unsigned char)result[i];
    }

    /* export image to png */
    unsigned image_error = lodepng::encode("test.png", output, size, size);
    if (image_error)
    {
        std::cout << "Image encoding error: " << lodepng_error_text(image_error);
        std::cout << std::endl;
        return EXIT_SUCCESS;
    }

    /* unload resources */
    platform.unloadCompiler();

    return EXIT_SUCCESS;
}
