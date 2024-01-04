#include <stdio.h>

#include <stdlib.h>

#include "lib/bmp.h"

#include <math.h>

#include <string.h>

#include <mpi.h>

#include <omp.h>


#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define MAX(a,b) ((a) < (b) ? (b) : (a))

#define GP(X,Y) GetPixel(img, width, height,(X),(Y))



/**

 * @brief Convert an image to grayscale

 *

 * @param img

 * @param width

 * @param height

 */

void ImageToGrayscale(RGB *img, const int width, const int height){

    for(int i=0;i<width*height;i++){

        char grayscale=img[i].red*0.3+img[i].green*0.59+img[i].blue*0.11;

        img[i].red=grayscale;

        img[i].green=grayscale;
  }

}



/**

 * @brief Return a pixel no matter the coordinate

 *

 * @param img

 * @param width

 * @param height

 * @param x

 * @param y

 * @return RGB

 */

RGB GetPixel(RGB *img, const int width, const int height, const int x, const int y){

    if(x<0 || y<0 || x>=width || y >=height){

        RGB pixel;

        pixel.red=0;

        pixel.green=0;

        pixel.blue=0;

        return(pixel);

    }

    return(img[x+y*width]);

}
void ApplySobel(RGB *img, const int width,const int height,RGB *out_img,const int rank){
    int sobel_x[3][3] ={{ -1, 0, 1 },
                        { -2, 0, 2 },
                        { -1, 0, 1 } };//matrix for convolution for x
    int sobel_y[3][3] ={{ -1, -2, -1 },
                        { 0,  0,  0 },
                        { 1,2,1 } };//matrix for convolution for y
    RGB *sobel=&img[rank * height * width]; //pointer for specific part of image on every processes
    #pragma omp parallel for                //parallelizing the loops
    for (int i = 0; i < width; i++)//loop for accessing all the pixels
    {
     	for (int j = 0; j < height; j++)
        {
                int xpixel=0;//storing x values for x matrix
                int ypixel=0; //storing y values for y matrix
                 for(int k=0; k < 3; k++) {//loop for convolution of size 3
                    for(int l=0; l < 3; l++) {
                        xpixel += sobel_x[k][l]* sobel[width * (j + k - 1) + (i + l - 1)].blue;/matrix 
                                                                                        //multiplication and adding them all
                        ypixel += sobel_y[k][l] * sobel[width * (j + k - 1) + (i + l - 1)].blue;    //    calculating the pixel 
          }
	}
                    int pixel = sqrt((xpixel * xpixel) + (ypixel * ypixel));  //value by adding both x and y squares and taking square root 

                    if (pixel<0)
                    {
                     	 pixel=0;   // if the the pixel values are negative
                    }

                    if(pixel>255){
                        pixel=255;  //if the pixel values are > 255 
                    }
                    out_img[i+(j*width)].blue = pixel;  //assigning the same value to each color pixel as calculation based on grayscale image so same value to all
                    out_img[i+(j*width)].red = pixel;
                    out_img[i+(j*width)].green = pixel;
      }


    }
 }


 void ApplyBoxBlur(RGB *img, const int width, const int height,RGB *out_img,const int rank){

    //my code starts here
    RGB *chunk = &img[rank * height * width]; //pointer for image chunk on every process based on rank  
    #pragma omp parallel for                   //omp parallel: loop parallization
    for(int i=0;i<width;i++){// loops for accessing the image pixel by pixel

        for (int j = 0; j <height; j++)
        {
            int bluePixel=0;    //setting varibles values to 0 for pixel calculation 
            int greenPixel=0;   //three different variable as Images are RGB combination
            int redPixel=0;
             for(int k=0; k < 3; k++) {//loop for values of each color pixel by adding 
                                        //values from neighbors pixels with size 3
                for(int l=0; l < 3; l++) {
                    bluePixel += chunk[width * (j + k - 1) + (i + l - 1)].blue;
                    greenPixel += chunk[width * (j + k - 1) + (i + l - 1)].green;
                    redPixel += chunk[width * (j + k - 1) + (i + l - 1)].red;
                }
            }

  	    out_img[i+j*width].red=redPixel/9;  //storing the calculated values in an output image and also 
                                            //dividing on 9 with respect to calculation on 3x3 matrix
        out_img[i+j*width].green=greenPixel/9;
        out_img[i+j*width].blue=bluePixel/9;
        }
    }
    //ends here

}



int main(){

    const char *marguerite="marguerite.bmp";
    const char *outsobel="sobel.bmp";
    const char *outbblur="boxblur.bmp";
    int width,height;
    GetSize(marguerite,&width, &height);

    int size, rank;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int chunkSize = height / size;  //defining height for each sub image based on mpi size

    
    RGB *sobel=malloc(sizeof(RGB)*width*height);

    RGB *bblur=malloc(sizeof(RGB)*width*height);
    RGB *output=malloc(sizeof(RGB)*width*chunkSize);


    if(rank == 0) {
    // Load images in first rank
        LoadRegion(marguerite, 0, 0, width, height, sobel);
        LoadRegion(marguerite, 0, 0, width, height, bblur);
    }
    //sending and distributing image to all processes in communicator
    MPI_Bcast(&(sobel[0]), height * width * sizeof(RGB), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    ApplySobel(sobel,width, chunkSize,output,rank);
    //taking and gathering data from all processes and combining to one root process
    MPI_Gather(&(output[0]), chunkSize * width * sizeof(RGB), MPI_UNSIGNED_CHAR, &((sobel)[0]), chunkSize * width * sizeof(RGB), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
   
    if(rank == 0) {
    // Save images in root process
    CreateBMP(outsobel, width, height);
    WriteRegion(outsobel, 0, 0, width, height, sobel);
    }

    //sending and distributing image to all processes in communicator
    MPI_Bcast(&(bblur[0]), height * width * sizeof(RGB), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    ApplyBoxBlur(bblur, width,chunkSize, output,rank);
    //taking and gathering data from all processes and combining to one root process
    MPI_Gather(&(output[0]), chunkSize * width * sizeof(RGB), MPI_UNSIGNED_CHAR, &((bblur)[0]), chunkSize * width * sizeof(RGB), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
   
    if(rank == 0) {
    // Save images in root process
    CreateBMP(outbblur, width, height);
    WriteRegion(outbblur, 0, 0, width, height, bblur);
    }

    //MPI exit

    MPI_Finalize();
    //freeing the memory
    free(sobel);
    free(bblur);
    free(output)


    return 0;

}



