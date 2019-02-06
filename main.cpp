#include<iostream>
#include<stdio.h>
#include<assert.h>
#include "bmp.h"

using namespace std;

const char* INPUT_IMAGE_PATH = "C:\\Users\\m0pxnn\\Desktop\\img1.bmp";
const char* OUTPUT_IMAGE_PATH = "C:\\Users\\m0pxnn\\Desktop\\img1_modified.bmp";

int main()
{
    BitmapImage bmpImage(INPUT_IMAGE_PATH);

    bmpImage.displayImageDetails();
    //bmpImage.displayImagePixels();
    int retval = bmpImage.writeModifiedImageDataToFile(OUTPUT_IMAGE_PATH);
    if (retval != 0)
    {
        printf("\n>> Failed to create %s\n", OUTPUT_IMAGE_PATH);
    }
    else
    {
        printf("\n>> Created %s\n", OUTPUT_IMAGE_PATH);
    }

    getchar();
    return 0;
}