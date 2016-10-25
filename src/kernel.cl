/*  kernel.cl
 *  
 *  Author: Sam Atkinson
 *  Date modified: Oct. 24, 2016
 *
 *
 */


/* define type Complex */
typedef double2 Complex;

/* get magnitude of Complex */
inline float c_abs(Complex a)
{
    return (sqrt(a.x * a.x + a.y * a.y));
}

/* multiply Complex numbers */
inline Complex c_multiply(Complex a, Complex b)
{
    return (Complex)(a.x * b.x - a.y * b.y,
                     a.x * b.y + a.y * b.x);
}

/* add Complex numbers */
inline Complex c_add(Complex a, Complex b)
{
    return (Complex)(a.x + b.x, a.y + b.y);
}

/* compute evenly spaced real values */
void kernel even_re(float center_re,
                    float zoom,
                    float size,
                    global float* out)
{
    float min = center_re - zoom / 2.0;
    float max = center_re + zoom / 2.0;
    float interval = (max - min) / size;
    for (unsigned int i = 0; i < (unsigned int)size; i++)
    {
        out[i] = min + interval * i;
    }
}

/* compute evenly spaced imaginary values */
void kernel even_im(float center_im,
                    float zoom,
                    float size,
                    global float* out)
{
    float min = center_im - zoom / 2.0;
    float max = center_im + zoom / 2.0;
    float interval = (max - min) / size;
    for (unsigned int i = 0; i < (unsigned int)size; i++)
    {
        out[i] = min + interval * i;
    }
}

/* compute the depth of one pixel of a fractal image */
void kernel render_image(__write_only image2d_t image, 
                         global const float* spaced_re,
                         global const float* spaced_im,
                         global const uint4* cmap,
                         unsigned int cmap_size,
                         float c_re,
                         float c_im)
{
    int2 pos = {get_global_id(0), get_global_id(1)};

    Complex z = (Complex)(spaced_re[pos.x], spaced_im[pos.y]);
    Complex c = (Complex)(c_re, c_im);
    unsigned char depth = 255; 
    while(c_abs(z) < 1000 && depth >= 1)
    {
        z = c_add(c_multiply(z, z), c);
        depth--;
    }
    unsigned int color_index = (float)(depth - 0) /
                               (float)(255 - 0) * cmap_size;
    write_imageui(image, pos, cmap[color_index]);
}
