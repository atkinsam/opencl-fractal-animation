/*  colormap.cpp
 *
 *  Author: Sam Atkinson
 *  Date modified: 08-Oct-2016
 *
 *  colormap object and functions
 */

#include "colormap.h"


colormap::colormap() {}

colormap::colormap(std::string filename_init, double weight_min_init, 
        double weight_max_init)
{
	filename = filename_init;
	weight_min = weight_min_init;
	weight_max = weight_max_init;
}

void colormap::analyze(void)
{
    std::vector<unsigned char> image;
    unsigned int width, height;
    unsigned error = lodepng::decode(image, width, height, filename.c_str());
    color c;
    for (int i = 0; i < width; i++)
    {
        c.R = image[i * 4 + 0];
        c.G = image[i * 4 + 1];
        c.B = image[i * 4 + 2];
        c.A = 255;
        cmap.push_back(c);
    }
}

color colormap::grayscale_to_color(double weight)
{
	int color_index = (weight - weight_min) / 
        (weight_max - weight_min) * cmap.size();
	return cmap[color_index];
}
