#include<iostream>
#include<stdio.h>
#include<assert.h>
#include "bmp.h"

using namespace std;

const char* INPUT_IMAGE_PATH = "C:\\Users\\m0pxnn\\Desktop\\ImageTestFiles\\img15.bmp";
const char* OUTPUT_IMAGE_PATH = "C:\\Users\\m0pxnn\\Desktop\\ImageTestFiles\\img15_modified.bmp";

//******************************************************************************************
// M A I N - for testing purpose.
//******************************************************************************************
int main()
{
    int retval = -1;
    
    {
        BitmapImage bmpImage(INPUT_IMAGE_PATH);

        bmpImage.displayImageDetails();

#if 0
        //bmpImage.displayImagePixels();
        retval = bmpImage.ConvertToGrayScale();
        if (retval != 0)
        {
            printf("\nERROR: Image modification failed!\n");
        }

        printf("\nWriting to file...\n");
        retval = bmpImage.writeModifiedImageDataToFile(OUTPUT_IMAGE_PATH);
        if (retval != 0)
        {
            printf("\n>> Failed to create %s\n", OUTPUT_IMAGE_PATH);
        }
        else
        {
            printf("\n>> Created %s\n", OUTPUT_IMAGE_PATH);
        }
#endif
        //bmpImage.displayHistogram();
#if 1
        printf("\n\nDoing histogram equalization...\n");
        bmpImage.doHistogramEqualization();
        //bmpImage.displayHistogram();

        printf("\nWriting to file...\n");
        retval = bmpImage.writeModifiedImageDataToFile(OUTPUT_IMAGE_PATH);
        if (retval != 0)
        {
            printf("\n>> Failed to create %s\n", OUTPUT_IMAGE_PATH);
        }
        else
        {
            printf("\n>> Created %s\n", OUTPUT_IMAGE_PATH);
        }
#endif
    }//Scope of BitmapImage object ends here

    getchar();
    return 0;
}