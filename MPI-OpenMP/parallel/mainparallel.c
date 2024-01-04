#include <stdio.h>
#include <stdlib.h>
#include "lib/bmp.h"
#include <mpi.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <omp.h>


void ImageToGrayscale(RGB *img, const int height, const int width){
  for(int i=0;i< height * width ;i++){
    char grayscale=img[i].red*0.3+img[i].green*0.59+img[i].blue*0.11;
    img[i].red=grayscale;
    img[i].green=grayscale;
    img[i].blue=grayscale;
  }
}

void ApplyBoxBlur(RGB *img, RGB *outImg, const int height, const int width){
  
  RGB *chunk = &img[width + 1];

  #pragma omp parallel for num_threads(4)
  for (int w=0; w < width; w++)
  {
    for (int h=0; h < height; h++)
    {
      
      int redPixel=0,greenPixel=0,bluePixel=0;
      for(int i=0; i < 3; i++) {
        for(int j=0; j < 3; j++) {
          redPixel += chunk[width * (h + i - 1) + (w + j - 1)].red;
          greenPixel += chunk[width * (h + i - 1) + (w + j - 1)].green;
          bluePixel += chunk[width * (h + i - 1) + (w + j - 1)].blue;
        }
      }
      redPixel /= 9;
      greenPixel /= 9;
      bluePixel /= 9;
      
      if(redPixel < 0) redPixel=0;
      else if(redPixel > 255) redPixel=255;
      
      if(greenPixel < 0) greenPixel=0;
      else if(greenPixel > 255) greenPixel=255;
      
      if(bluePixel < 0) bluePixel=0;
      else if(bluePixel > 255) bluePixel=255;
      
      outImg[w+h*width].red = redPixel;
      outImg[w+h*width].green = greenPixel;
      outImg[w+h*width].blue = bluePixel;
    }
  }
}

void ApplySobel(RGB *img, RGB *outImg, const int height, const int width){
  int x_sobel[3][3] = {{-1,0,1 },{-2,0,2 },{-1,0,1 }};

  int y_sobel[3][3] = {{-1,-2,-1 },{ 0,0,0 },{ 1,2,1 }}; 
  RGB *chunk = &img[width + 1];

  #pragma omp parallel for num_threads(4)
  for (int w=0; w < width; w++)
  {
    for (int h=0; h < height; h++)
    {
        int x_pixel = 0;
        int y_pixel = 0;

        for(int i=0; i < 3; i++) {
          for(int j=0; j < 3; j++) {
            x_pixel += x_sobel[i][j] * chunk[(w + j - 1) + (h + i - 1)*width].green;
            y_pixel += y_sobel[i][j] * chunk[(w + j - 1) + (h + i - 1)*width].green;
          }
        }
        int pixel = (int)sqrt((x_pixel * x_pixel) + (y_pixel * y_pixel));

        if(pixel < 0) pixel = 0;
        else if(pixel > 255) pixel = 255;

        outImg[w+h*width].blue = pixel;
        outImg[w+h*width].red = pixel;
        outImg[w+h*width].green = pixel;
    }
  }
}


int main (int argc , char* argv[]) {


  int rank, size;
  MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

  const char *marguerite = "marguerite.bmp";
  const char *outbblur = "boxblur.bmp";
  const char *outsobel = "sobel.bmp";
  

  // Get image dimensions
  int width, height;
  GetSize(marguerite, &width, &height);

  int chunk = height / (size - 1);
  if(rank == size - 1) {
    chunk += height % size;
  }

  // // Init memory
  RGB *sobel = malloc(sizeof(RGB) * width * height);
  RGB *bblur = malloc(sizeof(RGB) * width * height);
  //specifying the pixels to be sent to each process and recieved from them
  long long int sizeToSend = (sizeof(RGB) * chunk * width) + (sizeof(RGB) * 2 * width);
  long long int sizeToRec = sizeof(RGB) * chunk * width;
  //allocating memory for sending and recieving pixels
  RGB *SobBuff = malloc(sizeToSend);
  RGB *BblrBuff = malloc(sizeToSend);
  RGB *sobOutImg = malloc(sizeof(RGB) * chunk * width);
  RGB *bblrOutImg = malloc(sizeof(RGB) * chunk * width);
  // waiting for all processe to get time
  MPI_Barrier(MPI_COMM_WORLD);
  double time = MPI_Wtime();

  if(rank == 0) {
    // Load images in and sending specific chunk to each process in first rank
    LoadRegion(marguerite, 0, 0, width, height, sobel);
    LoadRegion(marguerite, 0, 0, width, height, bblur);

    for(int i = 1; i < size; i++) {
      int start = ((i - 1) * chunk * width);
      if(start > 0) {
        start = start - width + 1;
      }
      MPI_Send(&sobel[start], sizeToSend, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD);
      MPI_Send(&bblur[start], sizeToSend, MPI_UNSIGNED_CHAR, i, 2, MPI_COMM_WORLD);
    }
  }

  if(rank != 0) { //functions call based on the recieved image chunk
    MPI_Recv(SobBuff, sizeToSend, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(BblrBuff, sizeToSend, MPI_UNSIGNED_CHAR, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    ApplySobel(SobBuff, sobOutImg, chunk, width);
    ApplyBoxBlur(BblrBuff, bblrOutImg, chunk, width);
    //sending back the processed chunk of image to rank 0
      MPI_Send(sobOutImg, sizeToRec, MPI_UNSIGNED_CHAR, 0, 3, MPI_COMM_WORLD);
      MPI_Send(bblrOutImg, sizeToRec, MPI_UNSIGNED_CHAR, 0, 4, MPI_COMM_WORLD);
  }


  if(rank == 0) {

    for(int i = 1; i < size; i++) {
      int start = (i - 1) * chunk * width;
      if(start > 0) {
        start = start - width + 1;
      }
      //recieving back the processed part of image
      MPI_Recv(&sobel[start], sizeToRec, MPI_UNSIGNED_CHAR, i, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&bblur[start], sizeToRec, MPI_UNSIGNED_CHAR, i, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Saving images after processing
    CreateBMP(outbblur, width, height);
    WriteRegion(outbblur, 0, 0, width, height, bblur);
    CreateBMP(outsobel, width, height);
    WriteRegion(outsobel, 0, 0, width, height, sobel);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  double endtime=MPI_Wtime();
  free(bblur);
  free(sobel);
  
  MPI_Finalize();
  printf("time taken %f ",endtime-time);
  return 0;
}

