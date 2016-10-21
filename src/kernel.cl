void kernel render_image(__write_only image2d_t image, int size)
{
    
    size_t gid_x = get_global_id(0);
    size_t gid_y = get_global_id(1);    

    size_t lx = get_local_id(0);
    size_t ly = get_local_id(1);

    printf("[x: %d, y: %d]\t[lx: %d, ly: %d]\n", (int)gid_x, (int)gid_y, (int)lx, (int)ly);

    uint4 somecolor;
    if (gid_x == 0 && gid_y == 0)
        somecolor = (uint4){255, 0, 0, 255};
    if (gid_x == 0 && gid_y == 1)
        somecolor = (uint4){0, 255, 0, 255};
    if (gid_x == 1 && gid_y == 0)
        somecolor = (uint4){0, 0, 255, 255};
    if (gid_x == 1 && gid_y == 1)
        somecolor = (uint4){255, 255, 255, 255};

    int x_start = gid_x * (size/2);
    int x_end   = x_start + (size/2);
    int y_start = gid_y * (size/2);
    int y_end   = y_start + (size/2);

    int i_x, i_y;
    for (i_x = x_start; i_x < x_end; i_x++)
    {
        for (i_y = y_start; i_y < y_end; i_y++)
        {
            write_imageui(image, (int2){i_x, i_y}, somecolor);
        }
    }
}
