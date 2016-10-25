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


/* compute the depth of one pixel of a fractal image */
void kernel render_image(__write_only image2d_t image, 
                         __constant float* spaced_re,
                         __constant float* spaced_im,
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

    uint4 color = (uint4)(depth, depth, depth, 255);

    write_imageui(image, pos, color);
}
