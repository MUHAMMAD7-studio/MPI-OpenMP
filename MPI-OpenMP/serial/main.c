#include <stdio.h>
#include <stdlib.h>
#include "lib/bmp.h"
#include <math.h>
#include <string.h>
#include <time.h>

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
        img[i].blue=grayscale;
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

void ApplySobel(RGB *img, const int width, const int height,RGB *out_img){
     int sobel_x[3][3] ={{ -1, 0, 1 },   
                        { -2, 0, 2 },
                        { -1, 0, 1 } }; //matrix for convolution for x
    int sobel_y[3][3] ={{ -1, -2, -1 },
                        { 0,  0,  0 },
                        { 1,2,1 } }; 
    for(int i=0;i<width;i++)
    {
        for(int j=0;j<height;j++)
        {
            int xpixel=0;   //storing x values for x matrix
            int ypixel=0;
            xpixel =(sobel_x[0][0] * (GetPixel(img,width,height,i-1,j-1)).blue+sobel_x[0][1] * (GetPixel(img,width,height,i-1,j)).blue+sobel_x[0][2] * (GetPixel(img,width,height,i-1,j+1)).blue
                    + sobel_x[1][0] *(GetPixel(img,width,height,i,j-1)).red+sobel_x[1][1] * (GetPixel(img,width,height,i,j)).red+sobel_x[1][2] * (GetPixel(img,width,height,i,j+1)).red
                    + sobel_x[2][0] * (GetPixel(img,width,height,i+1,j-1)).green+sobel_x[2][1] * (GetPixel(img,width,height,i+1,j)).green+sobel_x[2][2] * (GetPixel(img,width,height,i+1,j+1)).green);
            ypixel =(sobel_y[0][0] * (GetPixel(img,width,height,i-1,j-1)).blue+sobel_y[0][1] * (GetPixel(img,width,height,i-1,j)).red+sobel_y[0][2] * (GetPixel(img,width,height,i-1,j+1)).green
                    + sobel_y[1][0] * (GetPixel(img,width,height,i,j-1)).blue+sobel_y[1][1] * (GetPixel(img,width,height,i,j)).red+sobel_y[1][2] * (GetPixel(img,width,height,i,j+1)).green
                    + sobel_y[2][0] * (GetPixel(img,width,height,i+1,j-1)).blue+sobel_y[2][1] * (GetPixel(img,width,height,i+1,j)).red+sobel_y[2][2] * (GetPixel(img,width,height,i+1,j+1)).green);
            int pixel = sqrt((xpixel * xpixel) + (ypixel * ypixel));
            if(pixel < 0){ 
            pixel=0;
            }
            else if(pixel > 255) pixel=255;
            out_img[i+j*width].blue=pixel;
            out_img[i+j*width].red=pixel;    
            out_img[i+j*width].green=pixel;  
        }
    }
}

void ApplyBoxBlur(RGB *img, const int width, const int height,RGB *out_img){
    for(int i=0;i<width;i++)
    {
        for(int j=0;j<height;j++)
        {
            int bluePixel=0;    
            int greenPixel=0;  
            int redPixel=0;
            for(int k=0;k<3;k++)
            {
                for(int l=0;l<3;l++)
                { //calculating 
                    bluePixel+=(GetPixel(img,width,height,i-1+k,j-1+l)).blue;
                    greenPixel+=(GetPixel(img,width,height,i-1+k,j-1+l)).green;
                    redPixel+=(GetPixel(img,width,height,i-1+k,j-1+l)).red;
                }
            }
            
            out_img[i+j*width].blue=bluePixel/9;
            out_img[i+j*width].green=greenPixel/9;
            out_img[i+j*width].red=redPixel/9;
        }
    }
}


int main(){
    const char *marguerite="sample.bmp";
    const char *outsobel="sobel.bmp";
    const char *outbblur="boxblur.bmp";
  
    // Get image dimensions
    int width,height;
    GetSize(marguerite,&width, &height);


    // Init memory
    RGB *sobel=malloc(sizeof(RGB)*width*height);
    RGB *bblur=malloc(sizeof(RGB)*width*height);
    RGB *outImgSob=malloc(sizeof(RGB)*width*height);
    RGB *outImgBlur=malloc(sizeof(RGB)*width*height);

    double time1=clock();
    // Load images
    LoadRegion(marguerite,0,0, width,height,sobel);
    LoadRegion(marguerite,0,0, width,height,bblur);
    
    // Apply filters
    ImageToGrayscale(sobel,width,height);
    ApplySobel(sobel,width,height,outImgSob);
    ApplyBoxBlur(bblur,width,height,outImgBlur);

    
    CreateBMP(outsobel,width,height);
    WriteRegion(outsobel,0,0,width, height,outImgSob);
    

    
    CreateBMP(outbblur,width,height);
    WriteRegion(outbblur,0,0,width, height,outImgBlur);
    double time2=clock();
    // Free memory
    free(sobel);
    free(bblur);
    free(outImgSob);
    free(outImgBlur);
    printf("time 1 %f and time 2 %f",time1,time2);
    printf("Time taken %f",(time2 - time1)/CLOCKS_PER_SEC);
    return(0);
}

