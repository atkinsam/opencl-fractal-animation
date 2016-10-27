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
#include <sys/types.h>
#include <sys/stat.h>
#include "lodepng.h"
#include "opencl_errors.hpp"
#include "julia_set.hpp"


cl_uint4* colormap(std::string filename, unsigned int* size);
cl::Platform get_platform(int* m_err);
cl::Device get_device(cl::Platform* platform, int* m_err);
cl::Context get_context(cl::Device* device, int* m_err);
void check_device_info(cl::Device* device, int* m_err);
void cleanup();


int main(int argc, char* argv[])
{
    /* OpenCL variables */
    cl_int err = CL_SUCCESS;
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::CommandQueue queue;
    /* main error */
    int m_err = 0;
    /* get OpenCL platform layer */
    platform = get_platform(&m_err);
    if (err != 0)
    {
        cleanup();
        return EXIT_FAILURE;
    }
    device = get_device(&platform, &m_err);
    if (err != 0)
    {
        cleanup();
        return EXIT_FAILURE;
    }
    context = get_context(&device, &m_err);
    if (err != 0)
    {
        cleanup();
        return EXIT_FAILURE;
    }
    queue = cl::CommandQueue(context, device);
    /* check device info for image support and output dimensions */
    check_device_info(&device, &m_err);
    /* tell the user we are starting execution */
    std::cout << "STARTING EXECUTION" << std::endl;
    /* choose parameters for fractal */
    size_t size = 1000;
    unsigned int num_frames = 100;
    float center_re = 0.0;
    float center_im = 0.0;
    float zoom = 1.0;
    float c_re = 0.0;
    float c_im = 0.62;
    float c_re_step = 0.0;
    float c_im_step = 0.0005;
    /* initialize image as type RGBA with each element having size 8 bytes */ 
    cl::ImageFormat image_format(CL_RGBA, CL_UNSIGNED_INT8);

    /* initialize julia sets and fill with white */
    std::vector<Julia_Set> frames;
    frames.resize(num_frames);
    for (unsigned int i = 0; i < num_frames; i++)
    {
        frames[i] = Julia_Set(size, &image_format, &context);
        frames[i].fill_white(&queue);
    }
    std::cout << "Successfully initalized images and filled with white"
              << std::endl;


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
        std::cout << "Successfully built OpenCL program from source"
                  << std::endl;
    }

    /* create colormap */
    unsigned int cmap_size;
    cl_uint4* cmap = colormap("colormaps/cool.png", &cmap_size);
    cl::Buffer cmap_buf(context, 
                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        sizeof(cl_uint4) * cmap_size,
                        cmap,
                        &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not create colormap buffer" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Successfully created colormap buffer" << std::endl;

    cl::Buffer buffer_re(context, CL_MEM_READ_WRITE, sizeof(float) * size);
    cl::Buffer buffer_im(context, CL_MEM_READ_WRITE, sizeof(float) * size);

    /* create kernels for julia sets */
    for (unsigned int i = 0; i < num_frames; i++)
        frames[i].create_kernel(&program,
                                "render_image",
                                &buffer_re,
                                &buffer_im,
                                &cmap_buf,
                                cmap_size,
                                c_re + i * c_re_step,
                                c_im + i * c_im_step);
    std::cout << "Successfully created kernels for julia set computation"
              << std::endl;
    
    /* create kernels for evely spaced real and imaginary values */    
    cl::Kernel spaced_re_kernel(program, "even_re");
    spaced_re_kernel.setArg(0, center_re);
    spaced_re_kernel.setArg(1, zoom);
    spaced_re_kernel.setArg(2, (float)size);
    spaced_re_kernel.setArg(3, buffer_re);
    cl::Kernel spaced_im_kernel(program, "even_im");
    spaced_im_kernel.setArg(0, center_im);
    spaced_im_kernel.setArg(1, zoom);
    spaced_im_kernel.setArg(2, (float)size);
    spaced_im_kernel.setArg(3, buffer_im);
    std::cout << "Successfully created kernels for real and imaginary values"
              << std::endl;

    /* add real and imaginary value kernels to queue and finish */
    err = queue.enqueueTask(spaced_re_kernel, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not compute spaced real values" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Successfully computed evenly spaced real values" 
              << std::endl;
    err = queue.enqueueTask(spaced_im_kernel, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not compute spaced imaginary values" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Successfully computed evenly spaced imaginary values"
              << std::endl;
    std::cout << "Waiting for queue to finish before starting render..." 
              << std::endl;
    err = queue.finish();
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not finish queue before render" << std::endl;
        return EXIT_FAILURE;
    }

    /* add julia set kernels to queue */
    for (unsigned int i = 0; i < num_frames; i++)
        frames[i].queue_kernel(&queue);
    std::cout << "Successfully added julia set kernels to queue" << std::endl;
    std::cout << "Waiting for julia sets to render..." << std::endl;
    err = queue.finish();
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not finish rendering" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Successfully computed julia sets" << std::endl;

    /* read julia sets to host memory */
    for (unsigned int i = 0; i < num_frames; i++)
        frames[i].read_image_to_host(&queue);
    std::cout << "Waiting for images to be read to host memory" << std::endl;
    err = queue.finish();
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not finish reading to host memory" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Finished reading images to host memory" << std::endl;

    /* create directory to hold finished PNG images */
    struct stat st = {0};
    if (stat("./frames/", &st) == -1)
        mkdir("./frames/", 0700);

    /* create buffer for output file names */
    char png_buf[100];
    
    /* export julia set images to PNG files */
    std::cout << "Waiting for images to be encoded to PNG..." << std::endl;
    for (unsigned int i = 0; i < num_frames; i++)
    {
        snprintf(png_buf, sizeof(png_buf), "./frames/F%04d.png", i);
        frames[i].export_to_png(png_buf);
    }
    err = queue.finish();
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not finish encoding images" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Finished encoding and exporting images to PNG" << std::endl;

    /* unload resources */
    platform.unloadCompiler();

    /* output to MP4 */
    std::string mp4_system_call = "ffmpeg -f image2 -r 30 -i ";
    mp4_system_call += "frames/F%04d.png -vcodec mpeg4 -qscale 1 -y ";
    mp4_system_call += "out.mp4";
    system(mp4_system_call.c_str());

    return EXIT_SUCCESS;
}


cl::Platform get_platform(int* m_err)
{
    /* get all available OpenCL platforms */
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
    {
        std::cerr << "No OpenCL platforms found... exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    /* print number of available platforms to stdout */
    else if (all_platforms.size() > 1)
        std::cout << "Found " << all_platforms.size() << " available"
                  << " OpenCL platforms" << std::endl;
    else
        std::cout << "Found 1 available OpenCL platform" << std::endl;
    cl::Platform platform = all_platforms[0];
    std::cout << "\tUsing platform " << platform.getInfo<CL_PLATFORM_NAME>()
              << std::endl;
    return platform;
}


cl::Device get_device(cl::Platform* platform, int* m_err)
{
    std::vector<cl::Device> all_devices;
    platform->getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0)
    {
        std::cerr << "No OpenCL devices found... exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    /* let user choose OpenCL device if more than 1 is available */
    unsigned int device_num = 0;
    if (all_devices.size() > 1)
    {
        std::cout << "Available OpenCL devices:" << std::endl;
        for (unsigned int i = 0; i < all_devices.size(); i++)
        {
            std::cout << "\t[" << i << "] "
                      << all_devices[i].getInfo<CL_DEVICE_NAME>()
                      << std::endl;
        }
        std::cout << "Device preference: ";
        std::cin >> device_num;
    }
    else
    {
        std::cout << "Found 1 available OpenCL device" << std::endl;
    }
    return all_devices[device_num]; 
}


cl::Context get_context(cl::Device* device, int* m_err)
{
    return cl::Context({*device});
}


void check_device_info(cl::Device* device, int* m_err)
{
    /* make sure device has image support */
    if (!device->getInfo<CL_DEVICE_IMAGE_SUPPORT>())
    {
        std::cerr << "OpenCL device does not have image support" << std::endl;
        *m_err = 1;
    }
    /* output device into to stdout */
    std::cout << "DEVICE INFO:" << std::endl;
    cl_uint max_work_item_dims;
    max_work_item_dims = device->getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
    std::cout << "\tMaximum work item dimensions: " << max_work_item_dims 
              << std::endl;
    std::vector<long unsigned int> max_work_item_sizes;
    max_work_item_sizes = device->getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    for (cl_uint i = 0; i < max_work_item_dims; i++)
        std::cout << "\t\tMaximum work item size for dimension " << i 
                  << ": " << max_work_item_sizes[i] << std::endl;
    cl_uint max_work_group_size;
    max_work_group_size = device->getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    std::cout << "\tMaximum work group size: " << max_work_group_size
              << std::endl;
}


cl_uint4* colormap(std::string filename, unsigned int* size)
{
    std::vector<unsigned char> image;
    unsigned int width, height;
    lodepng::decode(image, width, height, filename.c_str());
    cl_uint4* cmap = new cl_uint4[width]; 
    for (unsigned int i = 0; i < width; i++)
    {
        cmap[i] = {{image[i * 4 + 0],
                    image[i * 4 + 1],
                    image[i * 4 + 2],
                    255}};
    }
    *size = width;
    return cmap;
}


void cleanup()
{

}
