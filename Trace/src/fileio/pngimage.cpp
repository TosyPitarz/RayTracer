//#include "pngimage.h"
//
///* current versions of libpng should provide this macro: */
//#ifndef png_jmpbuf
//#  define png_jmpbuf(png_ptr)   ((png_ptr)->jmpbuf)
//#endif
//
//static png_structp png_ptr = null;
//static png_infop info_ptr = null;
//
//png_uint_32  width, height;
//int  bit_depth, color_type;
//uch  *image_data = null;
//
//void png_version_info(void) {
//
//	fprintf(stderr, "   compiled with libpng %s; using libpng %s.\n",
//		png_libpng_ver_string, png_libpng_ver);
//	fprintf(stderr, "   compiled with zlib %s; using zlib %s.\n",
//		zlib_version, zlib_version);
//}
//
///* return value = 0 for success, 1 for bad sig, 2 for bad ihdr, 4 for no mem, 8 for file open failure */
//
//int png_init(const char* filename, int &pwidth, int &pheight) {
//	
//	uch sig[8];
//	file *infile;
//
//	if ((infile = fopen(filename, "rb")) == null) return (8);
//
//	/* check that the file really is a png image; could
//	* have used slightly more general png_sig_cmp() function instead */
//
//	fread(sig, 1, 8, infile);
//	if (png_sig_cmp(sig, 0, 8) != 0) return 1;   /* bad signature */
//
//	/* could pass pointers to user-defined error handlers instead of nulls: */
//
//	png_ptr = png_create_read_struct(png_libpng_ver_string, null, null, null);
//	if (!png_ptr) return 4;   /* out of memory */
//
//	info_ptr = png_create_info_struct(png_ptr);
//	if (!info_ptr) {
//		png_destroy_read_struct(&png_ptr, null, null);
//		return 4;   /* out of memory */
//	}
//
//	/* we could create a second info struct here (end_info), but it's only
//	* useful if we want to keep pre- and post-idat chunk info separated
//	* (mainly for png-aware image editors and converters) */
//
//	/* setjmp() must be called in every function that calls a png-reading
//	* libpng function */
//
//	if (setjmp(png_jmpbuf(png_ptr))) {
//		png_destroy_read_struct(&png_ptr, &info_ptr, null);
//		return 2;
//	}
//
//	png_init_io(png_ptr, infile);
//	png_set_sig_bytes(png_ptr, 8);  /* we already read the 8 signature bytes */
//
//	png_read_info(png_ptr, info_ptr);  /* read all png info up to image data */
//
//	/* alternatively, could make separate calls to png_get_image_width(),
//	* etc., but want bit_depth and color_type for later [don't care about
//	* compression_type and filter_type => nulls] */
//
//	png_get_ihdr(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, null, null, null);
//	pwidth = (int)width;
//	pheight = (int)height;
//
//	/* ok, that's all we need for now; return happy */
//
//	return 0;
//}
//
///* returns 0 if succeeds, 1 if fails due to no bkgd chunk, 2 if libpng error;
//* scales values to 8-bit if necessary */
//
//int png_get_bgcolor(uch *red, uch *green, uch *blue) {
//
//	png_color_16p pbackground;
//
//	/* setjmp() must be called in every function that calls a png-reading
//	* libpng function */
//
//	if (setjmp(png_jmpbuf(png_ptr))) {
//		png_destroy_read_struct(&png_ptr, &info_ptr, null);
//		return 2;
//	}
//
//
//	if (!png_get_valid(png_ptr, info_ptr, png_info_bkgd)) return 1;
//
//	/* it is not obvious from the libpng documentation, but this function
//	* takes a pointer to a pointer, and it always returns valid red, green
//	* and blue values, regardless of color_type: */
//
//	png_get_bkgd(png_ptr, info_ptr, &pbackground);
//
//	/* however, it always returns the raw bkgd data, regardless of any
//	* bit-depth transformations, so check depth and adjust if necessary */
//
//	if (bit_depth == 16) {
//		*red   = pbackground->red   >> 8;
//		*green = pbackground->green >> 8;
//		*blue  = pbackground->blue  >> 8;
//	} else if (color_type == png_color_type_gray && bit_depth < 8) {
//		if (bit_depth == 1)
//			*red = *green = *blue = pbackground->gray? 255 : 0;
//		else if (bit_depth == 2)
//			*red = *green = *blue = (255/3) * pbackground->gray;
//		else /* bit_depth == 4 */
//			*red = *green = *blue = (255/15) * pbackground->gray;
//	} else {
//		*red   = (uch)pbackground->red;
//		*green = (uch)pbackground->green;
//		*blue  = (uch)pbackground->blue;
//	}
//
//	return 0;
//}
//
///* display_exponent == lut_exponent * crt_exponent */
//
//uch *png_get_image(double display_exponent, int &pchannels, int &prowbytes) {
//
//	double  gamma;
//	png_uint_32  i, rowbytes;
//	png_bytepp  row_pointers = null;
//
//	/* setjmp() must be called in every function that calls a png-reading
//	* libpng function */
//
//	if (setjmp(png_jmpbuf(png_ptr))) {
//		png_destroy_read_struct(&png_ptr, &info_ptr, null);
//		return null;
//	}
//
//	/* expand palette images to rgb, low-bit-depth grayscale images to 8 bits,
//	* transparency chunks to full alpha channel; strip 16-bit-per-sample
//	* images to 8 bits per sample; and convert grayscale to rgb[a] */
//
//	if (color_type == png_color_type_palette)
//		png_set_expand(png_ptr);
//	if (color_type == png_color_type_gray && bit_depth < 8)
//		png_set_expand(png_ptr);
//	if (png_get_valid(png_ptr, info_ptr, png_info_trns))
//		png_set_expand(png_ptr);
//	if (bit_depth == 16)
//		png_set_strip_16(png_ptr);
//	if (color_type == png_color_type_gray ||
//		color_type == png_color_type_gray_alpha)
//		png_set_gray_to_rgb(png_ptr);
//
//	/* unlike the example in the libpng documentation, we have *no* idea where
//	* this file may have come from--so if it doesn't have a file gamma, don't
//	* do any correction ("do no harm") */
//
//	if (png_get_gama(png_ptr, info_ptr, &gamma))
//		png_set_gamma(png_ptr, display_exponent, gamma);
//
//	/* all transformations have been registered; now update info_ptr data,
//	* get rowbytes and channels, and allocate image memory */
//
//	png_read_update_info(png_ptr, info_ptr);
//
//	prowbytes = rowbytes = png_get_rowbytes(png_ptr, info_ptr);
//	pchannels = (int)png_get_channels(png_ptr, info_ptr);
//
//	if ((image_data = (uch *)malloc(rowbytes*height)) == null) {
//		png_destroy_read_struct(&png_ptr, &info_ptr, null);
//		return null;
//	}
//	if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == null) {
//		png_destroy_read_struct(&png_ptr, &info_ptr, null);
//		free(image_data);
//		image_data = null;
//		return null;
//	}
//
//	trace((stderr, "readpng_get_image:  channels = %d, rowbytes = %ld, height = %ld\n", *pchannels, rowbytes, height));
//
//	/* set the individual row_pointers to point at the correct offsets */
//
//	for (i = 0;  i < height;  ++i) row_pointers[i] = image_data + i*rowbytes;
//
//	/* now we can go ahead and just read the whole image */
//
//	png_read_image(png_ptr, row_pointers);
//
//	/* and we're done!  (png_read_end() can be omitted if no processing of
//	* post-idat text/time/etc. is desired) */
//
//	free(row_pointers);
//	row_pointers = null;
//
//	png_read_end(png_ptr, null);
//
//	return image_data;
//}
//
//void png_cleanup(int free_image_data) {
//
//	if (free_image_data && image_data) {
//		free(image_data);
//		image_data = null;
//	}
//
//	if (png_ptr && info_ptr) {
//		png_destroy_read_struct(&png_ptr, &info_ptr, null);
//		png_ptr = null;
//		info_ptr = null;
//	}
//}
