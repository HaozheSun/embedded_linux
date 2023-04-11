#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <video.h>
#define PI 3.141592654

typedef unsigned char byte;
// The dimensions of the image
int width, height;
int screen_x, screen_y, char_x, char_y;

struct pixel {
    byte b;
    byte g;
    byte r;
};

// Read BMP file and extract the pixel values (store in data) and header (store in header)
// Data is data[0] = BLUE, data[1] = GREEN, data[2] = RED, data[3] = BLUE, etc...
int read_bmp(char *filename, byte **header, struct pixel **data) {
    struct pixel *data_tmp;
    byte *header_tmp;
    FILE *file = fopen (filename, "rb");

    if (!file) return -1;

    // read the 54-byte header
    header_tmp = malloc (54 * sizeof(byte));
    fread (header_tmp, sizeof(byte), 54, file); 

    // get height and width of image from the header
    width = *(int*)(header_tmp + 18);  // width is a 32-bit int at offset 18
    height = *(int*)(header_tmp + 22); // height is a 32-bit int at offset 22

    // Read in the image
    int size = width * height;
    data_tmp = malloc (size * sizeof(struct pixel)); 
    fread (data_tmp, sizeof(struct pixel), size, file); // read the data
    fclose (file);

    *header = header_tmp;
    *data = data_tmp;

    return 0;
}

// Determine the grayscale 8-bit value by averaging the r, g, and b channel values.
// Store the 8-bit grayscale value in the r channel.
void convert_to_grayscale(struct pixel *data) {
    int x, y;

    // declare image as a 2-D array so that we can use the syntax image[row][column]
    struct pixel (*image)[width] = (struct pixel (*)[width]) data;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {

            // Just use the 8 bits of the r field to hold the entire grayscale image
            image[y][x].r = (image[y][x].r + image[y][x].b + image[y][x].g) / 3;
            image[y][x].g = image[y][x].r;
            image[y][x].b = image[y][x].r;

        }
    }
}

// Write the grayscale image to disk. The 8-bit grayscale values should be inside the
// r channel of each pixel.
void write_bmp(char *filename, byte *header, struct pixel *data) {
    FILE* file = fopen (filename, "wb");
    // declare image as a 2-D array so that we can use the syntax image[row][column]
    struct pixel (*image)[width] = (struct pixel (*)[width]) data;

    // write the 54-byte header
    fwrite (header, sizeof(byte), 54, file);
    int size = width * height;
    fwrite (image, sizeof(struct pixel), size, file); // write the data
    fclose (file);
}

// The input data is either the x- or y-derivative of the image, as calculated by Sobel. The
// output produced is a bmp file in which each pixel corresponds to the absolute value of the 
// derivative at that pixel. This bmp file allows us to visualize the derivative as a bmp image.
void write_signed_bmp(char *filename, byte *header, signed int *data) {
    FILE* file = fopen (filename, "wb");
    struct pixel bytes;
    int val;

    signed int (*image)[width] = (signed int (*)[width]) data; // allow image[][]
    // write the 54-byte header
    fwrite (header, sizeof(byte), 54, file);
    int y, x;

    // convert the derivatives' values to pixels by copying each to an r, g, and b
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            val = abs(image[y][x]);
            val = (val > 255) ? 255 : val;
            bytes.r = val;
            bytes.g = bytes.r;
            bytes.b = bytes.r;
            fwrite (&bytes, sizeof(struct pixel), 1, file); // write the data
        }
    }
    fclose (file);
}

// Gaussian blur. Operate on the .r fields of the pixels only.
void gaussian_blur(struct pixel **data) {

    unsigned int filter[5][5] = {
        { 1,  4,  7,  4, 1 },
        { 4, 16, 26, 16, 4 },
        { 7, 26, 41, 26, 7 },
        { 4, 16, 26, 16, 4 },
        { 1,  4,  7,  4, 1 }
    };
    
    // allocate memory for the 2-D convolution image
    struct pixel (*convolution)[width] = malloc (sizeof(struct pixel [height][width])); 
    // declare an image[][] variable to access the pixel data
    struct pixel (*image)[width] = (struct pixel (*)[width]) *data;

    /** please complete this function  **/
    int x,y,i,j,sum = 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sum = 0;
            for (i = -2; i <= 2; i++) {
                for (j = -2; j <= 2; j++) {
                    int x1 = x + i;
                    int y1 = y + j;
                    if (x1<0) x1 = 0;
                    if (y1<0) y1 = 0;
                    if (x1>=width) x1 = width-1;
                    if (y1>=height) y1 = height-1;
                    
                    sum += image[y1][x1].r * filter[i + 2][j + 2];
                    
                    
                }
            }
            convolution[y][x].r = sum / 273;
            convolution[y][x].g = convolution[y][x].r;
            convolution[y][x].b = convolution[y][x].r;
        }
    }
    

    free (*data);                           // the input data is no longer needed
    *data = (struct pixel *) convolution;   // return the convolution of the image
}

/* This function finds the image gradient.
 * Input:
 *      data is a 2-D array of image pixels
 * Outputs: 
 *      data is used to return the intensity of the image gradient
 *      conv_x is used to return the convolution with the sobel_x operator (Df/dx)
 *      conv_y is used to return the convolution with the sobel_y operator (Df/dy)
 */
void sobel_filter(struct pixel **data, signed int **conv_x, signed int **conv_y) {
    // Definition of Sobel filter in horizontal direction (highlights vertical edges)
    int sobel_x[3][3] = {
        { -1,  0,  1 },
        { -2,  0,  2 },
        { -1,  0,  1 }
    };
    // Definition of Sobel filter in vertical direction (highlights horizontal edges)
    int sobel_y[3][3] = {
        { -1, -2, -1 },     // y == 0 (bottom) row
        {  0,  0,  0 },     // y == 1 (middle) row
        {  1,  2,  1 }      // y == 2 (top) row
    };

    signed int (*G_x)[width] = malloc (sizeof(signed int [height][width])); 
    signed int (*G_y)[width] = malloc (sizeof(signed int [height][width])); 
    struct pixel (*gradient)[width] = malloc (sizeof(struct pixel [height][width])); 
    // declare an image[][] varable to access the pixel data
    struct pixel (*image)[width] = (struct pixel (*)[width]) *data;

    /**  please complete this function  **/
    int x,y,i,j,sum_x = 0,sum_y = 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sum_x = 0;
            sum_y = 0;
            for (i = -1; i <= 1; i++) {
                for (j = -1; j <= 1; j++) {
                    int x1 = x + i;
                    int y1 = y + j;
                    if (x1<0) x1 = 0;
                    if (y1<0) y1 = 0;
                    if (x1>=width) x1 = width-1;
                    if (y1>=height) y1 = height-1;
                    
                    sum_x += image[y1][x1].r * sobel_x[i + 1][j + 1];
                    sum_y += image[y1][x1].r * sobel_y[i + 1][j + 1];
                    
                    
                }
            }
            G_x[y][x] = sum_x;
            G_y[y][x] = sum_y;
            int val = abs(sum_x)/2 + abs(sum_y)/2;
            if (val > 255) val = 255;
            gradient[y][x].r = val;
            gradient[y][x].g = gradient[y][x].r;
            gradient[y][x].b = gradient[y][x].r;
        }
    }



    free (*data);   // the previous image data is no longer needed
    *data = (struct pixel *) gradient;
    *conv_x = (signed int *) G_x;
    *conv_y = (signed int *) G_y;
}

/* Makes edges thinner */
void non_max_suppress(struct pixel **data, signed int *sobel_x, signed int *sobel_y) {

    struct pixel (*temp_data)[width] = malloc (sizeof(struct pixel [height][width])); 
    struct pixel (*image)[width] = (struct pixel (*)[width]) *data;
    signed int (*G_x)[width] = (signed int (*)[width]) sobel_x; // enable G_x[][]
    signed int (*G_y)[width] = (signed int (*)[width]) sobel_y; // enable G_y[][]

    /**  please complete this function  **/
    int x,y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (G_x[y][x] == 0) {
                if (G_y[y][x] == 0) {
                    temp_data[y][x].r = 0;
                    temp_data[y][x].g = 0;
                    temp_data[y][x].b = 0;
                }
                else {
                    if (y == 0){
                        if(image[y + 1][x].r > image[y][x].r){
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else{
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if (y == height - 1) {
                        if (image[y - 1][x].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else {
                        if (image[y + 1][x].r > image[y][x].r || image[y - 1][x].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                }
            }
            else {
                int theta = atan(G_y[y][x] / G_x[y][x]) * 180 / PI;
                if ((theta < 67 && theta > 22) || (theta > -157 && theta < -112)) {
                    if (x != 0 && x != width - 1 && y != 0 && y != height - 1) {
                        if (image[y+1][x + 1].r > image[y][x].r || image[y-1][x - 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if ( (x == width - 1 || y == height - 1) && (x != 0 && y != 0) ) {
                        if (image[y - 1][x - 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else{
                        if (image[y + 1][x + 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    
                }

                else if ((theta > -67 && theta < -22) || (theta > 112 && theta < 157)) {
                    if (x != 0 && x != width - 1 && y != 0 && y != height - 1) {
                        if (image[y + 1][x - 1].r > image[y][x].r || image[y - 1][x + 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if ((x == 0 && y == 0) || (x == width - 1 && y == height - 1)) {
                        temp_data[y][x].r = image[y][x].r;
                        temp_data[y][x].g = image[y][x].g;
                        temp_data[y][x].b = image[y][x].b;
                    }
                    else if (y == 0) {
                        if (image[y + 1][x - 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if (x == 0){
                        if (image[y - 1][x + 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if (x == width - 1) {
                        if (image[y + 1][x - 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if (y == height - 1) {
                        if (image[y - 1][x + 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                }

                else if ((theta >= 67 && theta <= 112) || (theta <= -67 && theta >= -112)) {
                    if (y == 0){
                        if(image[y + 1][x].r > image[y][x].r){
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else{
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if (y == height - 1) {
                        if (image[y - 1][x].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else {
                        if (image[y + 1][x].r > image[y][x].r || image[y - 1][x].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                }

                else{
                    if (x == 0){
                        if(image[y][x + 1].r > image[y][x].r){
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else{
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else if (x == width - 1) {
                        if (image[y][x - 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                    else {
                        if (image[y][x + 1].r > image[y][x].r || image[y][x + 1].r > image[y][x].r) {
                            temp_data[y][x].r = 0;
                            temp_data[y][x].g = 0;
                            temp_data[y][x].b = 0;
                        }
                        else {
                            temp_data[y][x].r = image[y][x].r;
                            temp_data[y][x].g = image[y][x].g;
                            temp_data[y][x].b = image[y][x].b;
                        }
                    }
                }
                    
                
            }
        }
    }



    free(*data);
    *data = (struct pixel *) temp_data;
}

// Only keep pixels that are next to at least one strong pixel.
void hysteresis_filter(struct pixel ** data) {
    #define strong_pixel 32 // example value
    
    struct pixel (*temp_data)[width] = malloc (sizeof(struct pixel [height][width])); 
    struct pixel (*image)[width] = (struct pixel (*)[width]) *data;

    /**  please complete this function  **/
    int x, y, i, j;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            temp_data[y][x].r = 0;
            temp_data[y][x].g = 0;
            temp_data[y][x].b = 0;
            if (image[y][x].r > strong_pixel) {
                for (i = -1; i<2; i++) {
                    for (j = -1; j<2; j++) {
                        if (y + i >= 0 && y + i < height && x + j >= 0 && x + j < width) {
                            if (image[y + i][x + j].r > strong_pixel) {
                                temp_data[y][x].r = image[y][x].r;
                                temp_data[y][x].g = image[y][x].g;
                                temp_data[y][x].b = image[y][x].b;
                                break;
                            }
                        }
                    }
                }
            }
            
        }
    }



    free (*data);
    *data = (struct pixel *) temp_data;
}

// Render an image on the VGA display
void draw_image (struct pixel * data)
{
    int x, y, stride_x, stride_y, i, j, vga_x, vga_y;
    int r, g, b, color;
    struct pixel (*image)[width] = (struct pixel (*)[width]) data; // allow image[][]

    video_clear ( );
    // scale the image to fit the screen
    stride_x = (width > screen_x) ? width / screen_x : 1;
    stride_y = (height > screen_y) ? height / screen_y : 1;
    // scale proportionally (don't stretch the image)
    stride_y = (stride_x > stride_y) ? stride_x : stride_y;
    stride_x = (stride_y > stride_x) ? stride_y : stride_x;
    for (y = 0; y < height; y += stride_y) {
        for (x = 0; x < width; x += stride_x) {
            // find the average of the pixels being scaled down to the VGA resolution
            r = 0; g = 0; b = 0;
            for (i = 0; i < stride_y; i++) {
                for (j = 0; j < stride_x; ++j) {
                    r += image[y + i][x + j].r;
                    g += image[y + i][x + j].g;
                    b += image[y + i][x + j].b;
                }
            }
            r = r / (stride_x * stride_y);
            g = g / (stride_x * stride_y);
            b = b / (stride_x * stride_y);

            // now write the pixel color to the VGA display
            r = r >> 3;      // VGA has 5 bits of red
            g = g >> 2;      // VGA has 6 bits of green
            b = b >> 3;      // VGA has 5 bits of blue
            color = r << 11 | g << 5 | b;
            vga_x = x / stride_x;
            vga_y = y / stride_y;
            if (screen_x > width / stride_x)   // center if needed
                video_pixel (vga_x + (screen_x-(width/stride_x))/2, (screen_y-1) - vga_y, color); 
            else
                if ((vga_x < screen_x) && (vga_y < screen_y))
                    video_pixel (vga_x, (screen_y-1) - vga_y, color); 
        }
    }
    video_show ( );
}

int main(int argc, char *argv[]) {
    struct pixel *image;
    signed int *G_x, *G_y;
    byte *header;
    int debug = 0, video = 0;
    time_t start, end;

    // Check inputs
    if (argc < 2) {
        printf("Usage: part1 [-d] [-v] <BMP filename>\n");
        printf("-d: produces debug output for each stage\n");
        printf("-v: draws the input and output images on a video-out display\n");
        return 0;
    }

    int opt;
    while ((opt = getopt (argc, argv, "dv")) != -1) {
        switch (opt) {
            case 'd':
                debug = 1;
                break;
            case 'v':
                video = 1;
                break;
            case '?':
                printf("unknown option: %c\n", optopt); 
                break;
        }
    }
    // Open input image file (24-bit bitmap image)
    if (read_bmp (argv[optind], &header, &image) < 0) {
        printf("Failed to read BMP\n");
        return 0;
    }
    if (video) {
        if (!video_open ())
        {
            printf ("Error: could not open video device\n");
            return -1;
        }
        video_read (&screen_x, &screen_y, &char_x, &char_y);   // get VGA screen size
        draw_image (image);
    }

    /********************************************
    *          IMAGE PROCESSING STAGES          *
    ********************************************/

    // Start measuring time
    start = clock ();

    /// Grayscale conversion
    convert_to_grayscale (image);
    if (debug) write_bmp ("stage0_grayscale.bmp", header, image);

    /** please uncomment after you implement gaussian_blur **/
    gaussian_blur (&image);
    if (debug) write_bmp ("stage1_gaussian.bmp", header, image);


    /** please uncomment after you implement sobel_filter **/
    sobel_filter (&image, &G_x, &G_y);
    if (debug) {
        write_signed_bmp ("stage2_gradient_x.bmp", header, G_x);
        write_signed_bmp ("stage2_gradient_y.bmp", header, G_y);
        write_bmp ("stage2_gradient.bmp", header, image);
    }

    /** please uncomment after you implement non_max_suppress **/
    non_max_suppress (&image, G_x, G_y);
    if (debug) write_bmp ("stage3_nonmax_suppression.bmp", header, image);

    /* Hysteresis*/
    /* please uncomment after you implement hysteresis_filter */
    hysteresis_filter (&image);
    if (debug) write_bmp ("stage4_hysteresis.bmp", header, image);

    end = clock();

    printf("TIME ELAPSED: %.0f ms\n", ((double) (end - start)) * 1000 / CLOCKS_PER_SEC);

    // write image to file
    write_bmp ("edges.bmp", header, image);

    /** display the final output image at runtime **/
    if (video) {
        printf("Press <Enter> to display output image\n");
        getchar ();
        draw_image (image);
        video_close ( );
    }

    return 0;
}