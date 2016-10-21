void kernel render_image(__write_only image2d_t image, int size)
{
    uint4 somecolor = {0, 255, 0, 100};
    int i, j;
    for (i = 0; i < size; i++)
    {
        for (j = 50; j < size - 50; j++)
        {
            int2 pos = {i, j};
            write_imageui(image, pos, somecolor);
        }
    }
}
