/* $Author: cvs $
 * $Name:  $
 * $Id: jpeg.h,v 1.1.1.1 2005/10/22 16:44:21 cvs Exp $
 * $Source: /usr/local/cvsroot/colibri-window/server/jpeg.h,v $
 * $Log: jpeg.h,v $
 * Revision 1.1.1.1  2005/10/22 16:44:21  cvs
 * initial creation
 *
 * Revision 1.1.1.1  2005/10/15 15:14:06  cvs
 * initial project comments
 *
 * Revision 1.2  2003/12/30 20:49:49  srik
 * Added ifdef flags for compatibility
 *
 * Revision 1.1.1.1  2003/12/30 20:39:19  srik
 * Helicopter deployment with firewire camera
 *
 * 18/01/2012 - Integration with CVGDroneProxy by Ignacio Mellado
 *
 */

#ifndef _JPEG_H_
#define _JPEG_H_

#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

int jpeg_compress(char *dst, char *src, int width, int height, int dstsize, int quality);
void jpeg_decompress(unsigned char *dst, unsigned char *src, int size, int *width, int *height);
void jpeg_decompress_from_file(unsigned char *dst, char *file, int size, int *width, int *height);

extern char jpeg_isError;

#endif
