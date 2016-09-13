/***************************************************************************
                         guifont.cpp -- GLTT fonts management
                             -------------------
    created              : Fri Aug 13 22:19:09 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: guifont.cpp 5829 2014-11-12 22:38:12Z wdbee $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* This font manipulation is based on Brad Fish's glFont format and code.  */
/* http://www.netxs.net/bfish/news.html                                    */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <machine/endian.h>
#endif

#ifdef LINUX
#include <endian.h>
#endif

#include <portability.h>

#include "tgfclient.h"
#include "glfeatures.h"

#include "guifont.h"

				  
static char buf[1024];

GfuiFontClass *gfuiFont[GFUI_FONT_NB];
const char *keySize[4] = { "size big", "size large", "size medium", "size small" };


#ifndef WIN32
#if BYTE_ORDER == BIG_ENDIAN
void swap32(unsigned int *p, unsigned int size)
{
	unsigned int i, t;
	for (i = 0; i < size; i += 4) {
		t = (unsigned int) *p;
		*p = (t & 0xff000000U) >> 24;
		*p |= (t & 0x00ff0000U) >> 8;
		*p |= (t & 0x0000ff00U) << 8;
		*p |= (t & 0x000000ffU) << 24;
		p++;
	}
}
#endif
#endif

void gfuiFreeFonts(void)
{
	GfuiFontClass* font;
	for ( int I = 0; I < GFUI_FONT_NB; I++)
	{
		font = gfuiFont[I];
		delete font;
	}
}

void gfuiLoadFonts(void)
{
	void *param;
	int	size;
	int	i;
	int nFontId;

	snprintf(buf, sizeof(buf), "%s%s", GfLocalDir(), GFSCR_CONF_FILE);
	param = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Menu Font", "name", "b5.glf"));
	GfLogTrace("Loading font 'Menu Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_BIG;
	for(i = 0; i < 4; i++, nFontId++) {
		size = (int)GfParmGetNum(param, "Menu Font", keySize[i], (char*)NULL, 10.0);
		GfLogTrace(" %d,", size);
		gfuiFont[nFontId] = new GfuiFontClass(buf);
		gfuiFont[nFontId]->create(size);
	}
	GfLogTrace("\n");

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Console Font", "name", "b7.glf"));
	GfLogTrace("Loading font 'Console Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_BIG_C;
	for(i = 0; i < 4; i++, nFontId++) {
		size = (int)GfParmGetNum(param, "Console Font", keySize[i], (char*)NULL, 10.0);
		GfLogTrace(" %d,", size);
		gfuiFont[nFontId] = new GfuiFontClass(buf);
		gfuiFont[nFontId]->create(size);
	}
	GfLogTrace("\n");

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Text Font", "name", "b6.glf"));
	GfLogTrace("Loading font 'Text Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_BIG_T;
	for(i = 0; i < 4; i++, nFontId++) {
		size = (int)GfParmGetNum(param, "Text Font", keySize[i], (char*)NULL, 10.0);
		GfLogTrace(" %d,", size);
		gfuiFont[nFontId] = new GfuiFontClass(buf);
		gfuiFont[nFontId]->create(size);
	}
	GfLogTrace("\n");

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Digital Font", "name", "digital.glf"));
	GfLogTrace("Loading font 'Digital Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_DIGIT;
	size = (int)GfParmGetNum(param, "Digital Font", keySize[0], (char*)NULL, 8.0);
	GfLogTrace(" %d\n", size);
	gfuiFont[nFontId] = new GfuiFontClass(buf);
	gfuiFont[nFontId]->create(size);

	GfParmReleaseHandle(param);
}

GfuiFontClass::GfuiFontClass(char *FileName)
{
	FILE *Input;
	char *TexBytes;
	int unsigned	Num;
	unsigned Tex;
	size_t readSize;

	font = NULL;
	size = 8.0;

	//Open font file
	if (!(Input = fopen(FileName, "rb"))) {
		perror(FileName);
		return;
	}

	if (!(font = (GLFONT *)malloc(sizeof(GLFONT))))
		return;

	//Read glFont structure
	//fread(font, sizeof(GLFONT), 1, Input);
	readSize = fread(font, 24, 1, Input); // for IA64...
	if( readSize <= 0 )
		GfLogWarning( "Not all bytes are successfully read" );
	//GfLogDebug("Font(%s) : texW=%d, texH=%d\n", FileName, font->TexWidth, font->TexHeight);

#ifndef WIN32
#if BYTE_ORDER == BIG_ENDIAN
	swap32((unsigned int *) font, 24);
#endif
#endif

	//Get number of characters
	Num = font->IntEnd - font->IntStart + 1;

	//Allocate memory for characters
	if ((font->Char = (GLFONTCHAR *)malloc(sizeof(GLFONTCHAR) * Num)) == NULL) {
		free(font);
		font = NULL;
		fclose(Input);
		return;
	}

    //Read glFont characters
	readSize = fread(font->Char, sizeof(GLFONTCHAR), Num, Input);
	if( readSize <= 0 )
		GfLogWarning( "Not all bytes are successfully read" );

#ifndef WIN32
#if BYTE_ORDER == BIG_ENDIAN
	swap32((unsigned int *)font->Char, sizeof(GLFONTCHAR) * Num);
#endif
#endif

	// Trace font info.
	// GfLogDebug("  %d chars\n", Num);
	// GfLogDebug("    %d : dx=%f, dy=%f\n", font->IntStart, font->Char[0].dx, font->Char[0].dy);
	// GfLogDebug("    %d : dx=%f, dy=%f\n", font->IntStart+2, font->Char[2].dx, font->Char[2].dy);
	// GfLogDebug("    %d : dx=%f, dy=%f\n", font->IntStart+Num-1, font->Char[Num-1].dx, font->Char[Num-1].dy);

	//Get texture size
	Num = font->TexWidth * font->TexHeight * 2;

	//Allocate memory for texture data
	if ((TexBytes = (char *)malloc(Num)) == NULL) {
		fclose(Input);
		return;
	}

	//Read texture data
	readSize = fread(TexBytes, sizeof(char), Num, Input);
	if( readSize < Num )
		GfLogWarning( "Not all bytes are successfully read" );

	fclose(Input);

	//Save texture number
	glGenTextures(1, &Tex);
	font->Tex = Tex;
	
	//Set texture attributes
	glBindTexture(GL_TEXTURE_2D, Tex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//Create texture
	glTexImage2D(GL_TEXTURE_2D, 0, 2, font->TexWidth,
		 font->TexHeight, 0, GL_LUMINANCE_ALPHA,
		 GL_UNSIGNED_BYTE, (void *)TexBytes);

	//Clean up
	free(TexBytes);

	//Return pointer to new font
	return;
}

GfuiFontClass::~GfuiFontClass()
{
	if (font) {
		glDeleteTextures(1, &font->Tex);
		free(font->Char);
		free(font);
	}
}

void GfuiFontClass::create(int point_size)
{
	//GfLogDebug("size = %d pts, %d pixels\n", point_size, (int)(font->Char[2].dy * point_size));
	size = point_size;
}

int GfuiFontClass::getWidth(const char* text)
{
	int Length, i;
	GLFONTCHAR *Char;
	float width = 0;

	if (!font)
		return 0;

	//Get length of string
	Length = strlen(text);

	//Loop through characters
	for (i = 0; i < Length; i++) {
		//Get pointer to glFont character
		Char = &font->Char[(int)((unsigned char)text[i]) - font->IntStart];
		width += Char->dx * size;
	}

	return (int)width;
}

// Get total height (descender included).
int GfuiFontClass::getHeight() const
{
	if (!font)
		return 0;
	
	// All chars of the font have the same dy (except 1st and last in liberation-sans-bold ?).
	return (int)(font->Char[2].dy * size);
}

int GfuiFontClass::getDescender() const
{
	if (!font)
		return 0;
	
	// All chars of the font have the same dy (except 1st and last in liberation-sans-bold ?).
	return (int)(font->Char[2].dy * size / 3.0);
}

void GfuiFontClass::drawString(int X, int Y, const char* text)
{
	int		Length, i;
	GLFONTCHAR	*Char;
	float	x = (float)X;
	float	y = (float)Y;

	//Return if we don't have a valid glFont
	if (!font)
		return;

	//Get length of string
	Length = strlen(text);

	//Begin rendering quads
	glBindTexture(GL_TEXTURE_2D, font->Tex);
	glBegin(GL_QUADS);

	//Loop through characters
	for (i = 0; i < Length; i++)
	{
		//Get pointer to glFont character
		Char = &font->Char[(int)((unsigned char)text[i]) - font->IntStart];

		//Specify vertices and texture coordinates
		glTexCoord2f(Char->tx1, Char->ty1);
		glVertex2f(x, y + Char->dy * size);
		glTexCoord2f(Char->tx1, Char->ty2);
		glVertex2f(x, y);
		glTexCoord2f(Char->tx2, Char->ty2);
		glVertex2f(x + Char->dx * size, y);
		glTexCoord2f(Char->tx2, Char->ty1);
		glVertex2f(x + Char->dx * size, y + Char->dy * size);

		//Move to next character
		x += Char->dx*size;
	}

	//Stop rendering quads
	glEnd();
}
