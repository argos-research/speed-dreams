/***************************************************************************

    file                 : grutil.cpp
    created              : Wed Nov  1 21:33:22 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grutil.cpp 4488 2012-02-08 23:59:13Z kmetykog $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <plib/ssg.h>

#include <tgfclient.h>
#include <algorithm>

#include "grutil.h"
#include "grscene.h"
#include "grtexture.h"

float grGammaValue = 1.8;
int grMipMap = 0;

char *grFilePath;			// Multiple path (';' separated) used to search for files.


int grGetFilename(const char *filename, const char *filepath, char *buf)
{
	const char *c1, *c2;
	int found = 0;
	int lg;

	if (filepath) {
		c1 = filepath;
		c2 = c1;
		while (!found && c2) {
			c2 = strchr(c1, ';');
			if (c2 == NULL) {
				sprintf(buf, "%s/%s", c1, filename);
			} else {
				lg = c2 - c1;
				strncpy(buf, c1, lg);
				buf[lg] = '/';
				strcpy(buf + lg + 1, filename);
			}
			if (ulFileExists(buf)) {
				found = 1;
			}
			c1 = c2 + 1;
		}
	} else {
		strcpy(buf, filename);
		if (ulFileExists(buf)) {
			found = 1;
		}
	}

	return found;
}


typedef struct stlist
{
    struct stlist* next;
    cgrSimpleState* state;
    char* name;
} stlist;


static stlist* stateList = NULL;


static cgrSimpleState* grGetState(const char *img)
{
    stlist	*curr = stateList;
    while (curr) {
		if (strcmp(curr->name, img) == 0) {
			return curr->state;
		}
		curr = curr->next;
    }
    return 0;
}


void grShutdownState(void)
{
	stlist *next;

	stlist *curr = stateList;
	while (curr) {
		next = curr->next;
		//GfLogTrace("Still in list : %s\n", curr->name);
		free(curr->name);
		//ssgDeRefDelete(curr->state); // Not needed, as in scene graph (?).
		free(curr);
		curr = next;
	}
	stateList = 0;
}


static void grSetupState(cgrSimpleState *st, char *buf)
{
	st->ref();			// cannot be removed
	st->enable(GL_LIGHTING);
	st->enable(GL_TEXTURE_2D);
	st->enable(GL_BLEND);
	st->setColourMaterial(GL_AMBIENT_AND_DIFFUSE);	

	stlist *curr = (stlist*)calloc(sizeof(stlist), 1);
	curr->next = stateList;
	stateList = curr;
	curr->state = st;
	curr->name = strdup(buf);

	GfLogTrace("Loading texture %s\n", buf);
}


ssgState* grSsgLoadTexState(const char *img, int errIfNotFound)
{
	char buf[256];

	// remove the directory
	const char* s = strrchr(img, '/');
	if (!s) {
		s = img;
	} else {
		s++;
	}

	if (!grGetFilename(s, grFilePath, buf)) {
		if (errIfNotFound)
			GfLogError("Texture file %s not found in %s\n", s, grFilePath);
		return NULL;
	}

	cgrSimpleState* st = grGetState(buf);
	if (st) {
		return (ssgState*)st;
	}

	st = grStateFactory->getSimpleState();
	grSetupState(st, buf);
	st->setTexture(buf);
	
	return (ssgState*)st;
}

ssgState* grSsgLoadTexStateEx(const char *img, const char *filepath,
							  int wrap, int mipmap, int errIfNotFound)
{
	char buf[256];

	// remove the directory path
	const char* s = strrchr(img, '/');
	if (!s) {
		s = img;
	} else {
		s++;
	}

	if (!grGetFilename(s, filepath, buf)) {
		if (errIfNotFound)
			GfLogError("Texture file (ex) %s not found in %s\n", s, filepath);
		return NULL;
	}

	cgrSimpleState* st = grGetState(buf);
	if (st) {
		return (ssgState*)st;
	}

	st = grStateFactory->getSimpleState();
	grSetupState(st, buf);
	st->setTexture(buf, wrap, wrap, mipmap);

	return (ssgState*)st;
}

cgrMultiTexState* grSsgEnvTexState(const char *img, cgrMultiTexState::tfnTexScheme fnTexScheme,
								   int errIfNotFound)
{
	char buf[256];

	// remove the directory
	const char *s = strrchr(img, '/');
	if (!s)
		s = img;
	else
		s++;

	if (!grGetFilename(s, grFilePath, buf)) {
		if (errIfNotFound)
			GfLogError("Env. texture file %s not found in %s\n", s, grFilePath);
		return 0;
    }

	cgrMultiTexState* st = grStateFactory->getMultiTexState(fnTexScheme);
	grSetupState(st, buf);
	st->setTexture(buf);

	return st;
}

/*
 * 
 * name: grWriteTime
 * Formats and outputs the time as a right aligned string.
 * 
 * @param color: colour to use
 * @param font: font to use
 * @param x: X coord of the left side of the bounding box
 * @param y: Y coord to bottom side of the bounding box
 * @param width: width of the bounding box
 * @param sec: time to format, in seconds
 * @param sgn: whether use +/- signs or not
 * 
 * @return
 */
void grWriteTime(float *color, int font, int x, int y, int width, tdble sec, int sgn)
{
	char  buf[256];
	
	grWriteTimeBuf(buf, sec, sgn);
	GfuiDrawString(buf, color, font, x, y, width, GFUI_ALIGN_HR);
}


/*
 * 
 * name: grWriteTimeBuf
 * Formats and write the time into the supplied character buffer.
 * 
 * @param buf: character buffer. Caller is totally responsible for this.
 * @param sec: time to format, in seconds
 * @param sgn: whether use +/- signs or not
 * 
 * @return
 */
void grWriteTimeBuf(char *buf, tdble sec, int sgn)
{
	const char* sign;

	if (sec < 0.0) {
		sec = -sec;
		sign = "-";
    } else {
		if (sgn) {
			sign = "+";
		} else {
			sign = "  ";
		}
    }

    const int h = (int)(sec / 3600.0);
    sec -= 3600 * h;
    const int m = (int)(sec / 60.0);
    sec -= 60 * m;
    const int s = (int)(sec);
    sec -= s;
    const int ms = (int)floor(sec * 1000.0);
    if (h) {
		sprintf(buf, "%s%2.2d:%2.2d:%2.2d.%3.3d", sign,h,m,s,ms);
    } else if (m) {
		sprintf(buf, "   %s%2.2d:%2.2d.%3.3d", sign,m,s,ms);
    } else {
		sprintf(buf, "      %s%2.2d.%3.3d", sign,s,ms);
    }
}


static inline float getPolyHOT(ssgHit& h) {
  return h.plane[2] == 0.0 ? 0.0 : - h.plane[3] / h.plane[2];
  }

// Binary comparison of 2 polys to decide which one is
//  lower over terrain. Used for max_element().
static inline bool HOTless(ssgHit& g, ssgHit& h) {
  return getPolyHOT(g) < getPolyHOT(h);
}

// Get height over terrain => hence HOT
float grGetHOT(float x, float y) {
  float ret = 0.0f;
	sgMat4 invmat;
	sgMakeIdentMat4(invmat);

	invmat[3][0] = -x;
	invmat[3][1] = -y;
	invmat[3][2] =  0.0f;

	sgVec3 test_vec = { 0 , 0 , 100000.0f };

  ssgHit *results = NULL;
	int num_hits = ssgHOT (TheScene, test_vec, invmat, &results);
  if (num_hits > 0) {
	ssgHit *h = std::max_element(&results[0], &results[num_hits-1], HOTless);
	if (h != NULL)
	  ret = getPolyHOT(*h);
  } else {
	GfLogWarning("grGetHOT: ssgHOT yielded 0 hits!\n");
  }
  return ret;
} //  grGetHOT

/*
 * NB: this solution is not faster, only cleaner
 * than a pure C-way implementation:

  	ssgHit* first = &results[0];
	ssgHit* last = &results[num_hits-1];
	ssgHit* largest = first;
	if (first == last) return getPolyHOT(*last);
	while (++first != last) {
      if (getPolyHOT(*largest) < getPolyHOT(*first))
        largest = first;
	}
    return getPolyHOT(*largest);

 * The C++ STL docs tells about max_element:
 * Complexity
 * Linear: Performs as many comparisons as the number of elements
 * in [first,last), except for first.
 *
 * However if needed, it can be improved like this:
 *
 * http://www.physicsforums.com/showthread.php?t=512668
 * Finding the maximum value in an array is a classic reduction problem;
 * it's the same as finding the sum, average, etc., just using a
 * different "binary operator" (one which returns the maximum of the
 * two arguments). That being said, given a fast (or the fastest) way
 * to compute the sum of an array of arguments in CUDA, the same
 * algorithm should be a fast (or the fastest) way to compute the
 * maximum value in the array.
 *
 * So I'm seeing this as a multi-phase solution. In the first phase,
 * each block should compute the maximum value of the array
 * corresponding to that block. In the second phase, some subset of
 * the blocks should compute the maximum values from the computed
 * maxima of all the blocks that did work in the first phase.
 * Repeat until only one block is considering all the maximal values,
 * and the result of this is guaranteed to be the maximum array value.
 * You can consider things like shared memory, data distribution, etc.
 * to increase performance.

Example: A = {3, 10, 1, 9, 2, 8, 3, 4, 8, 2, 1, 7, 2, 5, 6, 1, 2, 5, 3, 2}
Using 5 blocks to handle 4 elements each:

Phase 1:
{3, 10, 1, 9} {2, 8, 3, 4} {8, 2, 1, 7} {2, 5, 6, 1} {2, 5, 3, 2}
{10, 10, 9, 9} {8, 8, 4, 4} {8, 2, 7, 7} {5, 5, 6, 1} {5, 5, 3, 2}
{10, 10, 9, 9} {8, 8, 4, 4} {8, 2, 7, 7} {6, 5, 6, 1} {5, 5, 3, 2}

Phase 2:
{10, 8, 8, 6} {5}
{10, 8, 8, 6} {5}
{10, 8, 8, 6} {5}

Phase 3:
{10, 5}
{10, 5}

Maximum is 10

 * Question is: can it bring any real performance plus?
 * This grGetHOT() is only used in the F10 camera view.
 * Should look up if there are any other source files that use
 * a similar approach to find max, sum, avg or min value in an array.
 *
*/
