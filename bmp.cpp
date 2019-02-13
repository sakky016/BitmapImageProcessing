#include"bmp.h"
#include<assert.h>

//#define USE_BRIGHTNESS_LEVEL_FOR_HISTOGRAM_EQUALIZATION

//******************************************************************************************
// @name                    : CloseFile
//
// @description             : This is a static function. Closes the file
//
// @param fp                : file to be closed
//
// @returns                 : Nothing
//********************************************************************************************
static void CloseFile(FILE *fp)
{
    int retval = 0;
    if (fp != nullptr)
    {
        retval = fclose(fp);
        if (!retval)
        {
            fp = nullptr;
        }
        else
        {
            printf("ERROR: File could not close!\n");
        }
    }
}

//******************************************************************************************
// @name                    : FreeMemory
//
// @description             : This is a static function. Frees the memory if not already freed
//
// @param ptr               : Starting address of memory to be freed.
//
// @returns                 : Nothing
//********************************************************************************************
static void FreeMemory(void *ptr)
{
    if (ptr != nullptr)
    {
        free(ptr);
        ptr = nullptr;
    }
}

//******************************************************************************************
// @name                    : BitmapImage
//
// @description             : Constructor. Opens the input image, extracts and stores header 
//                            information image pixels.
//
// @param imagePath         : Path of image that will be loaded
//
// @returns                 : Nothing
//********************************************************************************************
BitmapImage::BitmapImage(const char *imagePath)
{
    if (!imagePath)
    {
        printf("Image file not specified!\n");
        assert(0);
    }

    m_inputFilePointer = fopen(imagePath, "rb");   //read the file//
    if (m_inputFilePointer == nullptr)
    {
        printf("File [%s] not found!\n", imagePath);
        throw "Exception: File not found!";
    }

    m_imagePath = imagePath;

    m_bitmapHeaderChar = LoadBitmapHeader();
    m_bitmapFileHeader = LoadBitmapFileImageHeader();

    //Check if this is a Bitmap image
    if ("BM" != getSignatureString())
    {
        printf("ERROR: Cannot process non-bitmap image files!\n");
        assert(0);
    }

    m_bitmapInfoHeader = LoadBitmapInfoImageHeader();
    m_bitmapImageChar = LoadBitmapImagePixels();

    // Modified buffers. To be used if required
    m_modifiedBitmapHeaderChar = nullptr;
    m_modifiedBitmapImageChar = nullptr;
    m_modifiedImageSize = 0;

    // Prepare histogram from fetched data
    this->prepareHistogram();

    assert(m_bitmapFileHeader);
    assert(m_bitmapInfoHeader);
}

//******************************************************************************************
// @name                    : ~BitmapImage
//
// @description             : Destructor
//
// @returns                 : Nothing
//********************************************************************************************
BitmapImage::~BitmapImage()
{
    // Free memory
    FreeMemory(m_bitmapHeaderChar);
    FreeMemory(m_bitmapImageChar);
    FreeMemory(m_modifiedBitmapHeaderChar);
    FreeMemory(m_modifiedBitmapImageChar);
    FreeMemory(m_bitmapFileHeader);
    FreeMemory(m_bitmapInfoHeader);

    // Close files
    CloseFile(m_inputFilePointer);
}

//******************************************************************************************
// @name                    : LoadBitmapHeader
//
// @description             : Extracts Bitmap image header from image 
//                            containing InfoHeader and FileHeader to a character array.
//
// @returns                 : Pointer to character array 
//********************************************************************************************
char* BitmapImage::LoadBitmapHeader()
{
    printf("\nReading Bitmap header...\n");

    char *bitmap_header = (char *)malloc(sizeof(char) * (BITMAP_HEADER_SIZE +1));
    
    if (!bitmap_header)
    {
        printf("ERROR: Malloc Failure!\n");
        assert(0);
    }

    memset(bitmap_header, 0, sizeof(bitmap_header));
    fread(bitmap_header, sizeof(char), BITMAP_HEADER_SIZE, m_inputFilePointer);

    return bitmap_header;
}

//******************************************************************************************
// @name                    : LoadBitmapFileImageHeader
//
// @description             : Stores the FileHeader to a structure
//
// @returns                 : Pointer to FileHeader
//********************************************************************************************
bitmap_file_header_t* BitmapImage::LoadBitmapFileImageHeader()
{
    printf("\nReading Bitmap File header...\n");

    bitmap_file_header_t *file_header = (bitmap_file_header_t *)malloc(sizeof(bitmap_file_header_t));
    if (!file_header)
    {
        printf("ERROR: Malloc Failure!\n");
        assert(0);
    }

    memset(file_header, 0, sizeof(file_header));

    // Populate the file header structure
    file_header->signature = *(short*)&m_bitmapHeaderChar[SIGNATURE];
    file_header->fileSize = *(int*)&m_bitmapHeaderChar[FILE_SIZE];
    file_header->dataOffset = *(int*)&m_bitmapHeaderChar[DATA_OFFSET];

    return file_header;
}

//******************************************************************************************
// @name                    : LoadBitmapInfoImageHeader
//
// @description             : Stores the InfoHeader to a structure
//
// @returns                 : Pointer to InfoHeader
//********************************************************************************************
bitmap_info_header_t* BitmapImage::LoadBitmapInfoImageHeader()
{
    printf("\nReading Bitmap Info header...\n");

    bitmap_info_header_t *info_header = (bitmap_info_header_t *)malloc(sizeof(bitmap_info_header_t));
    if (!info_header)
    {
        printf("ERROR: Malloc Failure!\n");
        assert(0);
    }

    memset(info_header, 0, sizeof(info_header));

    // Populate the info header structure
    info_header->infoHeaderSize = *(int*)&m_bitmapHeaderChar[INFO_HEADER_SIZE];
    info_header->width = *(int*)&m_bitmapHeaderChar[WIDTH];
    info_header->height = *(int*)&m_bitmapHeaderChar[HEIGHT];
    info_header->planes = *(short*)&m_bitmapHeaderChar[PLANES];
    info_header->bitsPerPixel = *(short*)&m_bitmapHeaderChar[BITS_PER_PIXEL];
    info_header->compressionType = *(int*)&m_bitmapHeaderChar[COMPRESSION_TYPE];
    info_header->compressedImageSize = *(int*)&m_bitmapHeaderChar[COMPRESSED_IMAGE_SIZE];
    info_header->xPixelsPerMeter = *(int*)&m_bitmapHeaderChar[X_PIXELS_PER_METER];
    info_header->yPixelsPerMeter = *(int*)&m_bitmapHeaderChar[Y_PIXELS_PER_METER];
    info_header->colorsUsed = *(int*)&m_bitmapHeaderChar[COLORS_USED];
    info_header->importantColors = *(int*)&m_bitmapHeaderChar[IMPORTANT_COLORS];

    // ColorTable
    info_header->redIntensity = *(char*)&m_bitmapHeaderChar[RED_INTENSITY];
    info_header->greenIntensity = *(char*)&m_bitmapHeaderChar[GREEN_INTENSITY];
    info_header->blueIntensity = *(char*)&m_bitmapHeaderChar[BLUE_INTENSITY];

    m_imageSize = info_header->width * info_header->height;
    //m_imageSize = m_bitmapFileHeader->fileSize - m_bitmapFileHeader->dataOffset;

    return info_header;
}

//******************************************************************************************
// @name                    : LoadBitmapImagePixels
//
// @description             : Loads the actual image data (pixel values)
//
// @returns                 : Pointer to image data.
//********************************************************************************************
unsigned char* BitmapImage::LoadBitmapImagePixels()
{
    // If required, read the 1024-byte from fp to colorTable
    char colorTable[COLOR_TABLE_SIZE + 1];
    if (m_bitmapInfoHeader->bitsPerPixel <= BITS_8_PALLETIZED) // Color table present
    {
        printf("\nReading color table...\n");
        fread(colorTable, sizeof(unsigned char), COLOR_TABLE_SIZE, m_inputFilePointer);
        // TODO: What to do of it??
    }

    printf("\nReading Bitmap pixels...\n");
    m_paddedWidth = (m_bitmapInfoHeader->width * 3 + 3) & (~3); // padded row length
    int bytesRead = 0;
    int totalBytesRead = 0;
    m_paddedImageSize = m_paddedWidth * m_bitmapInfoHeader->height;

    unsigned char *bitmap_pixels = (unsigned char *)malloc(sizeof(unsigned char) * (m_paddedImageSize));
    if (!bitmap_pixels)
    {
        printf("ERROR: Malloc Failure!\n");
        assert(0);
    }

    memset(bitmap_pixels, 0, m_paddedImageSize);
    bytesRead = fread(bitmap_pixels, sizeof(unsigned char), m_paddedImageSize, m_inputFilePointer);

#if 0
    for (int i = 0; i < m_bitmapInfoHeader->height; i++)
    {
        bytesRead = fread(&bitmap_pixels[m_paddedImageSize - m_paddedWidth], sizeof(unsigned char), m_paddedWidth, m_inputFilePointer);
        totalBytesRead += bytesRead;
        // TODO: We can prepare a 2-D pixel array here
    }
#endif

    return bitmap_pixels;
}

//******************************************************************************************
// @name                    : getBitsPerPixelInfoFromNumber
//
// @description             : 
//
// @param val               : BitsPerPixel/ImageDepth from image header
//
// @returns                 : Description of this parameter in string format
//********************************************************************************************
char* BitmapImage::getBitsPerPixelInfoFromNumber(short val)
{
    if (val == MONOCHROME)
    {
        return "Monochrome Palette. Number of colors: 1";
    }
    else if (val == BITS_4_PALLETIZED)
    {
        return "4-bits palletized. Number of colors: 16";
    }
    else if (val == BITS_8_PALLETIZED)
    {
        return "8-bits palletized. Number of colors: 256";
    }
    else if (val == BITS_16_RGB)
    {
        return "16-bits RGB. Number of colors: 65,536";
    }
    else if (val == BITS_24_RGB)
    {
        return "24-bits RGB. Number of colors: 16 million";
    }
    else
    {
        return "Invalid BitsPerPixel value!";
    }
}

//******************************************************************************************
// @name                    : getBitsCompressionTypeFromNumber
//
// @description             : 
//
// @param val               : Compression Type value from image header
//
// @returns                 : Description of compression type
//********************************************************************************************
char* BitmapImage::getBitsCompressionTypeFromNumber(int val)
{
        if (val == COMPRESSION_RGB)
        {
            return "No Compression";
        }
        else if (val == COMPRESSION_RLE8)
        {
            return "8-bits RLE encoding";
        }
        else if (val == COMPRESSION_RLE4)
        {
            return "4-bits RLE encoding";
        }
        else
        {
            return "Invalid CompressionType value!";
        }
}

//******************************************************************************************
// @name                    : getSignatureString
//
// @description             : Get signature text from its coded value.
//
// @returns                 : Human readable text form of Signature specified in image header
//********************************************************************************************
string BitmapImage::getSignatureString()
{
    char sig[3];
    sig[0] = *(char*)&m_bitmapHeaderChar[SIGNATURE];
    sig[1] = *(char*)&m_bitmapHeaderChar[SIGNATURE + 1];
    sig[2] = '\0';
    string signature(sig);

    return signature;
}

//******************************************************************************************
// @name                    : displayImageDetails
//
// @description             : Display image header information.
//
// @returns                 : Nothing
//********************************************************************************************
void BitmapImage::displayImageDetails()
{
    printf("\nImage path: %s\n", m_imagePath.c_str());
    printf("-------------------------------------------------------------\n");
    printf("Header:\n");
    printf("-------------------------------------------------------------\n");
    printf("Signature             : %s\n", (getSignatureString().c_str()));
    printf("FileSize              : %d bytes\n", m_bitmapFileHeader->fileSize);
    printf("Data Offset           : %d\n", m_bitmapFileHeader->dataOffset);

    printf("\n-------------------------------------------------------------\n");
    printf("InfoHeader:\n");
    printf("-------------------------------------------------------------\n");
    printf("infoHeaderSize        : %d\n", m_bitmapInfoHeader->infoHeaderSize);
    printf("Width                 : %d pixels\n", m_bitmapInfoHeader->width);
    printf("Height                : %d pixels\n", m_bitmapInfoHeader->height);
    printf("Planes                : %d\n", m_bitmapInfoHeader->planes);
    printf("Bits Per Pixel        : %s\n", getBitsPerPixelInfoFromNumber(m_bitmapInfoHeader->bitsPerPixel));
    printf("Compression           : %s\n", getBitsCompressionTypeFromNumber(m_bitmapInfoHeader->compressionType));
    printf("compressedImageSize   : %d bytes\n", m_bitmapInfoHeader->compressedImageSize);
    printf("x_pixelsPerMeter      : %d\n", m_bitmapInfoHeader->xPixelsPerMeter);
    printf("y_pixelsPerMeter      : %d\n", m_bitmapInfoHeader->xPixelsPerMeter);
    printf("Colors used           : %d\n", m_bitmapInfoHeader->colorsUsed);
    printf("Important colors      : %d\n", m_bitmapInfoHeader->importantColors);

    printf("\n-------------------------------------------------------------\n");
    printf("ColorTable:\n");
    printf("-------------------------------------------------------------\n");
    printf("Red intensity         : %d\n", m_bitmapInfoHeader->redIntensity);
    printf("Green intensity       : %d\n", m_bitmapInfoHeader->greenIntensity);
    printf("Blue intensity        : %d\n", m_bitmapInfoHeader->blueIntensity);
}

//******************************************************************************************
// @name                    : displayImagePixels
//
// @description             : 
//
// @returns                 : Nothing
//********************************************************************************************
void BitmapImage::displayImagePixels()
{
    printf("\n\n-------------------------------------------------------------\n");
    printf("Image Pixels Information:\n");
    printf("-------------------------------------------------------------\n");

    // For color image (rgb)
    // Formula: val(i,j) = imgArray[width * i + j]
    //                   = pixelValue[i][j] = m_bitmapImageChar[m_paddedWidth * i + j];

    for (int i = 0; i < m_bitmapInfoHeader->height; i++)
    {
        int j = 0;
        while (j < m_paddedWidth)
        {
            if (j >= (m_bitmapInfoHeader->width * 3))
            {
                // Reached end of pixels in a row. Rest of the values
                // in this row are padding
                break;
            }

            unsigned char *bluePixelValue  = &m_bitmapImageChar[m_paddedWidth * i + j++];
            unsigned char *greenPixelValue = &m_bitmapImageChar[m_paddedWidth * i + j++];
            unsigned char *redPixelValue   = &m_bitmapImageChar[m_paddedWidth * i + j++];

            printf("(%02d,%02d,%02d) ", *redPixelValue, *greenPixelValue, *bluePixelValue);
        }
    }
}

//******************************************************************************************
// @name                    : prepareHistogram
//
//@description              : Prepares histogram of input image
//
// @returns                 : Nothing
//********************************************************************************************
void BitmapImage::prepareHistogram()
{
    printf("\nPreparing histogram information...\n");
    for (int i = 0; i < m_bitmapInfoHeader->height; i++)
    {
        int j = 0;
        while (j < m_paddedWidth)
        {
            if (j >= (m_bitmapInfoHeader->width * 3))
            {
                // Reached end of pixels in a row. Rest of the values
                // in this row are padding
                break;
            }

            pixel_value_rbg_t pixel_value_rgb;
            pixel_value_rgb.blue  = m_bitmapImageChar[m_paddedWidth * i + j++];
            pixel_value_rgb.green = m_bitmapImageChar[m_paddedWidth * i + j++];
            pixel_value_rgb.red   = m_bitmapImageChar[m_paddedWidth * i + j++];

            // RGB histograms
            m_redHistogram[pixel_value_rgb.red]++;
            m_greenHistogram[pixel_value_rgb.green]++;
            m_blueHistogram[pixel_value_rgb.blue]++;

            // Brightness histogram
            pixel_value_ycbcr_t pixel_value_ycbcr;
            pixel_value_ycbcr = convertToYCbCr(pixel_value_rgb);

            m_brightnessHistogram[pixel_value_ycbcr.y]++;
        }
    }
}

//******************************************************************************************
// @name                    : displayHistogram
//
//@description              : Displays the histogram
//
// @returns                 : Nothing
//********************************************************************************************
void BitmapImage::displayHistogram()
{
    printf("\n\n------------------------------------------------------------------------------------\n");
    printf("H I S T O G R A M:  ");
    printf("Intensity Level - Number of pixels at that intenstiy level\n");
    printf("------------------------------------------------------------------------------------\n");
    for (int i = 0; i < MAX_COLORS; i++)
    {
        
        unsigned long scaledDownPixelValueCount = 0;
        unsigned long redPixelCount   = m_redHistogram[i];
        unsigned long greenPixelCount = m_greenHistogram[i];
        unsigned long bluePixelCount  = m_blueHistogram[i];

        printf("%03d:", i);
        scaledDownPixelValueCount = redPixelCount;// (HISTOGRAM_SCALING_FACTOR * redPixelCount) / m_imageSize;
        for (unsigned long i = 0; i < scaledDownPixelValueCount; i++)
        {
            printf("R");
        }
        printf("\n");

        printf("%03d:", i);
        scaledDownPixelValueCount = greenPixelCount;// (HISTOGRAM_SCALING_FACTOR * greenPixelCount) / m_imageSize;
        for (unsigned long i = 0; i < scaledDownPixelValueCount; i++)
        {
            printf("G");
        }
        printf("\n");

        printf("%03d:", i);
        scaledDownPixelValueCount = bluePixelCount;// (HISTOGRAM_SCALING_FACTOR * bluePixelCount) / m_imageSize;
        for (unsigned long i = 0; i < scaledDownPixelValueCount; i++)
        {
            printf("B");
        }
        printf("\n");
    }
}

//******************************************************************************************
// @name                    : writeModifiedImageDataToFile
//
//@description              : Write the modified image to output file. If there is no 
//                            modification to the image, then make a copy of the original
//                            image and write to outputFilePath
//
// @param outputFilePath    : Path of file
//
// @returns                 : Error value
//********************************************************************************************
int BitmapImage::writeModifiedImageDataToFile(const char *outputFilePath)
{
    int retval = -1;

    if (!outputFilePath)
    {
        printf("Output file path not specified!\n");
        assert(0);
    }

    // Open file for writing
    FILE *outfile = fopen(outputFilePath, "wb");
    if (outfile == nullptr)
    {
        printf("Cannot create file [%s]\n", outputFilePath);
    }

    // Write header
    m_modifiedBitmapHeaderChar = (char *)malloc(BITMAP_HEADER_SIZE + 1);
    if (m_modifiedBitmapHeaderChar == nullptr)
    {
        printf("ERROR: Malloc Failure!\n");
        assert(0);
    }

    memset(m_modifiedBitmapHeaderChar, 0, BITMAP_HEADER_SIZE + 1);
    memcpy(m_modifiedBitmapHeaderChar, m_bitmapHeaderChar, BITMAP_HEADER_SIZE);

    retval = fwrite(m_modifiedBitmapHeaderChar, sizeof(char), BITMAP_HEADER_SIZE, outfile);
    if (retval == 0)
    {
        printf("ERROR: Header write error!\n");
        assert(0);
    }

    // No modified image. Simply write the same image to output file.
    if (m_modifiedBitmapImageChar == nullptr)
    {
        this->allocateModifiedImageBuffer();
        printf("\nINFO: No modification to image. Making copy of original\n");
    }

    // Write modified image data
    retval = fwrite(m_modifiedBitmapImageChar, sizeof(unsigned char), m_paddedImageSize, outfile);
    if (retval == 0)
    {
        printf("ERROR: Content write error!\n");
        assert(0);
    }

    CloseFile(outfile);

    return 0;
}

//******************************************************************************************
// @name                    : allocateModifiedImageBuffer
//
//@description              : Allocate memory to modified image buffer. The image size is
//                            same as the original image.
//
// @returns                 : Nothing
//********************************************************************************************
void BitmapImage::allocateModifiedImageBuffer()
{
    // Allocate memory only if not already allocated
    if (m_modifiedBitmapImageChar == nullptr)
    {
        m_modifiedBitmapImageChar = (unsigned char *)malloc(m_paddedImageSize);
        if (m_modifiedBitmapImageChar == nullptr)
        {
            printf("ERROR: Malloc Failure!\n");
            assert(0);
        }
    }

    m_modifiedImageSize = m_imageSize;  // Same size image. Not used as of now
    memset(m_modifiedBitmapImageChar, 0, m_paddedImageSize);
    memcpy(m_modifiedBitmapImageChar, m_bitmapImageChar, m_paddedImageSize);

}

//******************************************************************************************
// @name                    : ConvertToGrayScale
//
//@description              : Transform the RGB image to a GrayScale image
//
// @returns                 : 0 if SUCCESS
//********************************************************************************************
int BitmapImage::ConvertToGrayScale()
{
    int i = 0;
    int retval = -1;

    this->allocateModifiedImageBuffer();

    for (int i = 0; i < m_bitmapInfoHeader->height; i++)
    {
        int j = 0;
        while (j < m_paddedWidth)
        {
            if (j >= (m_bitmapInfoHeader->width * 3))
            {
                // Reached end of pixels in a row. Rest of the values
                // in this row are padding
                break;
            }

            unsigned char* red;
            unsigned char* green;
            unsigned char* blue;

            blue  = &m_modifiedBitmapImageChar[m_paddedWidth * i + j++];
            green = &m_modifiedBitmapImageChar[m_paddedWidth * i + j++];
            red   = &m_modifiedBitmapImageChar[m_paddedWidth * i + j++];

            pixel_value_rbg_t pixel_value_rgb;
            pixel_value_rgb.red = *red;
            pixel_value_rgb.green = *green;
            pixel_value_rgb.blue = *blue;

            pixel_value_ycbcr_t pixel_value_ycbcr;

            pixel_value_ycbcr = convertToYCbCr(pixel_value_rgb);

            // Do modification to individual pixels here
            *red = pixel_value_ycbcr.y;
            *green = pixel_value_ycbcr.y;
            *blue = pixel_value_ycbcr.y;

        }
    }

    return 0;
}


//******************************************************************************************
// @name                    : doHistogramEqualization
//
//@description              : Do equalization and save to modified image buffer.
//
// @returns                 : return value
//********************************************************************************************
int BitmapImage::doHistogramEqualization()
{
    int i = 0;
    int retval = -1;

    this->allocateModifiedImageBuffer();

    // Probability table
    double probabilityTableRed[MAX_COLORS];
    double probabilityTableGreen[MAX_COLORS];
    double probabilityTableBlue[MAX_COLORS];
    double probabilityTableBrightness[MAX_COLORS];
    for (int i = 0; i < MAX_COLORS; i++)
    {
        probabilityTableRed[i]        = (double)m_redHistogram[i] / m_imageSize;
        probabilityTableGreen[i]      = (double)m_greenHistogram[i] / m_imageSize;
        probabilityTableBlue[i]       = (double)m_blueHistogram[i] / m_imageSize;
        probabilityTableBrightness[i] = (double)m_brightnessHistogram[i] / m_imageSize;
    }

    // Cumulative Distribution Function
    double cdfRed[MAX_COLORS];
    double cdfGreen[MAX_COLORS];
    double cdfBlue[MAX_COLORS];
    double cdfBrightness[MAX_COLORS];

    cdfRed[0]   = probabilityTableRed[0];
    cdfGreen[0] = probabilityTableGreen[0];
    cdfBlue[0]  = probabilityTableBlue[0];
    cdfBrightness[0] = probabilityTableBrightness[0];

    for (int i = 1; i < MAX_COLORS; i++)
    {
        cdfRed[i]   = probabilityTableRed[i] + cdfRed[i - 1];
        cdfGreen[i] = probabilityTableGreen[i] + cdfGreen[i - 1];
        cdfBlue[i]  = probabilityTableBlue[i] + cdfBlue[i - 1];
        cdfBrightness[i] = probabilityTableBrightness[i] + cdfBrightness[i - 1];
    }

    // Pixel processing for histogram equalization
    for (int i = 0; i < m_bitmapInfoHeader->height; i++)
    {
        int j = 0;
        while (j < m_paddedWidth)
        {
            if (j >= (m_bitmapInfoHeader->width * 3))
            {
                // Reached end of pixels in a row. Rest of the values
                // in this row are padding
                break;
            }

            unsigned char* red;
            unsigned char* green;
            unsigned char* blue;
            
            blue  = &m_modifiedBitmapImageChar[m_paddedWidth * i + j++];
            green = &m_modifiedBitmapImageChar[m_paddedWidth * i + j++];
            red   = &m_modifiedBitmapImageChar[m_paddedWidth * i + j++];

            pixel_value_rbg_t pixel_value_rgb;
            pixel_value_rgb.red = *red;
            pixel_value_rgb.green = *green;
            pixel_value_rgb.blue = *blue;

#ifdef USE_BRIGHTNESS_LEVEL_FOR_HISTOGRAM_EQUALIZATION
            // Obtain brightness level and do equalization on that.
            pixel_value_ycbcr_t pixel_value_ycbcr;
            pixel_value_ycbcr = convertToYCbCr(pixel_value_rgb);
            unsigned char brightness = pixel_value_ycbcr.y;
            brightness = cdfBrightness[brightness] * (MAX_COLORS - 1);
            pixel_value_ycbcr.y = cdfBrightness[brightness] * (MAX_COLORS - 1);

            // Convert this to RGB
            pixel_value_rgb = convertToRGB(pixel_value_ycbcr);
#else
            pixel_value_rgb.red = cdfRed[pixel_value_rgb.red] * (MAX_COLORS - 1);
            pixel_value_rgb.green = cdfGreen[pixel_value_rgb.green] * (MAX_COLORS - 1);
            pixel_value_rgb.blue = cdfBlue[pixel_value_rgb.blue] * (MAX_COLORS - 1);
#endif
            // Do modification to individual pixels here
            *red = pixel_value_rgb.red;
            *green = pixel_value_rgb.green;
            *blue = pixel_value_rgb.blue;
        }
    }

    return 0;
}

pixel_value_ycbcr_t BitmapImage::convertToYCbCr(pixel_value_rbg_t pixelValue)
{
    pixel_value_ycbcr_t pixelYCbCr;
    int r = pixelValue.red;
    int g = pixelValue.green;
    int b = pixelValue.blue;

    /*pixelYCbCr.y = ((0.299 * r) + (0.587 * g) + (0.114 * b));
    pixelYCbCr.Cb = 128 - ((0.168736 * r) - (0.331364 * g) + (0.5 * b));
    pixelYCbCr.Cr = 128 + ((0.5 * r) - (0.418688 * g) - (0.081312 * b));*/

    // As per Recommendation ITU-R BT.601”[6]
    int y = ((0.257 * r) + (0.504 * g) + (0.098 * b) + 16);
    int Cb = ((-0.148 * r) - (0.291 * g) + (0.439 * b) + 128);
    int Cr = ((0.439 * r) - (0.386 * g) - (0.071 * b) + 128);

    // Clamping on YCbCr values
    if (y > MAX_COLORS - 1)
        y = MAX_COLORS - 1;

    if (y < 0)
        y = 0;

    if (Cb > MAX_COLORS - 1)
        Cb = MAX_COLORS - 1;

    if (Cb < 0)
        Cb = 0;

    if (Cr > MAX_COLORS - 1)
        Cr = MAX_COLORS - 1;

    if (Cr < 0)
        Cr = 0;

    // Use clamped values
    pixelYCbCr.y = y;
    pixelYCbCr.Cb = Cb;
    pixelYCbCr.Cr = Cr;

    return pixelYCbCr;
}

pixel_value_rbg_t BitmapImage::convertToRGB(pixel_value_ycbcr_t pixelYCbCr)
{
    pixel_value_rbg_t pixelValue;
    int y = pixelYCbCr.y;
    int Cb = pixelYCbCr.Cb;
    int Cr = pixelYCbCr.Cr;

    /*pixelValue.red = y + 1.402 * (Cr - 128);
    pixelValue.green = y - 0.34414 * (Cb - 128) - 0.71414 * (Cr - 128);
    pixelValue.blue = y + 1.772 * (Cb - 128);*/

    // As per Recommendation ITU-R BT.601”[6]
    pixelValue.red = (1.164 * (y - 16)) + (1.596 * (Cr - 128));
    pixelValue.green = (1.164 * (y - 16)) - (0.391 * (Cb - 128)) - (0.813 * (Cr - 128));
    pixelValue.blue = (1.164 * (y - 16)) + (2.018 * (Cb - 128));

    // Clamping on RGB values
    if (pixelValue.red > MAX_COLORS - 1)
        pixelValue.red = MAX_COLORS - 1;

    if (pixelValue.red < 0)
        pixelValue.red = 0;

    if (pixelValue.green > MAX_COLORS - 1)
        pixelValue.green = MAX_COLORS - 1;

    if (pixelValue.green < 0)
        pixelValue.green = 0;

    if (pixelValue.blue > MAX_COLORS - 1)
        pixelValue.blue = MAX_COLORS - 1;

    if (pixelValue.blue < 0)
        pixelValue.blue = 0;

    return pixelValue;
}