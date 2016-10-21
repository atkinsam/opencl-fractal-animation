/*  colormap.h
 *
 *  Author: Sam Atkinson
 *  Date modified: 08-Oct-2016
 *
 *  Header file for colormap object and functions 
 */

#ifndef COLORMAP_H
#define COLORMAP_H

#include "lodepng.h"

typedef struct {
	unsigned char R, G, B, A;
} color;


class colormap
{
	public:
        colormap();
		colormap(std::string filename, double weight_min, double weight_max);
		void analyze(void);
		color grayscale_to_color(double weight);
        std::string filename;
        std::vector<color> cmap;
        double weight_min, weight_max;
};


#endif /* COLORMAP_H */
