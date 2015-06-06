/***************************************************************************

    file                 : grtexture.cpp
    created              : Wed Jun 1 14:56:31 CET 2005
    copyright            : (C) 2005 by Bernhard Wymann
    version              : $Id: grtexture.cpp 5353 2013-03-24 10:26:22Z pouillot $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
	This classes/methods are used to handle texture compression and
	textures which are shared among multiple objects. In the long term
	they should obsolete parts of grutil.cpp.
*/

#include <glfeatures.h> // GfglFeatures

#include "grtexture.h"

#include "grutil.h"


int doMipMap(const char *tfname, int mipmap)
{
	char *buf = strdup(tfname);

	// find the filename extension.
	char *s = strrchr(buf, '.');
	if (s) {
		*s = 0;
	}

	// search for the texture parameters.
	s = strrchr(buf, '_');

	// 1) no mipmap for "*_n".
	if (s && s[1] == 'n') {
		mipmap = FALSE;
	}

	// 1) no mipmap for "*shadow*".
	if (mipmap) {
		// Check the shadow.
		s = strrchr((char *)tfname, '/');
		if (!s) {
			s = (char*)tfname;
		} else {
			s++;
		}
		if (strstr(s, "shadow")) {
			mipmap = FALSE;
		}
	}
	free(buf);
	return mipmap;
}

// cgrStateFactory class ========================================================
// TODO: really manage shared textures (see obsolete grutil.cpp parts).

cgrStateFactory* grStateFactory = new cgrStateFactory;

cgrSimpleState* cgrStateFactory::getSimpleState()
{
	return new cgrSimpleState();
}

cgrMultiTexState* cgrStateFactory::getMultiTexState(cgrMultiTexState::tfnTexScheme fnTexScheme)
{
	return new cgrMultiTexState(fnTexScheme);
}

// SGI loader==================================================================
/*
	The latter parts are derived from plib (plib.sf.net) and have this license:

	PLIB - A Suite of Portable Game Libraries
	Copyright (C) 1998,2002  Steve Baker

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.

	For further information visit http://plib.sourceforge.net
*/

/*
	Modifications:
		Copyright (c) 2005 Bernhard Wymann
*/




// SGI texture loading function.
bool grLoadSGI(const char *fname, ssgTextureInfo* info)
{
	cgrSGIHeader *sgihdr = new cgrSGIHeader(fname, info);
	bool returnval = sgihdr->loadSGI_bool;
	delete sgihdr;
	return returnval;
}

// Register customized loader in plib.
void grRegisterCustomSGILoader(void)
{
	ssgAddTextureFormat(".rgb", grLoadSGI);
	ssgAddTextureFormat(".rgba", grLoadSGI);
	ssgAddTextureFormat(".int", grLoadSGI);
	ssgAddTextureFormat(".inta", grLoadSGI);
	ssgAddTextureFormat(".bw", grLoadSGI);
	ssgAddTextureFormat(".png", grLoadPngTexture);
	ssgAddTextureFormat(".jpg", grLoadJpegTexture);
}

cgrSGIHeader::cgrSGIHeader(const char *fname, ssgTextureInfo* info)
{
	cgrSGIHeader *sgihdr = this;

	start = NULL;
	leng = NULL;

	bool success = openFile(fname);

	int mipmap = doMipMap(fname, TRUE);

	if (!success) {
		loadSGI_bool = false;
		return;
	}

	GLubyte *image = (GLubyte*)malloc( sizeof(GLubyte) * sgihdr->xsize*sgihdr->ysize*sgihdr->zsize );
	GLubyte *ptr = image;

	unsigned char *rbuf = new unsigned char[sgihdr->xsize];
	unsigned char *gbuf = (sgihdr->zsize>1) ? new unsigned char[sgihdr->xsize] : 0 ;
	unsigned char *bbuf = (sgihdr->zsize>2) ? new unsigned char[sgihdr->xsize] : 0 ;
	unsigned char *abuf = (sgihdr->zsize>3) ? new unsigned char[sgihdr->xsize] : 0 ;

	for (int y = 0 ; y < sgihdr->ysize ; y++) {
		int x ;

		switch (sgihdr->zsize) {
			case 1:
				sgihdr->getRow(rbuf, y, 0);
				for (x = 0; x < sgihdr->xsize; x++) {
					*ptr++ = rbuf[x];
				}
				break;

			case 2:
				sgihdr->getRow (rbuf, y, 0);
				sgihdr->getRow (gbuf, y, 1);

				for (x = 0; x < sgihdr->xsize; x++) {
					*ptr++ = rbuf[x];
					*ptr++ = gbuf[x];
				}
				break;

			case 3:
				sgihdr->getRow(rbuf, y, 0);
				sgihdr->getRow(gbuf, y, 1);
				sgihdr->getRow(bbuf, y, 2);

				for (x = 0; x < sgihdr->xsize; x++) {
					*ptr++ = rbuf[x];
					*ptr++ = gbuf[x];
					*ptr++ = bbuf[x];
				}
				break;

			case 4:
				sgihdr->getRow(rbuf, y, 0);
				sgihdr->getRow(gbuf, y, 1);
				sgihdr->getRow(bbuf, y, 2);
				sgihdr->getRow(abuf, y, 3);

				for (x = 0; x < sgihdr->xsize; x++ ) {
					*ptr++ = rbuf[x];
					*ptr++ = gbuf[x];
					*ptr++ = bbuf[x];
					*ptr++ = abuf[x];
				}
				break;
		}
	}

	fclose(image_fd);
	image_fd = NULL ;
	delete [] rbuf;
	delete [] gbuf;
	delete [] bbuf;
	delete [] abuf;

	if (info) {
		info->width = sgihdr->xsize;
		info->height = sgihdr->ysize;
		info->depth = sgihdr->zsize;
		info->alpha = (sgihdr->zsize == 2 || sgihdr->zsize == 4);
	}

	loadSGI_bool = grMakeMipMaps(image, sgihdr->xsize, sgihdr->ysize, sgihdr->zsize, mipmap);
}

bool grMakeMipMaps (GLubyte *image, int xsize, int ysize, int zsize, int mipmap)
{


	if (!((xsize & (xsize-1))==0) || !((ysize & (ysize-1))==0)) {
		ulSetError ( UL_WARNING, "Map is not a power-of-two in size!" ) ;
    	return false ;
	}

	GLubyte *texels[20];   // One element per level of MIPmap.

	for (int l = 0; l < 20; l++) {
    	texels [l] = NULL;
	}

	texels[0] = image;

	int lev;

	for (lev = 0 ;((xsize >> (lev+1)) != 0 || (ysize >> (lev+1)) != 0); lev++) {
		// Suffix '1' is the higher level map, suffix '2' is the lower level.
		int l1 = lev;
		int l2 = lev+1;
		int w1 = xsize >> l1;
		int h1 = ysize >> l1;
		int w2 = xsize >> l2;
		int h2 = ysize >> l2;

		if (w1 <= 0) {
			w1 = 1;
		}
		if (h1 <= 0) {
			h1 = 1;
		}
		if (w2 <= 0) {
			w2 = 1;
		}
		if (h2 <= 0) {
			h2 = 1;
		}

		texels[l2] = (GLubyte*)malloc( sizeof(GLubyte) * w2*h2*zsize );

		for (int x2 = 0; x2 < w2; x2++) {
			for (int y2 = 0; y2 < h2; y2++) {
				for (int c = 0; c < zsize; c++) {
					int x1 = x2 + x2;
					int x1_1 = (x1 + 1) % w1;
					int y1 = y2 + y2;
					int y1_1 = (y1 + 1) % h1;
					int t1 = texels[l1][(y1*w1 + x1)*zsize + c];
					int t2 = texels[l1][(y1_1*w1 + x1)*zsize + c];
					int t3 = texels[l1][(y1*w1 + x1_1)*zsize + c];
					int t4 = texels[l1][(y1_1*w1 + x1_1)*zsize + c];

					if (c == 3) { // Alpha.
						int a = t1;
						if (t2 > a) {
							a = t2;
						}
						if (t3 > a) {
							a = t3;
						}
						if (t4 > a) {
							a = t4;
						}
						texels[l2][(y2*w2 + x2)*zsize + c] = a;
					} else {
						texels[l2][(y2*w2 + x2)*zsize + c] = ( t1 + t2 + t3 + t4 )/4;
					}
				}
			}
		}
	}

	texels[lev + 1] = NULL;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
	int map_level = 0;

	GLint ww;

	GLint textureTargetFormat;
	if (GfglFeatures::self().isSelected(GfglFeatures::TextureCompression)) {
		//GfTrace("COMPRESSOR: ");

		switch (zsize) {
			case 1:
				textureTargetFormat = GL_COMPRESSED_LUMINANCE_ARB;
				//GfTrace("GL_COMPRESSED_LUMINANCE_ARB\n");
				break;
			case 2:
				textureTargetFormat = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
				//GfTrace("GL_COMPRESSED_LUMINANCE_ALPHA_ARB\n");
				break;
			case 3:
				textureTargetFormat = GL_COMPRESSED_RGB_ARB;
				//GfTrace("GL_COMPRESSED_RGB_ARB\n");
				break;
			default:
				textureTargetFormat = GL_COMPRESSED_RGBA_ARB;
				//GfTrace("GL_COMPRESSED_RGBA_ARB\n");
				break;
		}
	} else {
		textureTargetFormat = zsize;
		//GfTrace("NON COMPRESSOR\n");
	}

	const int tlimit = GfglFeatures::self().getSelected(GfglFeatures::TextureMaxSize);

	do {
		if (xsize > tlimit || ysize > tlimit) {
			ww = 0;
		} else {

			glTexImage2D(GL_PROXY_TEXTURE_2D, map_level, textureTargetFormat, xsize, ysize, FALSE /* Border */,
									(zsize==1)?GL_LUMINANCE:
									(zsize==2)?GL_LUMINANCE_ALPHA:
									(zsize==3)?GL_RGB:
											GL_RGBA,
									GL_UNSIGNED_BYTE, NULL);

			glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &ww);
		}

		if (ww == 0) {
			free( texels[0] );
			xsize >>= 1;
			ysize >>= 1;
			for (int l = 0; texels [l] != NULL; l++) {
				texels [l] = texels[l+1];
			}

			if (xsize < 8 && ysize < 8) {
				//ulSetError (UL_FATAL, "SSG: OpenGL will not accept a downsized version ?!?");
			}
		}
	} while (ww == 0);


	for (int i = 0; texels[i] != NULL; i++) {
		int w = xsize >> i;
		int h = ysize >> i;

		if (w <= 0) {
			w = 1;
		}
		if (h <= 0) {
			h = 1;
		}

		if (mipmap == TRUE || i == 0) {
			glTexImage2D  ( GL_TEXTURE_2D,
						map_level, textureTargetFormat, w, h, FALSE /* Border */,
								(zsize==1)?GL_LUMINANCE:
								(zsize==2)?GL_LUMINANCE_ALPHA:
								(zsize==3)?GL_RGB:
										GL_RGBA,
								GL_UNSIGNED_BYTE, (GLvoid *) texels[i] ) ;


			/*int compressed;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, map_level, GL_TEXTURE_COMPRESSED_ARB, &compressed);
			if (compressed == GL_TRUE) {
				int csize;
				glGetTexLevelParameteriv(GL_TEXTURE_2D, map_level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &csize);
				GfTrace("compression ratio: %d to %d\n", csize, w*h*zsize);
			} else {
				GfTrace("not compressed\n");
			}*/
		}

		map_level++ ;
		free( texels[i] );
	}

	return true;
}

void doAnisotropicFiltering(){
    int aniS;
    float aniD;

    if(GfglFeatures::self().getSupported(GfglFeatures::AnisotropicFiltering)!=GfglFeatures::InvalidInt)
    {
        aniS = GfglFeatures::self().getSelected(GfglFeatures::AnisotropicFiltering);

        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);

        switch(aniS)
		{
			case 1:
				aniD = fLargest/2;
				break;
			case 2:
				aniD = fLargest;
				break;
			default:
				aniD = 0;
        }

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniD);/**/
    }
}

bool grLoadPngTexture (const char *fname, ssgTextureInfo* info)
{
	GLubyte *tex;
	int w, h;
	int mipmap = 1;

	TRACE_GL("Load: loadPngTexture start");

	tex = (GLubyte*)GfTexReadImageFromPNG(fname, 2.0, &w, &h, 0, 0);
	if (!tex) {
		return false;
	}

	if (info) {
		info -> width  = w;
		info -> height = h;
		info -> depth  = 4;
		info -> alpha  = true;
	}

	TRACE_GL("Load: loadPngTexture stop");

	// TODO: Check if tex is freed => yes, it is in grMakeMipMaps through "free(texel[i])".
	//       Check/fix potential problems related to malloc/delete mixture
	//       (instead of malloc/free or new/delete). => done below

	mipmap = doMipMap(fname, mipmap);

// Don't know why this code, but it allocates with new something that is later freed
// with free in grMakeMipMaps ... so I comment it out.
// #ifdef WIN32
// 	GLubyte* tex2 = new GLubyte[w*h*4];
// 	memcpy(tex2, tex, w*h*4);
// 	free(tex);
// 	tex = tex2;
// #endif // WIN32

    bool res = grMakeMipMaps(tex, w, h, 4, mipmap) == TRUE ? true : false;

   doAnisotropicFiltering();

    return res;
}

bool grLoadJpegTexture (const char *fname, ssgTextureInfo* info)
{
	GLubyte *tex;
	int w, h;
	int mipmap = 1;

	TRACE_GL("Load: loadJpegTexture start");

	tex = (GLubyte*)GfTexReadImageFromJPEG(fname, 2.0, &w, &h, 0, 0);
	if (!tex) {
		return false;
	}

	if (info) {
		info -> width  = w;
		info -> height = h;
		info -> depth  = 4;
		info -> alpha  = true;
	}

	TRACE_GL("Load: loadPngTexture stop");

	mipmap = doMipMap(fname, mipmap);


    bool res = grMakeMipMaps(tex, w, h, 4, mipmap) == TRUE ? true : false;

    doAnisotropicFiltering();

    return res;
}
