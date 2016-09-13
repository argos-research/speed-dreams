/***************************************************************************
                          guitexture.cpp -- Images manipulation
                             -------------------
    created              : Tue Aug 17 20:13:08 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: guitexture.cpp 6393 2016-03-27 16:23:23Z beaglejoe $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
    		Images manipulation tools.
		Load and store png images with easy interface.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: guitexture.cpp 6393 2016-03-27 16:23:23Z beaglejoe $
    @ingroup	img		
*/

#include <cstdlib>
#include <cstdio>

#ifdef WIN32
#include <direct.h>
#define XMD_H
#endif

#include <png.h>
extern "C"
{
    #include <jpeglib.h>
}

#include <portability.h> // snprintf

#include "tgfclient.h"
#include "glfeatures.h"


unsigned gfTexGetClosestGreaterPowerOf2(unsigned nSize)
{
	unsigned nPow2Size = 2;
	
	while (nPow2Size < nSize)
		nPow2Size *= 2;

	return nPow2Size;
}

// Previous implementation, with strange behaviour that need to be checked / explained :
// - why limit texture size to 1024 ? why not try and detect tha actual hardware limit ?
// - why not all possible power of 2 values supported (see the sizes array) ?
// unsigned gfTexGetClosestGreaterPowerOf2(unsigned nSize)
// {
// 	static const int nSizes = 7;
// 	static const unsigned sizes[nSizes] = { 2, 4, 16, 128, 256, 512, 1024 };

// 	for (int i = 0; i < nSizes; i++)
// 	{
// 		if (size <= sizes[i])
// 			return sizes[i];
// 	}

// 	// Do not allow textures larger than this for memory usage and performance reasons
// 	return sizes[nSizes - 1];
// }


#ifdef Useful
void
GfTexScaleImagePowerof2(unsigned char *pSrcImg, int srcW, int srcH, GLenum format, unsigned &texId)
{
	int destH = 128;
	int destW = 128;

	destH = (int)gfTexGetClosestGreaterPowerOf2((unsigned)srcH);
	destW = (int)gfTexGetClosestGreaterPowerOf2((unsigned)srcW);

	if ( destH != srcH || destW != srcW)
	{
	
		unsigned char *texData = NULL;
		if (format == GL_RGB)
		{
			texData = new unsigned char[destW*destH*3];
		}
		else if(format == GL_RGBA)
		{
			texData = new unsigned char[destW*destH*4];

		}

		int r = gluScaleImage(format, srcW,srcH,GL_UNSIGNED_BYTE,(void*)pSrcImg,destW,destH,GL_UNSIGNED_BYTE,(void*)texData);
		if (r!=0)
			GfError("GfTexScaleImagePowerof2 : Error trying to scale image\n");

		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGBA, destW, destH, 0, format, GL_UNSIGNED_BYTE, (GLvoid *)(texData));
		delete [] texData;
	}
	else
	{
		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, format, destW, destH, 0, format, GL_UNSIGNED_BYTE, (GLvoid *)(pSrcImg));
	}
}
#endif

/** Load a PNG image from disk to a buffer in RGBA mode (if specified, enforce 2^N x 2^P size for the target buffer, to suit with low-end OpenGL hardwares/drivers poor texture support).
    @ingroup	img		
    @param	filename	name of the image to load
    @param	screen_gamma	gamma correction value
    @param	pWidth		original width of the read image (left aligned in target buffer)
    @param	pHeight		original height of the read image (top aligned in target buffer)
    @param	pPow2Width	if not 0, pointer to 2^N width of the target image buffer
    @param	pPow2Height	if not 0, pointer to 2^N height of the target image buffer
    @return	Pointer on the buffer containing the image
		<br>NULL Error
 */
unsigned char *
GfTexReadImageFromPNG(const char *filename, float screen_gamma, int *pWidth, int *pHeight,
					  int *pPow2Width, int *pPow2Height, bool useGammaCorrection)
{
	static const int nPNGBytesToCheck = 4;

	unsigned char buf[nPNGBytesToCheck];
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 src_width, src_height;
	png_uint_32 tgt_width, tgt_height;
	int bit_depth, color_type, interlace_type;
	
	double gamma;
	png_bytep *row_pointers;
	unsigned char *image_ptr, *cur_ptr;
	png_uint_32 src_rowbytes, tgt_rowbytes;
	png_uint_32 i;

	//const double dStartTime = GfTimeClock(); // Start measuring elapsed time.

	// Check if we can open the image file.
	if ((fp = fopen(filename, "rb")) == NULL) {
		GfError("GfTexReadImageFromPNG(%s) : Can't open file for reading\n", filename);
		return (unsigned char *)NULL;
	}
	
	// Check if we can read from it.
	if (fread(buf, 1, nPNGBytesToCheck, fp) != (int)nPNGBytesToCheck) {
		GfError("GfTexReadImageFromPNG(%s) : Can't read file\n", filename);
		fclose(fp);
		return (unsigned char *)NULL;
	}
	
	// Check if really in PNG format.
	if (png_sig_cmp(buf, (png_size_t)0, nPNGBytesToCheck) != 0) {
		GfError("GfTexReadImageFromPNG(%s) : File not in png format\n", filename);
		fclose(fp);
		return (unsigned char *)NULL;
	}

	//  Setup PNG reader.
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
									 (png_error_ptr)NULL, (png_error_ptr)NULL);
	if (png_ptr == NULL) {
		GfError("GfTexReadImageFromPNG(%s) : Failed to create read_struct\n", filename);
		fclose(fp);
		return (unsigned char *)NULL;
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return (unsigned char *)NULL;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) // Compiler warning: not portable mix of setjmp and C++ object delete
	{
		// Free all of the memory associated with the png_ptr and info_ptr
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		// If we get here, we had a problem reading the file
		return (unsigned char *)NULL;
	}
	
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, nPNGBytesToCheck);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &src_width, &src_height,
				 &bit_depth, &color_type, &interlace_type, NULL, NULL);

	if (bit_depth == 1 && color_type == PNG_COLOR_TYPE_GRAY) 
	    png_set_invert_mono(png_ptr);

	if (bit_depth == 16) {
		png_set_swap(png_ptr);
		png_set_strip_16(png_ptr);
	}
	
	if (bit_depth < 8) {
		png_set_packing(png_ptr);
	}
	
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_expand(png_ptr);
	}
	
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand(png_ptr);
	}
	
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_set_expand(png_ptr);
	}
	
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}
	
	if (bit_depth == 8 && color_type == PNG_COLOR_TYPE_RGB) {
		png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	}
	
	if(useGammaCorrection){
		if (png_get_gAMA(png_ptr, info_ptr, &gamma)) {
			png_set_gamma(png_ptr, screen_gamma, gamma);
		} else {
			png_set_gamma(png_ptr, screen_gamma, 0.50);
		}
	}

	// Store read image width and height.
	*pWidth = (int)src_width;
	*pHeight = (int)src_height;
	
	// Compute and store target 2^N x 2^P image buffer size if specified.
	// Note: This 2^N x 2^P stuff is needed by some low-end OpenGL hardwares/drivers
	//       that don't support non 2^N x 2^P textures (or at extremely low frame rates).
	if (pPow2Width)	{
	    tgt_width = gfTexGetClosestGreaterPowerOf2(src_width);
		*pPow2Width = (int)tgt_width;
	} else {
	    tgt_width = (int)src_width;
	}
	
	if (pPow2Height)
	{
	    tgt_height = gfTexGetClosestGreaterPowerOf2(src_height);
		*pPow2Height = (int)tgt_height;
	} else {
	    tgt_height = (int)src_height;
	}
		
	// Read image row size from the file
	png_read_update_info(png_ptr, info_ptr);
	src_rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	tgt_rowbytes = src_rowbytes;
	if (pPow2Width && pPow2Height) 
	    tgt_rowbytes = tgt_width * src_rowbytes / src_width;
	
	// Check if we have really a RGBA file (nothing else supported).
	if (src_rowbytes != (4 * src_width)) {
		GfError("GfTexReadImageFromPNG(%s) : Bad byte count (%lu instead of %lu)\n", 
            filename, (unsigned long)src_rowbytes, (unsigned long)(4 * src_width));
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return (unsigned char *)NULL;
	}

	// Allocate and target memory buffers for the image.
	row_pointers = (png_bytep*)malloc(tgt_height * sizeof(png_bytep));
	if (row_pointers == NULL) {
		GfError("GfTexReadImageFromPNG(%s) : Failed to allocate row pointers table (%ul bytes)\n", 
				filename, (unsigned long)(tgt_height * sizeof(png_bytep)));
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return (unsigned char *)NULL;
	}
	
	image_ptr = (unsigned char *)malloc(tgt_height * tgt_rowbytes);
	if (image_ptr == NULL) {
		GfError("GfTexReadImageFromPNG(%s) : Failed to allocate pixel buffer (%ul bytes)\n", 
				filename, (unsigned long)(tgt_height * tgt_rowbytes));
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return (unsigned char *)NULL;
	}

	// Initialize row pointers table
	for (i = 0, cur_ptr = image_ptr + (tgt_height - 1) * tgt_rowbytes ; 
	     i < tgt_height; i++, cur_ptr -= tgt_rowbytes) {
		row_pointers[i] = cur_ptr;
	}

	// Initialize pixel buffer if padding is needed (as transparent pixels)
	// (as some pixels in the buffer won't be written by the PNG reader,
	//  we have to initialize them, and force at least the alpha channel to 0 = transparent).
	// TODO (Optimization) : Only initialize the pixels that are not overwritten by png_read_image.
	if (tgt_width > src_width || tgt_height > src_height)
		memset(image_ptr, 0x00, tgt_height * tgt_rowbytes);
	png_read_image(png_ptr, row_pointers);

	// Free allocated memory (except for the image buffer !).
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	free(row_pointers);

	// Close source file.
	fclose(fp);
	
	//GfLogDebug("GfTexReadImageFromPNG(%s) : %.3f s)\n", filename, GfTimeClock() - dStartTime);
	
	return image_ptr;
}

/** Load a JPEG image from disk to a buffer in RGBA 8888 mode 
    @ingroup	img		
    @param	filename	 name of the image to load
    @param	screen_gamma gamma correction value
    @param	pWidth		 original width of the read image (left aligned in target buffer)
    @param	pHeight		 original height of the read image (top aligned in target buffer)
    @param	pPow2Width	 if not 0, pointer to 2^N width of the target image buffer (if not equal to original width, transparent pixels are added as necessary, on the left side of the image)
    @param	pPow2Height	 if not 0, pointer to 2^N height of the target image buffer (if not equal to original height, transparent pixels are added as necessary, on the bottom side of the image)
                         WARNING: Not yet implemented ; = *pHeight
    @return	Pointer to the buffer RGBA 8888 containing the image
		<br>NULL Error
 */
struct gfTexJPEGErrorManager {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
gfTexJPEGErrorExit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a jpeg_error_mgr, so coerce pointer */
	struct gfTexJPEGErrorManager* pErrMgr =
		(struct gfTexJPEGErrorManager*)cinfo->err;
	
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);
	
	/* Return control to the setjmp point */
	longjmp(pErrMgr->setjmp_buffer, 1);
}

unsigned char *
GfTexReadImageFromJPEG(const char *filename, float screen_gamma, int *pWidth, int *pHeight, 
					   int *pPow2Width, int *pPow2Height)
{
	//const double dStartTime = GfTimeClock(); // Start measuring elapsed time.

	// Try and open the source image file.
	FILE* infile;
	if ((infile = fopen(filename, "rb")) == NULL) 
	{
		fprintf(stderr, "GfTexReadImageFromJPEG(%s) : Can't open file\n", filename);
		return 0;
	}

	// Initialize the error handler associated to the JPEG image decompressor.
	struct jpeg_decompress_struct cinfo;
	struct gfTexJPEGErrorManager jerr;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = gfTexJPEGErrorExit;

	if (setjmp(jerr.setjmp_buffer)) 
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return 0;
	}

	// Initialize the JPEG image decompressor for 24 bit RGB output.
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void)jpeg_read_header(&cinfo, TRUE);

	cinfo.out_color_space = JCS_RGB;
	cinfo.quantize_colors = FALSE;
	jpeg_calc_output_dimensions(&cinfo);
	
	// Load and check image dimensions.
	(void)jpeg_start_decompress(&cinfo);
	const unsigned nSrcWidth = cinfo.output_width;
	const unsigned nSrcHeight = cinfo.output_height;
	const unsigned nSrcBytesPerPixel = cinfo.output_components;
	//GfTrace("GfTexReadImageFromJPEG(%s) : %ux%ux%u\n",
	//		filename, nSrcWidth, nSrcHeight, nSrcBytesPerPixel);

	if (nSrcBytesPerPixel != 3)
	{
		(void)jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		fprintf(stderr, "GfTexReadImageFromJPEG(%s) : Unsupported %u bytes per pixel JPEG image\n",
				filename, nSrcBytesPerPixel);
		return 0;
	}

	// Store read image width and height.
	*pWidth = (int)nSrcWidth;
	*pHeight = (int)nSrcHeight;
	
	// Compute and store target 2^N x 2^P image buffer size if specified.
	// Note: This 2^N x 2^P stuff is needed by some low-end OpenGL hardwares/drivers
	//       that don't support non 2^N x 2^P textures (or at extremely low frame rates).
	unsigned nTgtWidth, nTgtHeight;
	if (pPow2Width)
	{
	    nTgtWidth = gfTexGetClosestGreaterPowerOf2(nSrcWidth);
		*pPow2Width = (int)nTgtWidth;
	}
	else
		nTgtWidth = nSrcWidth;
	if (pPow2Height)
	{
	    nTgtHeight = gfTexGetClosestGreaterPowerOf2(nSrcHeight);
		*pPow2Height = (int)nTgtHeight;
	}
	else
		nTgtHeight = nSrcHeight;

	// Allocate the image buffer (4 bytes per pixel as we add the alpha channel).
	const unsigned nTgtBytesPerPixel = 4;
	unsigned char *aTgtImageBuffer =
		(unsigned char*)malloc(nTgtHeight * nTgtWidth * nTgtBytesPerPixel);

	// Allocate the line buffer for the decompressor.
	JSAMPARRAY aSrcLineBuffer =
		(*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
								   nSrcWidth * nSrcBytesPerPixel, 1);

	// Initialize pixel buffer if padding is needed (as transparent pixels)
	// (as some pixels in the buffer won't be written by the JPEG reader,
	//  we have to initialize them, and force at least the alpha channel to 0 = transparent).
	// TODO (Optimization) : Only initialize the pixels that are not overwritten by the reader.
	if (nTgtWidth > nSrcWidth || nTgtHeight > nSrcHeight)
		memset(aTgtImageBuffer, 0x00, nTgtHeight * nTgtWidth * nTgtBytesPerPixel);

	// Load source pixel lines into the image buffer (from bottom to top),
	// and add the alpha channel to each pixel.
	JSAMPLE* aSrcPixelData;
	unsigned char* pTgtPixelData =
		aTgtImageBuffer + (nTgtHeight - 1) * nTgtWidth * nTgtBytesPerPixel;
	while (cinfo.output_scanline < cinfo.output_height) 
	{
		(void)jpeg_read_scanlines(&cinfo, aSrcLineBuffer, 1);
		
		for (unsigned nPixelIndex = 0; nPixelIndex < nSrcWidth; nPixelIndex++)
		{
			aSrcPixelData = &(aSrcLineBuffer[0][nPixelIndex * nSrcBytesPerPixel]);
			*pTgtPixelData++ = aSrcPixelData[0]; // Red channel.
			*pTgtPixelData++ = aSrcPixelData[1]; // Green channel.
			*pTgtPixelData++ = aSrcPixelData[2]; // Blue channel.
			*pTgtPixelData++ = 255; // No alpha channel in JPEG files : we force it to "opaque".
		}

		// Next line in target image buffer (reversed because of OpenGL / JPEG conventions).
		pTgtPixelData -= nTgtBytesPerPixel * (nTgtWidth + nSrcWidth);
	}
	
	// Close the JPEG image decompressor and source file.
	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	//GfLogDebug("GfTexReadImageFromJPEG(%s) : %.3f s)\n", filename, GfTimeClock() - dStartTime);
	
	return aTgtImageBuffer;
}

/** Load a PNG or JPEG image from disk to a buffer in RGBA mode (if specified, enforce 2^N x 2^P size for the target buffer, to suit with low-end OpenGL hardwares/drivers poor texture support).
    WARNING: 2^N x 2^P enforcement not yet implemented for JPEG images.
    @ingroup	img		
    @param	filename	name of the image to load
    @param	screen_gamma	gamma correction value
    @param	pWidth		if not 0, pointer to original width of the read image (left aligned in target buffer)
    @param	pHeight		if not 0, pointer to original height of the read image (top aligned in target buffer)
    @param	pPow2Width	if not 0, pointer to 2^N width of the target image buffer
    @param	pPow2Height	if not 0, pointer to 2^N height of the target image buffer
    @return	Pointer on the buffer containing the image
		<br>NULL Error
 */
unsigned char *
GfTexReadImageFromFile(const char *filename, float screen_gamma, int *pWidth, int *pHeight,
					   int *pPow2Width, int *pPow2Height)
{
	unsigned char* pImageBuffer = 0;
	
	if (strstr(filename, ".png") || strstr(filename, ".PNG"))
	{
	    pImageBuffer = GfTexReadImageFromPNG(filename, screen_gamma, pWidth, pHeight,
											 pPow2Width, pPow2Height);
	}
	else if (strstr(filename, ".jpg") || strstr(filename, ".JPG")
		 || strstr(filename, ".jpeg") || strstr(filename, ".JPEG"))
	{
	    pImageBuffer = GfTexReadImageFromJPEG(filename, screen_gamma, pWidth, pHeight,
											  pPow2Width, pPow2Height);
	}
	else
	{
		GfError("Could not read image from %s : only JPEG / PNG are supported\n", filename);
	}

	return pImageBuffer;
}

/** Write a buffer to a png image on disk.
    @ingroup	img
    @param	img		image data (RGB)
    @param	filename	filename of the png file
    @param	width		width of the image
    @param	height		height of the image
    @return	0 Ok
		<br>-1 Error
 */
int
GfTexWriteImageToPNG(unsigned char *img, const char *filename, int width, int height)
{
	FILE *fp;
	png_structp	png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	png_uint_32 rowbytes;
	int i;
	unsigned char *cur_ptr;
	float		screen_gamma;
#define DEFGAMMA 1.0
#define ReadGammaFromSettingsFile 1

	if (!img) {
		GfError("GfTexWriteImageToPNG(%s) : Null image buffer pointer\n", filename);
		return -1;
	}

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		GfError("GfTexWriteImageToPNG(%s) : Can't open file for writing\n", filename);
		return -1;
	}
	
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, (png_error_ptr)NULL, (png_error_ptr)NULL);
	if (png_ptr == NULL) {
		return -1;
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return -1;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {    
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return -1;
	}
	
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
#if (ReadGammaFromSettingsFile)
	char pszConfFilename[256];
	snprintf(pszConfFilename, sizeof(pszConfFilename), "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
    void *handle = GfParmReadFile(pszConfFilename, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    screen_gamma = (float)GfParmGetNum(handle, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_GAMMA, (char*)NULL, DEFGAMMA);
    GfParmReleaseHandle(handle);
#else
	screen_gamma = DEFGAMMA;
#endif
	png_set_gAMA(png_ptr, info_ptr, screen_gamma);
	/* png_set_bgr(png_ptr);    TO INVERT THE COLORS !!!! */
	png_write_info(png_ptr, info_ptr);
	png_write_flush(png_ptr);
	
	rowbytes = width * 3;
	row_pointers = (png_bytep*)malloc(height * sizeof(png_bytep));
	
	if (row_pointers == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return -1;
	}
	
	for (i = 0, cur_ptr = img + (height - 1) * rowbytes ; i < height; i++, cur_ptr -= rowbytes) {
		row_pointers[i] = cur_ptr;
	}
	
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, (png_infop)NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	free(row_pointers);

	return 0;
}

/** Read a PNG RGBA 8888 / JPEG RGB 888 image into a RGBA 8888 OpenGL 2D texture.
    @ingroup	img
    @param	filename	file name of the image
    @param	pWidth	read image original width (pixels)
    @param	pHeight	read image original height (pixels)
    @param	pPow2Width	read texture total width (pixels, as the closest greater or equal power of 2 width value)
    @param	pPow2Height	read texture total height (pixels, as the closest greater or equal power of 2 height value)
    @return	None.
 */
unsigned
GfTexReadTexture(const char *filename, int* pWidth, int* pHeight,
				 int* pPow2Width, int* pPow2Height)
{
	// Get screen gamma value from graphics params.
	char buf[256];
	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	void *handle = GfParmReadFile(buf, GFPARM_RMODE_STD);
	const float screen_gamma =
		(float)GfParmGetNum(handle, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_GAMMA, (char*)NULL, 2.0);
	GfParmReleaseHandle(handle);

	// Load the image buffer from the file (JPEG 888 / PNG 8888)
	int nWidth, nHeight;
	GLbyte *pImageBuffer =
		(GLbyte*)GfTexReadImageFromFile(filename, screen_gamma, &nWidth, &nHeight,
										pPow2Width, pPow2Height);
	if (!pImageBuffer)
		return 0;

	// Generate the OpenGL texture from the image buffer.
	GLuint glTexId;
	glGenTextures(1, &glTexId);
	glBindTexture(GL_TEXTURE_2D, glTexId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 pPow2Width ? *pPow2Width : nWidth, pPow2Height ? *pPow2Height : nHeight,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, pImageBuffer);

	// Free the image buffer.
	free(pImageBuffer);

// 	GfLogDebug("GfTexReadTexture(%s) : id=%u (%dx%d =>%dx%d)\n",
// 			   filename, glTexId, nWidth, nHeight,
// 			   pPow2Width ? *pPow2Width : nWidth, pPow2Height ? *pPow2Height : nHeight);

	// Set the output dimensions if requested.
	if (pWidth)
		*pWidth = nWidth;
	if (pHeight)
		*pHeight = nHeight;

	return (unsigned)glTexId;
}

/** Free the texture
    @ingroup	img
    @param	tex	texture to free
    @return	none
*/
void
GfTexFreeTexture(unsigned glTexId)
{
	if (glTexId)
		glDeleteTextures(1, &glTexId);
}
