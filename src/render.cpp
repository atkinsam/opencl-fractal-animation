//  render_mp4.cpp
//
//  Author: Sam Atkinson
//  Date modified: 28-Oct-2016
//
//  Renders an animated mp4 of a julia fractal transformation


#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <iomanip>
#include <cstdlib>
#include "lodepng.h"
#include "opencl_errors.hpp"
#include "julia_set.hpp"

// Function prototypes
cl_uint4* colormap(std::string filename, unsigned int* size);
cl::Platform get_platform(void);
cl::Device get_device(cl::Platform* platform);
cl::Context get_context(cl::Device* device);
cl::Program build_program(std::string source_file, cl::Context* context,
                          cl::Device* device);
void check_device_info(cl::Device* device);
std::string ts(time_t* start_time);


int main(int argc, char** argv)
{
    // Parse arguments
    if (argc != 3)
    {
        std::cerr << "Error: Incomplete arguments" << std::endl
                  << "Usage: ./render.o <video size in px> <colormap png>"
                  << std::endl;
        return EXIT_FAILURE;
    }
    size_t size = (size_t)atoi(argv[1]);
    std::string cmap_filename = argv[2];
   
    // Define parameters for fractal animation 
    unsigned int num_frames = 600;
    float center_re = 0.0;
    float center_im = 0.0;
    float zoom = 1.0;
    float c_re = 0.0;
    float c_im = 0.635;
    float c_re_step = 0.0;
    float c_im_step = 0.00002;

    // Declare OpenCL objects
    cl_int err = CL_SUCCESS;
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::CommandQueue queue;

    // Initialize OpenCL platform layer
    // ===============================================================
    // Create OpenCL platform
    platform = get_platform();
    // Create OpenCL device
    device = get_device(&platform);
    // Create OpenCL context 
    context = get_context(&device);
    // Create OpenCL command queue
    queue = cl::CommandQueue(context, device);
    // Check device information for image support and dimensions
    check_device_info(&device);

    // Create buffers
    // ===============================================================
    // Create buffers for real and imaginary values
    cl::Buffer buffer_re(context, CL_MEM_READ_WRITE, sizeof(float) * size);
    cl::Buffer buffer_im(context, CL_MEM_READ_WRITE, sizeof(float) * size);
    // Create buffer for colormap and fill with generated colormap
    unsigned int cmap_size;
    cl_uint4* cmap = colormap(cmap_filename, &cmap_size);
    if (cmap == NULL) return EXIT_FAILURE;
    cl::Buffer cmap_buf(context, 
                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        sizeof(cl_uint4) * cmap_size,
                        cmap,
                        &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not create colormap buffer: " << get_err_str(err)
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize Julia Set objects and fill images with white
    // ===============================================================
    // Declare image format as RGBA with 1 byte per color element
    cl::ImageFormat image_format(CL_RGBA, CL_UNSIGNED_INT8);
    std::vector<Julia_Set> frames;
    frames.resize(num_frames);
    for (unsigned int i = 0; i < num_frames; i++)
    {
        frames[i] = Julia_Set(size, &image_format, &context);
        frames[i].fill_white(&queue);
    }

    // Create kernels
    // ===============================================================
    // Build kernel program from source
    cl::Program program = build_program("src/kernel.cl", &context, &device);
    // Create kernels for julia set objects 
    for (unsigned int i = 0; i < num_frames; i++)
        frames[i].create_kernel(&program,
                                "render_image",
                                &buffer_re,
                                &buffer_im,
                                &cmap_buf,
                                cmap_size,
                                c_re + i * c_re_step,
                                c_im + i * c_im_step);
    // Create kernels for real & imaginary value buffers
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
   
     
    // Start OpenCL operations
    // ===============================================================
    
    // Start execution timer
    std::cout << "STARTING EXECUTION" << std::endl;
    time_t t_s = time(0);
   
    // Compute evenly spaced real and imaginary values
    // ===============================================================
    err = queue.enqueueTask(spaced_re_kernel, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not compute spaced real values: " 
                  << get_err_str(err) << std::endl;
        return EXIT_FAILURE;
    }
    err = queue.enqueueTask(spaced_im_kernel, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not compute spaced imaginary values: " 
                  << get_err_str(err) << std::endl;
        return EXIT_FAILURE;
    }
    err = queue.finish();
    std::cout << ts(&t_s) << "Computed evenly spaced real and imaginary values"
              << std::endl;
    
    // Compute julia sets
    // ===============================================================
    for (unsigned int i = 0; i < num_frames; i++)
        frames[i].queue_kernel(&queue);
    err = queue.finish();
    std::cout << ts(&t_s) << "Computed julia sets" << std::endl;

    if (err != CL_SUCCESS)
    {
        std::cerr << "Could not finish rendering: " << get_err_str(err)
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Read julia sets to host memory and export
    // ===============================================================
    for (unsigned int i = 0; i < num_frames; i++)
        frames[i].read_image_to_host(&queue);
    err = queue.finish();
    std::cout << ts(&t_s) << "Finished reading julia sets to host memory" 
              << std::endl;
    // Create directory to hold rendered frames
    struct stat st = {0};
    if (stat("./frames/", &st) == -1)
        mkdir("./frames/", 0700);
    // Export julia set images to PPM files 
    char ppm_buf[100];
    for (unsigned int i = 0; i < num_frames; i++)
    {
        snprintf(ppm_buf, sizeof(ppm_buf), "./frames/F%04d.ppm", i);
        frames[i].export_to_ppm(ppm_buf);
    }
    err = queue.finish();
    std::cout << ts(&t_s) << "Finished exporting julia sets to PPM files" 
              << std::endl;

    // Cleanup resources
    // ===============================================================
    platform.unloadCompiler();

    // Create MP4 video from PPM image frames
    // ===============================================================
    std::string mp4_system_call = "ffmpeg -f image2 -r 60 -i ";
    mp4_system_call += "frames/F%04d.ppm -vcodec mpeg4 -q:v 20 -c:v libx264 ";
    mp4_system_call += "-y out.mp4";
    system(mp4_system_call.c_str());

    return EXIT_SUCCESS;
}


cl::Platform get_platform(void)
{
    // Get all available OpenCL platforms
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
    {
        std::cerr << "No OpenCL platforms found... exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Print number of available platforms to stdout
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


cl::Device get_device(cl::Platform* platform)
{
    // Get list of all OpenCL devices on platform
    std::vector<cl::Device> all_devices;
    platform->getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0)
    {
        std::cerr << "No OpenCL devices found... exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Let user choose OpenCL device if more than 1 is avialable
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
    std::cout << "\tUsing device " 
              << all_devices[device_num].getInfo<CL_DEVICE_NAME>()
              << std::endl;
    return all_devices[device_num]; 
}


cl::Context get_context(cl::Device* device)
{
    return cl::Context({*device});
}


cl::Program build_program(std::string source_file, cl::Context* context,
                          cl::Device* device)
{
    // Get text from kernel file 
    cl::Program::Sources sources;
    std::ifstream kernel_file(source_file);
    std::stringstream buffer;
    buffer << kernel_file.rdbuf();
    std::string kernel_source = buffer.str();
    sources.push_back({kernel_source.c_str(), kernel_source.length()});
    
    // Build program from kernel source code
    cl::Program program(*context, sources);
    if (program.build({*device}) != CL_SUCCESS)
    {
        // Output build errors to file
        std::string err_str;
        err_str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device);
        std::ofstream build_log;
        build_log.open("kernel_build_log.txt", std::ios::out);
        build_log << err_str;
        build_log.close();
        std::cerr << "Error building kernel. See file kernel_build_log.txt" <<
                     std::endl;
    }
    return program;
}


void check_device_info(cl::Device* device)
{
    // Make sure device has image support
    if (!device->getInfo<CL_DEVICE_IMAGE_SUPPORT>())
    {
        std::cerr << "OpenCL device does not have image support" << std::endl;
    }
    // Output device info to stdout
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
    // Use lodepng library to decode png colormap
    unsigned err  = lodepng::decode(image, width, height, filename.c_str());
    if (err)
    {
        std::cerr << "Invalid colormap PNG file" << std::endl;
        return NULL;
    }
    // Add colors from image vector into array of uint vector types
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


std::string ts(time_t* start_time)
{   
    // Return string with time difference from start of execution
    std::stringstream stream;
    stream << difftime(time(0), *start_time);
    return "[" + stream.str() + "s] ";
}


