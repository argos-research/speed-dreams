/***************************************************************************

    file                 : grloadac.h
    copyright            : (C) 2000 by Eric Espie
    web                  : www.speed-dreams.org
    version              : $Id: grloadac.h 4902 2012-08-27 10:04:20Z kmetykog $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRLOADAC_H_
#define _GRLOADAC_H_

#include <map>
#include <string>

#include <plib/ssg.h>


class grssgLoaderOptions : public ssgLoaderOptions
{
 public:

	grssgLoaderOptions(bool bTextureMipMap = false);
	
	virtual void makeModelPath(char* path, const char *fname) const;
	virtual void makeTexturePath(char* path, const char *fname) const;

	//! Specialized in order to do special things from the texture name (like mip map) before creation if specified (see constructor).
	ssgTexture* createTexture(char* tfname, int wrapu = TRUE, int wrapv = TRUE, int mipmap = TRUE);

	//! Add a texture mapping.
	void addTextureMapping(const char* pszSrcFileName, const char* pszTgtFileName);

	//! Answer if there's any available texture mapping or not (avoid calling mapTexture if not needed, for optimization purpose).
	bool textureMapping() const;
	
	//! Return the texture file name after applying the first available mapping on the source name.
	const char* mapTexture(const char* pszSrcFileName) const;
	
 protected:

	//! Flag for texture preprocessing before creation.
	bool _bTextureMipMap;
	
	//! Texture substitution map, from the source file name (read in the AC file) to the target file name (found in map).
	std::map<std::string, std::string> _mapTextures;

	//! Only for optimizations : tell if _mapTextures is empty or not (computed in addTextureMapping).
	bool _bTextureMapping;
} ;


//! Set current SSG loader options.
extern void grssgSetCurrentOptions(grssgLoaderOptions* options);

//! Load a car AC3D model from a file
extern ssgEntity *grssgCarLoadAC3D(const char *fname, const grssgLoaderOptions* options, int carIndex);

//! Load a car wheel AC3D model from a file
extern ssgEntity *grssgCarWheelLoadAC3D(const char *fname, const grssgLoaderOptions* options, int carIndex);

//! Load an AC3D model from a file
extern ssgEntity *grssgLoadAC3D(const char *fname, const grssgLoaderOptions* options);

//! ?
extern double carTrackRatioX;
extern double carTrackRatioY;


#endif /* _GRLOADAC_H_ */ 
