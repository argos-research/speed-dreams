/***************************************************************************

 file        : ac3dload.cpp
 created     : Fri Apr 18 23:00:28 CEST 2003
 copyright   : (C) 2003 by Christophe Guionneau
 version     : $Id: ac3dload.cpp 5451 2013-05-12 23:47:53Z torcs-ng $

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

 @author Christophe Guionneau
 @version    $Id: ac3dload.cpp 5451 2013-05-12 23:47:53Z torcs-ng $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cstring>
#include <sstream>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <math.h>
#include <float.h>
#include "portability.h"
#include "accc.h"

#define AC3D     "AC3Db"
#define MATERIAL "MATERIAL"
#define OBJECT   "OBJECT"
#define KIDS     "Kids"
#define NAME     "name"
#define LOC      "loc"
#define DATA     "data"
#define TEXTURE  "texture"
#define TEXREP   "texrep"
#define NUMVERT  "numvert"
#define NUMSURF  "numsurf"
#define SURF     "SURF"
#define MAT      "mat"
#define REFS     "refs"
#define CREASE   "crease"

#define SetPoint(p, a, b, c) (p).x = a; (p).y = b; (p).z = c;

void copyPoint(point_t * dest, point_t * src)
{
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
}

void copyTexCoord(tcoord_t * dest, tcoord_t * src)
{
    dest->indice = src->indice;
    dest->u = src->u;
    dest->v = src->v;
    dest->saved = src->saved;
}
void storeTexCoord(tcoord_t * dest, int indice, double u, double v, int saved)
{
    dest->indice = indice;
    dest->u = u;
    dest->v = v;
    dest->saved = saved;
}

ob_t * obCreate()
{
    ob_t * ob = (ob_t *) malloc(sizeof(ob_t));
    memset(ob, 0, sizeof(ob_t));

    return ob;
}

void obFree(ob_t *o)
{
    freez(o->name);
    freez(o->type);
    freez(o->texture);
    freez(o->vertex);
    freez(o->norm);
    freez(o->snorm);
    freez(o->vertexarray);
    freez(o->vertexarray1);
    freez(o->vertexarray2);
    freez(o->vertexarray3);
    freez(o->textarray);
    freez(o->textarray1);
    freez(o->textarray2);
    freez(o->textarray3);
}

ob_t * obAppend(ob_t * destob, ob_t * srcob)
{
    if(!destob)
        return srcob;
    if(!srcob)
        return destob;

    ob_t * curob = destob;

    while(curob->next != NULL)
        curob = curob->next;

    curob->next = srcob;

    return destob;
}

void obInitSpacialExtend(ob_t * ob)
{
    ob->x_min = ob->y_min = ob->z_min = DBL_MAX;
    ob->x_max = ob->y_max = ob->z_max = DBL_MIN;

    for (int v = 0; v < ob->numvertice; v++)
    {
        if (ob->vertex[v].x > ob->x_max)
            ob->x_max = ob->vertex[v].x;
        if (ob->vertex[v].x < ob->x_min)
            ob->x_min = ob->vertex[v].x;

        if (ob->vertex[v].y > ob->y_max)
            ob->y_max = ob->vertex[v].y;
        if (ob->vertex[v].y < ob->y_min)
            ob->y_min = ob->vertex[v].y;

        if (ob->vertex[v].z > ob->z_max)
            ob->z_max = ob->vertex[v].z;
        if (ob->vertex[v].z < ob->z_min)
            ob->z_min = ob->vertex[v].z;
    }
}

/** Helper method for obCreateTextArrays().
 *  Copies the u/v coords of the given texcoord into the given textarray, based
 *  on texcoord->indice property. */
void copyTexCoordToTextArray(double * textarray, tcoord_t * texcoord)
{
    int fstIdx = texcoord->indice * 2;
    textarray[fstIdx] = texcoord->u;
    textarray[fstIdx + 1] = texcoord->v;
}

void obCreateTextArrays(ob_t * ob)
{
    const int numEls = ob->numvertice * 2;
    const int elSize = sizeof(tcoord_t);

    if(ob->vertexarray)
        ob->textarray = (double *) calloc(numEls, elSize);
    if(ob->vertexarray1)
        ob->textarray1 = (double *) calloc(numEls, elSize);
    if(ob->vertexarray2)
        ob->textarray2 = (double *) calloc(numEls, elSize);
    if(ob->vertexarray3)
        ob->textarray3 = (double *) calloc(numEls, elSize);

    for (int i = 0; i < ob->numsurf * 3; i++)
    {
        copyTexCoordToTextArray(ob->textarray, &ob->vertexarray[i]);
        if(ob->vertexarray1)
            copyTexCoordToTextArray(ob->textarray1, &ob->vertexarray1[i]);
        if(ob->vertexarray2)
            copyTexCoordToTextArray(ob->textarray2, &ob->vertexarray2[i]);
        if(ob->vertexarray3)
            copyTexCoordToTextArray(ob->textarray3, &ob->vertexarray3[i]);
    }
}

void obCreateVertexArrays(ob_t * ob)
{
    int numEls = ob->numsurf * 3;

    if(ob->texture)
        ob->vertexarray = (tcoord_t *) calloc(numEls, sizeof(tcoord_t));

    if(ob->texture1)
        ob->vertexarray1 = (tcoord_t *) calloc(numEls, sizeof(tcoord_t));

    if(ob->texture2)
        ob->vertexarray2 = (tcoord_t *) calloc(numEls, sizeof(tcoord_t));

    if(ob->texture3)
        ob->vertexarray3 = (tcoord_t *) calloc(numEls, sizeof(tcoord_t));
}

void obCopyTextureNames(ob_t * destob, ob_t * srcob)
{
    if(srcob->texture)
        destob->texture = strdup(srcob->texture);
    if(srcob->texture1)
        destob->texture1 = strdup(srcob->texture1);
    if(srcob->texture2)
        destob->texture2 = strdup(srcob->texture2);
    if(srcob->texture3)
        destob->texture3 = strdup(srcob->texture3);
}

void obSetVertexArraysIndex(ob_t * ob, int vaIdx, int newIndex)
{

    ob->vertexarray[vaIdx].indice = newIndex;

    if(ob->vertexarray1)
        ob->vertexarray1[vaIdx].indice = newIndex;
    if(ob->vertexarray2)
        ob->vertexarray2[vaIdx].indice = newIndex;
    if(ob->vertexarray3)
        ob->vertexarray3[vaIdx].indice = newIndex;
}

#ifndef M_PI
#define M_PI 3.14159267
#endif
extern int far_dist;
void computeTriNorm(ob_t * object);
void smoothTriNorm(ob_t * object);
void computeObjectTriNorm(ob_t * object);
void smoothFaceTriNorm(ob_t * object);
void smoothObjectTriNorm(ob_t * object);
void normalize(point_t *t);
int checkMustSmoothVector(point_t *n1, point_t *n2, point_t *t1, point_t *t2);
void mapNormalToSphere(ob_t * object);
void mapNormalToSphere2(ob_t * object);
void normalMap(ob_t * object);
void mapTextureEnv(ob_t * object);
ob_t ob[10000];
/*mat_t  material[10000];*/
mat_t * root_material = NULL;
point_t tmpPoint[100000];
tcoord_t tmpva[100000];
double tmptexa[200000];
int tmpsurf[100000];
ob_t * root_ob;
int refs = 0;
char * const shadowtexture = strdup("shadow2.png");

FILE * ofile;

int numob = 0;
int nummaterial = 0;
int numvertex = 0;
int numvertFound = 0;
int numrefsFound = 0;
int attrSurf = 0;
int attrMat = 0;
int numrefs = 0;
int numrefstotal = 0;
char tmpname[256];
int tmpIndice = 0;
int tmpIndice2 = 0;
int tmpIndice3 = 0;
int vert;
int numvertice = 0;
char tex[256][256];
int texnum = 0;
double smooth_angle = 70.0;
typedef struct verbaction
{
    const char * verb;
    int (*doVerb)(char * Line, ob_t *object, mat_t * material);
} verbaction_t;

int doMaterial(char *Line, ob_t *object, mat_t *material);
int doObject(char *Line, ob_t *object, mat_t *material);
int doKids(char *Line, ob_t *object, mat_t *material);
int doName(char *Line, ob_t *object, mat_t *material);
int doLoc(char *Line, ob_t *object, mat_t *material);
int doData(char *Line, ob_t *object, mat_t *material);
int doTexture(char *Line, ob_t *object, mat_t *material);
int doTexrep(char *Line, ob_t *object, mat_t *material);
int doNumvert(char *Line, ob_t *object, mat_t *material);
int doNumsurf(char *Line, ob_t *object, mat_t *material);
int doSurf(char *Line, ob_t *object, mat_t *material);
int doMat(char *Line, ob_t *object, mat_t *material);
int doRefs(char *Line, ob_t *object, mat_t *material);
int doCrease(char *Line, ob_t *object, mat_t *material);

#ifdef _3DS
void saveObin3DS( char * OutputFilename, ob_t * object);
#endif
void computeSaveAC3D(char * OutputFilename, ob_t * object);
void computeSaveOBJ(char * OutputFilename, ob_t * object);
void computeSaveAC3DM(char * OutputFilename, ob_t * object);
void computeSaveAC3DStrip(char * OutputFilename, ob_t * object);
void stripifyOb(ob_t * object, int writeit);

verbaction_t verbTab[] =
{
{ MATERIAL, doMaterial },
{ OBJECT, doObject },
{ KIDS, doKids },
{ NAME, doName },
{ LOC, doLoc },
{ DATA, doData },
{ TEXTURE, doTexture },
{ TEXREP, doTexrep },
{ NUMVERT, doNumvert },
{ NUMSURF, doNumsurf },
{ SURF, doSurf },
{ MAT, doMat },
{ REFS, doRefs },
{ CREASE, doCrease },
{ "END", NULL } };

void copyVertexArraysSurface(ob_t * destob, int destSurfIdx, ob_t * srcob, int srcSurfIdx)
{
    int firstDestIdx = destSurfIdx * 3;
    int firstSrcIdx = srcSurfIdx * 3;

    for(int off = 0; off < 3; off++)
    {
        copyTexCoord(&(destob->vertexarray[firstDestIdx + off]),
                     &(srcob->vertexarray[firstSrcIdx + off]));

        if(srcob->vertexarray1)
            copyTexCoord(&(destob->vertexarray1[firstDestIdx + off]),
                         &(srcob->vertexarray1[firstSrcIdx + off]));
        if(srcob->vertexarray2)
            copyTexCoord(&(destob->vertexarray2[firstDestIdx + off]),
                         &(srcob->vertexarray2[firstSrcIdx + off]));
        if(srcob->vertexarray3)
            copyTexCoord(&(destob->vertexarray3[firstDestIdx + off]),
                         &(srcob->vertexarray3[firstSrcIdx + off]));
    }
}

/** copy the (u,v) coords from srcidxarr to the corresponding position in destarr.
 *  destarr needs to have 2 * number of vertices entries.
 */
void createTexCoordArray(double * destarr, tcoord_t * srcidxarr, int numidx)
{
    tcoord_t * curidxobj = NULL;
    int curidx = 0;

    for (curidx = 0; curidx < numidx; curidx++)
    {
        curidxobj = &srcidxarr[curidx];

        destarr[curidxobj->indice * 2] = curidxobj->u;
        destarr[curidxobj->indice * 2 + 1] = curidxobj->v;
    }
}

/** Creates a new object from the given data. This function is used during object splitting.
 *
 *  @param splitid the id of this split object
 *  @param srcobj the original object, which was split
 *  @param tmpob the temporary storage object using in splitting
 *
 *  In the tmpob the following should be set: numvertice, numsurf, vertex, snorm,
 *  vertexarray, textarray
 *
 *  @return the created split object
 */
ob_t * createObjectSplitCopy(int splitid, ob_t * srcobj, ob_t * tmpob)
{
    int numtri = tmpob->numsurf;
    int numvert = tmpob->numvertice;

    int size_pts = sizeof(point_t) * numvert;

    /* allocate space */
    ob_t * retob = obCreate();

    retob->vertex = (point_t*) malloc(size_pts);
    memset(retob->vertex, 0, size_pts);
    retob->norm = (point_t*) malloc(size_pts);
    memset(retob->norm, 0, size_pts);
    retob->snorm = (point_t*) malloc(size_pts);
    memset(retob->snorm, 0, size_pts);

    /* assign data */
    createTexChannelArrays(retob, tmpob);

    retob->attrSurf = srcobj->attrSurf;
    retob->attrMat = srcobj->attrMat;

    if (srcobj->data)
        retob->data = strdup(srcobj->data);

    memcpy(retob->vertex, tmpob->vertex, size_pts);
    memcpy(retob->snorm, tmpob->snorm, size_pts);
    memcpy(retob->norm, tmpob->snorm, size_pts);

    obCopyTextureNames(retob, srcobj);

    retob->numsurf = numtri;
    retob->numvert = numvert;
    retob->numvertice = numvert;

    retob->type = strdup(srcobj->type);

    /* special handling of name */
    std::stringstream namestream;

    namestream << srcobj->name << "_s_" << splitid;
    retob->name = strdup(namestream.str().c_str());

    return retob;
}
void copyTexChannel(double * desttextarray, tcoord_t * destvertexarray, tcoord_t * srcvert,
    int storedptidx, int destptidx, int destvertidx)
{
    desttextarray[destptidx * 2] = srcvert->u;
    desttextarray[destptidx * 2 + 1] = srcvert->v;

    storeTexCoord(&(destvertexarray[destvertidx]),
            storedptidx, srcvert->u, srcvert->v, 0);
}

void copySingleVertexData(ob_t * destob, ob_t * srcob,
    int storedptidx, int destptidx, int destvertidx, int srcvertidx)
{
    tcoord_t * srcvert;

    /* channel 0 */
    if(destob->textarray != NULL)
    {
        srcvert = &(srcob->vertexarray[srcvertidx]);

        copyTexChannel(destob->textarray, destob->vertexarray, srcvert,
                storedptidx, destptidx, destvertidx);
    }

    /* channel 1 */
    if(destob->textarray1 != NULL)
    {
        srcvert = &(srcob->vertexarray1[srcvertidx]);

        copyTexChannel(destob->textarray1, destob->vertexarray1, srcvert,
                storedptidx, destptidx, destvertidx);
    }

    /* channel 2 */
    if(destob->textarray2 != NULL)
    {
        srcvert = &(srcob->vertexarray2[srcvertidx]);

        copyTexChannel(destob->textarray2, destob->vertexarray2, srcvert,
                storedptidx, destptidx, destvertidx);
    }

    /* channel 3 */
    if(destob->textarray3 != NULL)
    {
        srcvert = &(srcob->vertexarray3[srcvertidx]);

        copyTexChannel(destob->textarray3, destob->vertexarray3, srcvert,
                storedptidx, destptidx, destvertidx);
    }
}

void clearSavedInVertexArrayEntry(ob_t * ob, int vertidx)
{
    ob->vertexarray[vertidx].saved = 0;
}

void createSingleTexChannelArrays(ob_t * destob, ob_t * srcob, int channel)
{
    int size_va = srcob->numsurf * 3 * sizeof(tcoord_t);
    int size_ta = 2 * srcob->numvertice * sizeof(double);

    tcoord_t * va = (tcoord_t *) malloc(size_va);
    double* ta = (double *) malloc(size_ta);

    switch(channel)
    {
    case 0:
        memcpy(va, srcob->vertexarray, size_va);
        memcpy(ta, srcob->textarray, size_ta);

        destob->vertexarray = va;
        destob->textarray = ta;
        break;

    case 1:
        memcpy(va, srcob->vertexarray1, size_va);
        memcpy(ta, srcob->textarray1, size_ta);

        destob->vertexarray1 = va;
        destob->textarray1 = ta;
        break;

    case 2:
        memcpy(va, srcob->vertexarray2, size_va);
        memcpy(ta, srcob->textarray2, size_ta);

        destob->vertexarray2 = va;
        destob->textarray2 = ta;
        break;

    case 3:
        memcpy(va, srcob->vertexarray3, size_va);
        memcpy(ta, srcob->textarray3, size_ta);

        destob->vertexarray3 = va;
        destob->textarray3 = ta;
        break;
    }
}

void createTexChannelArrays(ob_t * destob, ob_t * srcob)
{

    if(srcob->vertexarray != NULL)
        createSingleTexChannelArrays(destob, srcob, 0);
    if(srcob->vertexarray1 != NULL)
        createSingleTexChannelArrays(destob, srcob, 1);
    if(srcob->vertexarray2 != NULL)
        createSingleTexChannelArrays(destob, srcob, 2);
    if(srcob->vertexarray3 != NULL)
        createSingleTexChannelArrays(destob, srcob, 3);
}


int computeNorm(point_t * pv1, point_t *pv2, point_t *pv3, point_t *norm)
{
    double p1, p2, p3, q1, q2, q3, dd;
    double x1, y1, z1, x2, y2, z2, x3, y3, z3;

    x1 = pv1->x;
    y1 = pv1->y;
    z1 = pv1->z;

    x2 = pv2->x;
    y2 = pv2->y;
    z2 = pv2->z;

    x3 = pv3->x;
    y3 = pv3->y;
    z3 = pv3->z;

    if (((x1 == x2) && (y1 == y2) && (z1 == z2))
            || ((x1 == x3) && (y1 == y3) && (z1 == z3))
            || ((x2 == x3) && (y2 == y3) && (z2 == z3)))
    {
        norm->x = 0;
        norm->y = 1.0;
        norm->z = 0;
        return 0;
    }

    p1 = x2 - x1;
    p2 = y2 - y1;
    p3 = z2 - z1;

    q1 = x3 - x1;
    q2 = y3 - y1;
    q3 = z3 - z1;

    dd = sqrt(
            (p2 * q3 - q2 * p3) * (p2 * q3 - q2 * p3)
                    + (p3 * q1 - q3 * p1) * (p3 * q1 - q3 * p1)
                    + (p1 * q2 - q1 * p2) * (p1 * q2 - q1 * p2));
    if (dd == 0.0)
    {
        norm->x = 0;
        norm->y = 1.0;
        norm->z = 0;
        return 0;
    }

    norm->x = (p2 * q3 - q2 * p3) / dd;
    norm->y = (p3 * q1 - q3 * p1) / dd;
    norm->z = (p1 * q2 - q1 * p2) / dd;

    if (isnan(norm->x) || isnan(norm->y) || isnan(norm->z))
    {
        norm->x = 0;
        norm->y = 1.0;
        norm->z = 0;
        return 0;
    }

    return 0;
}

void computeObSurfCentroid(ob_t * ob, int obsurf, point_t * out)
{
    tcoord_t * idx = ob->vertexarray;
    point_t * vert = ob->vertex;

    int firstIdx = obsurf * 3;

    out->x = 0;
    out->y = 0;
    out->z = 0;

    int curVert;
    for(curVert = 0; curVert < 3; curVert++)
    {
        out->x += vert[idx[firstIdx + curVert].indice].x;
        out->y += vert[idx[firstIdx + curVert].indice].y;
        out->z += vert[idx[firstIdx + curVert].indice].z;
    }

    out->x /= 3;
    out->y /= 3;
    out->z /= 3;
}

int doMaterial(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    char name[256];
    mat_t * materialt, *t1;

    materialt = (mat_t*) malloc(sizeof(mat_t));
    memset(materialt, '\0', sizeof(mat_t));

    nummaterial++;

    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown MATERIAL format %s \n", Line);
        return (-1);
    }
    sscanf(p,
            "%s rgb %lf %lf %lf amb %lf %lf %lf emis %lf %lf %lf spec %lf %lf %lf shi %d trans %lf",
            name, &(materialt->rgb.r), &(materialt->rgb.g), &(materialt->rgb.b),
            &(materialt->amb.r), &(materialt->amb.g), &(materialt->amb.b),
            &(materialt->emis.r), &(materialt->emis.g), &(materialt->emis.b),
            &(materialt->spec.r), &(materialt->spec.g), &(materialt->spec.b),
            &(materialt->shi), &(materialt->trans));

    materialt->name = strdup(name);
    t1 = material->next;
    material->next = materialt;
    materialt->next = t1;

    return (0);
}

int doObject(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    char name[256];
    ob_t * objectt, *t1;

    objectt = (ob_t*) malloc(sizeof(ob_t));
    memset(objectt, '\0', sizeof(ob_t));
    objectt->x_min = 1000000;
    objectt->y_min = 1000000;
    objectt->z_min = 1000000;

    numob++;
    numrefs = 0;
    numvertFound = 0;

    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown OBJECT format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%s", name);

    objectt->type = strdup(name);

    t1 = object->next;
    object->next = objectt;
    objectt->next = t1;
    object->next->texrep_x = 1.0;
    object->next->texrep_y = 1.0;

    return (0);
}

int findIndice(int indice, int *oldva, int n)
{
    int i = 0;
    for (i = 0; i < n; i++)
    {
        if (oldva[i] == indice)
            return i;
    }
    return -1;
}

ob_t* terrainSplitOb(ob_t * object)
{
    ob_t * tob = NULL;
    ob_t * tob0 = NULL;

    printf("terrain splitting %s \n", object->name);
    if ((object->x_max - object->x_min) < 2 * distSplit)
        return 0;
    if ((object->y_max - object->y_min) < 2 * distSplit)
        return 0;
    printf("terrain splitting %s started\n", object->name);

    int numSurf = object->numsurf;
    int *oldSurfToNewObjMap = (int *) calloc(numSurf, sizeof(int));

    int numNewObjs = 0;

    for (double curXPos = object->x_min; curXPos < object->x_max; curXPos += distSplit)
    {
        for (double curYPos = object->y_min; curYPos < object->y_max; curYPos += distSplit)
        {

            int numTriFound = 0;
            int found_a_tri = 0;

            for (int curObjSurf = 0; curObjSurf < object->numsurf; curObjSurf++)
            {
                point_t surfCentroid;
                computeObSurfCentroid(object, curObjSurf, &surfCentroid);

                if (surfCentroid.x >= curXPos && surfCentroid.x < curXPos + distSplit)
                {
                    if (surfCentroid.y >= curYPos && surfCentroid.y < curYPos + distSplit)
                    {
                        found_a_tri = 1;
                        oldSurfToNewObjMap[curObjSurf] = numNewObjs;
                        numTriFound++;
                    }
                }
            }

            if (found_a_tri)
            {
                printf("surface num %d : numtri : %d\n", numNewObjs, numTriFound);
                numNewObjs++;
            }
        }

    }
    printf("found in %s : %d subsurfaces \n", object->name, numNewObjs);

    for (int curNewObj = 0; curNewObj < numNewObjs; curNewObj++)
    {
        int numNewSurf = 0;
        /* find the number of surface */
        for (int curSurf = 0; curSurf < object->numsurf; curSurf++)
        {
            if (oldSurfToNewObjMap[curSurf] != curNewObj)
                continue;
            numNewSurf++;
        }

        /* initial creation of tob */

        tob = obCreate();

        tob->numsurf = numNewSurf;
        tob->attrSurf = object->attrSurf;
        tob->attrMat = object->attrMat;
        if (object->data)
            tob->data = strdup(object->data);
        tob->type = strdup(object->type);

        /* special name handling */
        std::stringstream namestream;
        namestream << object->name << "__split__" << curNewObj;
        tob->name = strdup(namestream.str().c_str());

        obCopyTextureNames(tob, object);

        /* store the index data in tob's vertexarray */

        obCreateVertexArrays(tob);

        int curNewSurf = 0;
        for (int curSurf = 0; curSurf < object->numsurf; curSurf++)
        {
            if (oldSurfToNewObjMap[curSurf] != curNewObj)
                continue;

            copyVertexArraysSurface(tob, curNewSurf, object, curSurf);

            curNewSurf++;
        }

        /* create a list with temporal points and smoothed normals and store the index
         * to them in tob's vertexarray.indice property.
         */

        /* temporal storage for points and smoothed normals. Temporal because
         * we don't know the size, so we allocate the same number as in the
         * source object.
         */
        point_t* pttmp = (point_t*) calloc(object->numvertice, sizeof(point_t));
        point_t* snorm = (point_t*) calloc(object->numvertice, sizeof(point_t));

        /* storedPtIdxArr: keep a list of the indices of points stored in the new object.
         * If an index is contained in storedPtIdxArr we don't store the point itself,
         * but only the index in the vertexarray of the new object.
         */
        int* storedPtIdxArr = (int*) calloc(object->numvertice, sizeof(int));

        int curNewPtIdx = 0;
        for (int curNewIdx = 0; curNewIdx < numNewSurf * 3; curNewIdx++)
        {
            int idx = tob->vertexarray[curNewIdx].indice;

            int storedIdx = findIndice(idx, storedPtIdxArr, curNewPtIdx);
            if (storedIdx == -1)
            {
                storedPtIdxArr[curNewPtIdx] = idx;
                storedIdx = curNewPtIdx;
                copyPoint(&(pttmp[curNewPtIdx]), &(object->vertex[idx]));
                copyPoint(&(snorm[curNewPtIdx]), &(object->norm[idx]));
                curNewPtIdx++;
            }

            obSetVertexArraysIndex(tob, curNewIdx, storedIdx);
        }

        free(storedPtIdxArr);

        int numNewPts = curNewPtIdx;

        tob->numvert = numNewPts;
        tob->numvertice = numNewPts;

        /* create and store tob's norm, snorm, vertex and textarray data */

        tob->norm = (point_t*) calloc(numNewPts, sizeof(point_t));
        memcpy(tob->norm, snorm, numNewPts * sizeof(point_t));

        tob->snorm = (point_t*) calloc(numNewPts, sizeof(point_t));
        memcpy(tob->snorm, snorm, numNewPts * sizeof(point_t));

        tob->vertex = (point_t*) calloc(numNewPts, sizeof(point_t));
        memcpy(tob->vertex, pttmp, numNewPts * sizeof(point_t));

        free(pttmp);
        free(snorm);

        obCreateTextArrays(tob);

        obInitSpacialExtend(tob);

        // prepend the new object to the list
        tob0 = obAppend(tob, tob0);

    }

    return tob0;
}

ob_t* splitOb(ob_t *object)
{
    int *oldva = 0;
    int curtri = 0;
    int numptstored = 0; /* number of vertices stored */
    int oldnumptstored = 0; /* temporary placeholder for numptstored */

    /* The object we use as storage during splitting.
     * Following attribs will be used: vertexarray, vertex, snorm, textarray
     */
    ob_t * workob;

    tcoord_t curvertex[3];
    int curstoredidx[3];
    int i = 0;

    int touse = 0;
    int orignumtris = 0; /* number of surfaces/triangles in the source object */
    int orignumverts = 0; /* number of vertices in the source object: orignumtris * 3 */
    int * tri;
    int numvertstored = 0; /* number of vertices stored in the object */
    int numtristored = 0; /* number of triangles stored in the object: numvertstored/3 */
    int mustcontinue = 1;
    ob_t * tob = NULL;
    ob_t * tob0 = NULL;
    int numob = 0;
    int firstTri = 0;
    int atleastone = 0;
    int curvert = 0;

    orignumtris = object->numsurf;
    orignumverts = orignumtris * 3;

    tri = (int *) calloc(orignumtris, sizeof(int));
    oldva = (int *) calloc(orignumverts, sizeof(int));

    workob = obCreate();
    workob->vertex = (point_t *) calloc(orignumverts, sizeof(point_t));
    workob->snorm = (point_t *) calloc(orignumverts, sizeof(point_t));

    // create texture channels
    createTexChannelArrays(workob, object);

    while (mustcontinue == 1)
    {
        numvertstored = 0;
        numtristored = 0;

        numptstored = 0;
        mustcontinue = 0;
        firstTri = 0;
        atleastone = 1;
        while (atleastone == 1)
        {
            atleastone = 0;
            for (curtri = 0; curtri < orignumtris; curtri++)
            {
                touse = 0;
                if (tri[curtri] == 1)
                    continue;
                mustcontinue = 1;

                curvert = curtri * 3;

                /** find vertices of the triangle */
                for(i = 0; i < 3; i++)
                {
                    copyTexCoord(&curvertex[i], &(object->vertexarray[curvert+i]));

                    curstoredidx[i] = findIndice(curvertex[i].indice, oldva, numptstored);
                }

                if (curstoredidx[0] == -1 && curstoredidx[1] == -1 && curstoredidx[2] == -1)
                {
                    if (firstTri == 0)
                        touse = 1;
                    else
                        touse = 0;
                    /* triangle is ok */
                }
                else
                {
                    touse = 1;

                    for(i = 0; i < 3; i++)
                    {
                        if (curstoredidx[i] != -1)
                            if(workob->textarray[curstoredidx[i] * 2] != curvertex[i].u
                            || workob->textarray[curstoredidx[i] * 2 + 1] != curvertex[i].v)
                            {
                                touse = 0;
                                /* triangle is not ok */
                            }
                    }
                }

                if (touse == 1)
                {
                    firstTri = 1;
                    /* triangle is ok */

                    tri[curtri] = 1; /* mark this triangle */

                    /* store the vertices of the triangle with new indice */
                    /* not yet in the array : store it at the current position */
                    for(i = 0; i < 3; i++)
                    {
                        oldnumptstored = numptstored;

                        if (curstoredidx[i] == -1)
                        {
                            copyPoint(&(workob->vertex[numptstored]), &(object->vertex[curvertex[i].indice]));
                            copyPoint(&(workob->snorm[numptstored]), &(object->norm[curvertex[i].indice]));

                            clearSavedInVertexArrayEntry(object, curvert+i);

                            oldva[numptstored] = curvertex[i].indice; /* remember the value of the vertice already saved */
                            curstoredidx[i] = numptstored;
                            numptstored++;
                        }

                        copySingleVertexData(workob, object, curstoredidx[i],
                                oldnumptstored, numvertstored, curvert+i);

                        numvertstored++;
                    }

                    numtristored++;
                    atleastone = 1;

                } // if (touse == 1)

            } // for (curtri = 0; curtri < orignumtris; curtri++)
        } // while (atleastone == 1)

        if (numtristored == 0)
            continue;

        /* must saved the object */
        workob->numvertice = numptstored;
        workob->numsurf = numvertstored/3;

        tob = createObjectSplitCopy(numob++, object, workob);

        attrSurf = tob->attrSurf;
        attrMat = tob->attrMat;

        // prepend the new object to the list
        tob0 = obAppend(tob, tob0);

        printf("numtri = %d on orignumtris =%d \n", numtristored, orignumtris);

    } // while (mustcontinue == 1)

    freez(tri);
    freez(oldva);
    obFree(workob);

    return tob0;
}

int doKids(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    int kids;

    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Kids format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%d", &kids);
    if (kids == 0)
    {
        object->next->vertexarray = (tcoord_t *) malloc(
                sizeof(tcoord_t) * numrefstotal);
        object->next->textarray = (double *) malloc(
                sizeof(double) * numrefstotal * 2);
        object->next->surfrefs = (int *) malloc(sizeof(int) * numrefs);
        object->next->norm = (point_t*) malloc(
                sizeof(point_t) * numrefstotal * 3);
        object->next->snorm = (point_t*) malloc(
                sizeof(point_t) * numrefstotal * 3);
        object->next->attrSurf = attrSurf;
        object->next->attrMat = attrMat;
        attrSurf = 0x20;
        memset(object->next->snorm, 0, sizeof(point_t) * numrefstotal * 3);
        memset(object->next->norm, 0, sizeof(point_t) * numrefstotal * 3);

        memcpy(object->next->vertexarray, tmpva,
                numrefstotal * sizeof(tcoord_t));
        memcpy(object->next->textarray, tmptexa,
                numrefstotal * 2 * sizeof(double));
        memcpy(object->next->surfrefs, tmpsurf, numrefs * sizeof(int));
        object->next->numvertice = numvertice;

        if ((object->next->name) == NULL)
        {
            std::stringstream namestream;
            namestream << tmpname << tmpIndice;
            object->next->name = strdup(namestream.str().c_str());

            tmpIndice++;
        }

        if ((typeConvertion == _AC3DTOAC3DS
                && (extendedStrips == 1 || extendedTriangles == 1))
                || typeConvertion == _AC3DTOAC3DGROUP
                || (typeConvertion == _AC3DTOAC3D && (extendedTriangles == 1)))
        {
            printf("Computing normals for %s \n", object->next->name);
            computeObjectTriNorm(object->next);
            //smoothObjectTriNorm(object->next );
        }

        numrefs = numrefstotal = 0;
        numvertFound = 0;
        numrefsFound = 0;
        numvertex = 0;
        numvertice = 0;

    }
    else
        object->next->kids = kids;

    return (0);
}

int doName(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    char *q;
    char name[256];
    char name2[256];
    p = strstr(Line, "\"");
    if (p == NULL)
    {
        fprintf(stderr, "unknown name format %s \n", Line);
        return (-1);
    }
    else
        p++;
    sprintf(name, "%s", p);
    p = strstr(name, "\n");
    if (p != NULL)
        *p = '\0';

    if (!strcmp("\"n\"", name))
    {
        sprintf(name, "terrain%d", tmpIndice2++);
    }
    if (!strcmp("\"NoName\"", name))
    {
        sprintf(name, "ob%d", tmpIndice3++);
    }
    p = name;
    q = name2;
    while (*p)
    {
        if ((*p <= 'z' && *p >= 'a'))
        {
            *p = (*p - 'a') + 'A';
        }
        if ((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
        {
            *q = *p;
            q++;
            *q = '\0';
        }
        p++;
    }
    sprintf(name, "%s", name2);

    /*sprintf(name,"terrain%d",tmpIndice2++);*/
    object->next->name = strdup(name);
    sprintf(tmpname, "%s", name);

    fprintf(stderr, "loading  %s object                             \r", name);
    printf("loading  %s object\n", name);
    tmpIndice = 0;
    return (0);
}

int doLoc(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Loc format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%lf %lf %lf", &(object->next->loc.x), &(object->next->loc.y),
            &(object->next->loc.z));

    return (0);
}

int doData(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Loc format %s \n", Line);
        return (-1);
    }
    object->next->data = strdup(p);
    return (0);
}

int doCrease(char *Line, ob_t *object, mat_t *material)
{
    return (0);
}

int doTexture(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    char name[256];
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown texture format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%s", name);
    p = NULL;
    p = strstr(name, "\"");
    if (p != NULL)
    {
        p++;
        object->next->texture = strdup(p);
    }
    else
        object->next->texture = strdup(name);
    p = strstr(object->next->texture, "\"");
    if (p != NULL)
        *p = '\0';
    return (0);
}

int doTexrep(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Texrep format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%lf %lf", &(object->next->texrep_x), &(object->next->texrep_y));

    return (0);
}

int doNumvert(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown numvert format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%d", &(object->next->numvert));
    object->next->vertex = (point_t *) malloc(
            sizeof(point_t) * object->next->numvert);
    numvertex = 0;
    numvertFound = 1;
    return (0);
}

int doNumsurf(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown numsurf format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%d", &(object->next->numsurf));
    numvertFound = 0;
    return (0);
}

int doGetVertex(char *Line, ob_t *object, mat_t *material)
{
    sscanf(Line, "%lf %lf %lf ", &(object->next->vertex[numvertex].x),
            &(object->next->vertex[numvertex].z),
            &(object->next->vertex[numvertex].y));
    object->next->vertex[numvertex].x += object->next->loc.x;
    object->next->vertex[numvertex].y += object->next->loc.z;
    object->next->vertex[numvertex].z += object->next->loc.y;
    object->next->vertex[numvertex].y = -object->next->vertex[numvertex].y;
    /* compute min/max of the vertex for this object */
    if (object->next->vertex[numvertex].x > object->next->x_max)
        object->next->x_max = object->next->vertex[numvertex].x;
    if (object->next->vertex[numvertex].x < object->next->x_min)
        object->next->x_min = object->next->vertex[numvertex].x;

    if (object->next->vertex[numvertex].y > object->next->y_max)
        object->next->y_max = object->next->vertex[numvertex].y;
    if (object->next->vertex[numvertex].y < object->next->y_min)
        object->next->y_min = object->next->vertex[numvertex].y;

    if (object->next->vertex[numvertex].z > object->next->z_max)
        object->next->z_max = object->next->vertex[numvertex].z;
    if (object->next->vertex[numvertex].z < object->next->z_min)
        object->next->z_min = object->next->vertex[numvertex].z;

    numvertex++;
    /*fprintf(stderr,"numvertex = %d \n",numvertex);*/
    return (0);
}

int doGetSurf(char *Line, ob_t *object, mat_t *material)
{
    /*  double u,v;*/

    sscanf(Line, "%d %lf %lf ", &(tmpva[numvertice].indice),
            &(tmpva[numvertice].u), &(tmpva[numvertice].v));
    /*fprintf(stderr,"numrefs = %d \n",numrefs);*/
    /*printf("%.2lf %.2lf \n",tmpva[numvertice].u,tmpva[numvertice].v);*/
    tmpva[numvertice].saved = 0;
    tmptexa[tmpva[numvertice].indice * 2] = tmpva[numvertice].u
            * object->next->texrep_x;
    tmptexa[tmpva[numvertice].indice * 2 + 1] = tmpva[numvertice].v
            * object->next->texrep_y;
    numvertice++;
    return (0);
}

int doSurf(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown SURF format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%xd", &attrSurf);
    numvertFound = 0;
    return (0);
}

int doMat(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown mat format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%d", &attrMat);
    numvertFound = 0;
    return (0);
}

int doRefs(char *Line, ob_t *object, mat_t *material)
{
    char * p;
    p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Refs format %s \n", Line);
        return (-1);
    }
    sscanf(p, "%d", &refs);

    numrefstotal += refs;
    numrefsFound = 1;
    tmpsurf[numrefs] = refs;
    numrefs++;
    return (0);
}

/* We need to split an object face in more faces
 * if there are common points with different texture coordinates.
 */
bool isObjectSplit(ob_t* object)
{
    if (notexturesplit == 1)
        return false;

    bool same_pt = false, diff_u = false, diff_v = false;
    int numverts = object->numvertice;

    for (int i = 0; i < numverts; i++)
    {
        for (int j = i + 1; j < numverts; j++)
        {
            same_pt = (object->vertexarray[i].indice
                    == object->vertexarray[j].indice);
            diff_u = (object->vertexarray[i].u != object->vertexarray[j].u);
            diff_v = (object->vertexarray[i].v != object->vertexarray[j].v);

            if (same_pt && (diff_u || diff_v))
                return true;
        }
    }

    if (collapseObject)
        return true;

    return false;
}

bool isTerrainSplit(ob_t* object)
{
    /* general showstoppers */
    if (typeConvertion == _AC3DTOAC3DS)
        return false;

    if (distSplit <= 0)
        return false;

    if (object->name)
    {
        /* denied prefixes */
        const int num_prefixes = 17;
        const char* denied_prefixes[num_prefixes] =
        { "tkrb", "tkmn", "tkrs", "tklb", "brlt", "brrt", "tkls", "t0RB",
                "t1RB", "t2RB", "tkRS", "t0LB", "t1LB", "t2LB", "tkLS", "BOLt",
                "BORt" };

        for (int i = 0; i < num_prefixes; i++)
        {
            if (!strnicmp(object->name, denied_prefixes[i],
                    strlen(denied_prefixes[i])))
                return false;
        }

        /* name contains terrain or ground */
        if (strstr(object->name, "terrain") || strstr(object->name, "TERRAIN")
                || strstr(object->name, "GROUND")
                || strstr(object->name, "ground"))
            return true;
    }

    /* dimension within splitting distance */
    if (((object->x_max - object->x_min) > 1.5 * distSplit)
            || ((object->y_max - object->y_min) > 1.5 * distSplit))
        return true;

    return false;
}

ob_t * splitObjects(ob_t* object)
{
    if (NULL == object)
        return NULL;

    /* The returned object. Contains all objects from the given
     * object, split and non-split objects.
     */
    ob_t* newob = NULL;

    ob_t* current_ob = object;
    while(current_ob != NULL)
    {
        const char* objname = current_ob->name;
        if(!objname)
            objname = "";

        ob_t* next_ob = current_ob->next; // remember next one, cuz we'll modify current_ob
        ob_t* splitob = NULL;

        if (isObjectSplit(current_ob))
        {
            printf("Found in %s, a duplicate coord with not the same u,v, split is required\n",
                   objname);

            splitob = splitOb(current_ob);
        }
        else if (isTerrainSplit(current_ob))
        {
            printf("Splitting surfaces of %s\n", objname);

            splitob = terrainSplitOb(current_ob);
        }

        if(splitob != NULL)
        {
            obFree(current_ob);
            newob = obAppend(newob, splitob);
        }
        else
        {
            printf("No split required for %s\n", objname);

            // Append only the single object, not the whole list.
            // The others will be appended in this function one by one.
            current_ob->next = NULL;
            newob = obAppend(newob, current_ob);
        }

        current_ob = next_ob;
    }

    return newob;
}

int loadAC(char * inputFilename, char * outputFilename)
{
    /* saveIn : 0= 3ds , 1= obj , 2=ac3d grouped (track) , 3 = ac3d strips (cars) */
    char Line[256];
    int ret = 0;
    int (*doVerb)(char * Line, ob_t *object, mat_t * material);
    FILE * file;
    ob_t * current_ob;
    mat_t * current_material;

    if ((file = fopen(inputFilename, "r")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", inputFilename);
        return (-1);
    }
    if (fgets(Line, 256, file) == NULL)
    {
        fprintf(stderr, "failed to read first line of the file\n");
        return (-1);
    }
    if (strnicmp(Line, AC3D, strlen(AC3D)))
    {
        fprintf(stderr, "unknown format %s \n", Line);
        return (-1);
    }

    current_ob = root_ob = (ob_t*) malloc(sizeof(ob_t));
    current_material = root_material = (mat_t *) malloc(sizeof(mat_t));
    memset(current_ob, '\0', sizeof(ob_t));
    memset(current_material, '\0', sizeof(mat_t));
    root_ob->name = strdup("root");
    root_material->name = strdup("root");
    fprintf(stderr, "starting loading ...\n");

    while (fgets(Line, sizeof(Line), file))
    {
        int i = 0;
        /*fprintf(stderr,"parsing line: %s", Line);*/
        doVerb = NULL;
        while (1)
        {
            if (stricmp("END", verbTab[i].verb) == 0)
                break;
            if (strnicmp(Line, verbTab[i].verb, strlen(verbTab[i].verb)) == 0)
            {
                doVerb = verbTab[i].doVerb;
                break;
            }
            i++;
        }
        if (numvertFound == 1 && doVerb == NULL)
        {
            ret = doGetVertex(Line, current_ob, current_material);
            if(ret != 0)
                break;
        }
        else if (numrefsFound == 1 && doVerb == NULL)
        {
            ret = doGetSurf(Line, current_ob, current_material);
            if(ret != 0)
                break;
        }
        else
        {
            if (doVerb == NULL)
            {
                fprintf(stderr, " Unknown verb %s\n", Line);
                continue;
            }
            numvertFound = 0;
            numrefsFound = 0;
            ret = doVerb(Line, current_ob, current_material);
            if(ret != 0)
                break;
        }
    }
    fclose(file);
    if(ret != 0)
    {
        obFree(current_ob);
        return ret;
    }

    root_ob = current_ob;

    if(splitObjectsDuringLoad != 0)
        root_ob = splitObjects(root_ob);

    // --- perform file output ---

    if(!outputFilename)
        return 0;

    if (typeConvertion == _AC3DTOOBJ)
    {
        computeSaveOBJ(outputFilename, root_ob);
    }
    else if (typeConvertion == _AC3DTOAC3DM)
    {
        computeSaveAC3DM(outputFilename, root_ob);
    }
    else if (typeConvertion == _AC3DTOAC3DS)
    {
        computeSaveAC3DStrip(outputFilename, root_ob);
    }
    else if (typeConvertion == _AC3DTOAC3D)
    {
        computeSaveAC3D(outputFilename, root_ob);
    }
#ifdef _3DS
    else if (typeConvertion == _AC3DTO3DS)
    {
        saveObin3DS(outputFilename, root_ob);
    }
#endif

    return 0;
}

#ifdef _3DS
void saveIn3DSsubObject(ob_t * object,database3ds *db)
{

    /*material3ds *matr     = NULL;*/
    mesh3ds *mobj = NULL;
    kfmesh3ds *kobj = NULL;
    int i=0;

    if (object->name==NULL && (!stricmp("world",object->type)))
    {
        return;
    }
    else if (!stricmp("world",object->name))
    {
        return;
    }
    else if (!stricmp("root",object->name))
    {
        if (object->next!=NULL)
        {
            saveIn3DSsubObject(object->next,db);
            return;
        }
    }

    if (object->numvert!=0)
    {
        if (object->name==NULL)
        {
            sprintf(tmpname,"TMPNAME%d",tmpIndice);
            tmpIndice=0;
        }
        else
        {
            sprintf(tmpname,"%s",object->name);
        }

        printf("saving %s , numvert=%d , numsurf=%d\n",object->name,object->numvert,object->numsurf);

        InitMeshObj3ds(&mobj, object->numvert, object->numsurf,
                InitNoExtras3ds|InitTextArray3ds|InitVertexArray3ds|InitFaceArray3ds);

        for (i = 0; i < object->numvert; i++)
        {
            mobj->vertexarray[i].x = object->vertex[i].x;
            mobj->vertexarray[i].y = object->vertex[i].y;
            mobj->vertexarray[i].z = object->vertex[i].z;
        }

        for (i = 0; i < object->numvert; i++)
        {
            mobj->textarray[i].u = object->textarray[i*2];
            mobj->textarray[i].v = object->textarray[i*2+1];
        }

        for (int j=0; j<object->numsurf; j++)
        {
            /* GUIONS */
            mobj->facearray[j].v1=object->vertexarray[j*3].indice;
            mobj->facearray[j].flag=FaceABVisable3ds|FaceBCVisable3ds|FaceCAVisable3ds;
            mobj->facearray[j].v2=object->vertexarray[j*3+1].indice;
            mobj->facearray[j].v3=object->vertexarray[j*3+2].indice;

        }

        ON_ERROR_RETURN;
        sprintf(mobj->name, "%s", tmpname);
        printf("generating object %s faces: %d vertex:%d             \r",
                mobj->name,object->numsurf,object->numvert);
        mobj->ntextverts =object->numvert;
        mobj->usemapinfo = True3ds;

        if (object->texture!=NULL)
        {
            mobj->nmats = 1;
            InitMeshObjField3ds (mobj, InitMatArray3ds);
            ON_ERROR_RETURN;

            InitMatArrayIndex3ds (mobj, 0, mobj->nfaces);
            ON_ERROR_RETURN;

            for (i=0; i<texnum; i++)
            {
                if (tex[i]!=NULL)
                if (object->texture!=NULL)
                if (!strncmp(object->texture,tex[i],13))
                sprintf(mobj->matarray[0].name,"texture%d",i);
            }

            for (i=0; i<mobj->nfaces; i++)
            mobj->matarray[0].faceindex[i] = (ushort3ds)i;
            mobj->matarray[0].nfaces = mobj->nfaces;
        }

        FillMatrix3ds(mobj);
        PutMesh3ds(db, mobj);
        ON_ERROR_RETURN;
        InitObjectMotion3ds(&kobj, 1, 1, 1, 0, 0);
        ON_ERROR_RETURN;
        sprintf(kobj->name, "%s",tmpname);
        /* Set the pivot point to the mesh's center */
        SetPoint(kobj->pivot, 0.0F, 0.0F, 0.0F);
        SetBoundBox3ds (mobj, kobj);
        ON_ERROR_RETURN;
        kobj->pkeys[0].time = 0;
        kobj->pos[0].x = (kobj->boundmax.x - kobj->boundmin.x) / 2.0F + kobj->boundmin.x;
        kobj->pos[0].y = (kobj->boundmax.y - kobj->boundmin.y) / 2.0F + kobj->boundmin.y;
        kobj->pos[0].z = (kobj->boundmax.z - kobj->boundmin.z) / 2.0F + kobj->boundmin.z;
        kobj->rkeys[0].time = 0;
        kobj->rot[0].angle = 0.0F;
        kobj->rot[0].x = 0.0F;
        kobj->rot[0].y = 0.0F;
        kobj->rot[0].z = -1.0F;
        PutObjectMotion3ds(db, kobj);
        ON_ERROR_RETURN;
        ReleaseObjectMotion3ds (&kobj);
        ON_ERROR_RETURN;
        RelMeshObj3ds(&mobj);
    }

    if (object->next)
    saveIn3DSsubObject(object->next,db);

}

void saveObin3DS( char * OutputFilename, ob_t * object)
{
    database3ds *db = NULL;
    file3ds *file;
    material3ds *matr = NULL;

    meshset3ds *mesh = NULL;
    background3ds *bgnd = NULL;
    atmosphere3ds *atmo = NULL;
    int i =0;
    int j=0;
    char name2[256];
    char *p, *q;
    viewport3ds *view = NULL;
    ob_t * tmpob =NULL;

    /* Clear error list, not necessary but safe */
    ClearErrList3ds();
    file = OpenFile3ds(OutputFilename, "w");
    PRINT_ERRORS_EXIT(stderr);
    InitDatabase3ds(&db);
    PRINT_ERRORS_EXIT(stderr);
    CreateNewDatabase3ds(db, MeshFile);
    PRINT_ERRORS_EXIT(stderr);
    /*--- MESH SETTINGS */
    InitMeshSet3ds(&mesh);
    ON_ERROR_RETURN;
    mesh->ambientlight.r = mesh->ambientlight.g = mesh->ambientlight.b = 0.3F;
    PutMeshSet3ds(db, mesh);
    ON_ERROR_RETURN;
    ReleaseMeshSet3ds(&mesh);
    ON_ERROR_RETURN;
    InitBackground3ds(&bgnd);
    ON_ERROR_RETURN;
    PutBackground3ds(db, bgnd);
    ON_ERROR_RETURN;
    ReleaseBackground3ds(&bgnd);
    ON_ERROR_RETURN;
    /*--- ATMOSPHERE */
    InitAtmosphere3ds(&atmo);
    ON_ERROR_RETURN;
    PutAtmosphere3ds(db, atmo);
    ON_ERROR_RETURN;
    ReleaseAtmosphere3ds(&atmo);
    ON_ERROR_RETURN;
    /*--- MATERIAL */
    InitMaterial3ds(&matr);
    ON_ERROR_RETURN;
    strcpy(matr->name, "RedSides");
    matr->ambient.r = 0.0F;
    matr->ambient.g = 0.0F;
    matr->ambient.b = 0.0F;
    matr->diffuse.r = 1.0F;
    matr->diffuse.g = 0.0F;
    matr->diffuse.b = 0.0F;
    matr->specular.r = 1.0F;
    matr->specular.g = 0.7F;
    matr->specular.b = 0.65F;
    matr->shininess = 0.0F;
    matr->shinstrength = 0.0F;
    matr->blur = 0.2F;
    matr->transparency = 0.0F;
    matr->transfalloff = 0.0F;
    matr->selfillumpct = 0.0F;
    matr->wiresize = 1.0F;
    matr->shading = Phong;
    matr->useblur = True3ds;
    PutMaterial3ds(db, matr);
    ON_ERROR_RETURN;
    ReleaseMaterial3ds(&matr);
    ON_ERROR_RETURN;

    texnum=0;
    tmpob=object;
    while (tmpob!=NULL)
    {
        int texnofound=0;
        if (tmpob->name==NULL)
        {
            tmpob=tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob=tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob=tmpob->next;
            continue;
        }
        texnofound=1;
        for (i=0; i<texnum; i++)
        {
            if (tmpob->texture==NULL)
            {
                texnofound=0;
                break;

            }
            if (!strncmp(tex[i],tmpob->texture,13))
            {
                texnofound=0;
                break;

            }
            else
            {
                texnofound=1;
            }
        }
        if (texnofound==1)
        {
            if (tmpob->texture!=NULL)
            {
                strcpy(tex[texnum],tmpob->texture);
                tex[texnum][13]='\0';
            }
            texnum ++;
        }
        tmpob=tmpob->next;
    }

    for (i=0; i<texnum; i++)
    {

        InitMaterial3ds(&matr);
        ON_ERROR_RETURN;
        sprintf(matr->name,"texture%d",i);
        /*sprintf(matr->texture.map.name,"texture%d",i);*/
        printf("analysing  %s \n",tex[i]);
        p=tex[i];
        q=name2;
        while (*p)
        {
            if ( (*p<='Z'&&*p>='A'))
            {
                *p=(*p-'A')+'a';
            }

            if ( (*p>='a' && *p<='z') || (*p>='0' && *p<='9') || (*p=='.'))
            {
                *q=*p; q++; *q='\0';
            }
            p++;
        }
        j=0;
        while (name2[j]!='\0')
        {
            if (name2[j]=='.')
            {
                name2[j]='\0';
                break;
            }
            j++;
            if (j==8)
            {
                name2[j]='\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png \n",tex[i],name2);
        sprintf(matr->texture.map.name,"%s.png",name2);

        if (root_material!=NULL)
        {
            matr->ambient.r = root_material->amb.r;
            matr->ambient.g = root_material->amb.g;
            matr->ambient.b = root_material->amb.b;
            matr->specular.r = root_material->spec.r;
            matr->specular.g = root_material->spec.g;
            matr->specular.b = root_material->spec.b;
            matr->shininess = root_material->shi;
            matr->transparency = root_material->trans;
            matr->diffuse.r = root_material->rgb.r;
            matr->diffuse.g = root_material->rgb.g;
            matr->diffuse.b = root_material->rgb.b;
        }
        else
        {
            matr->ambient.r = 1.0F;
            matr->ambient.g = 1.0F;
            matr->ambient.b = 1.0F;
            matr->specular.r =1.0F;
            matr->specular.g =1.0F;
            matr->specular.b =1.0F;
            matr->shininess = 0.0F;
            matr->transparency = 0.0F;
            matr->diffuse.r = 1.0F;
            matr->diffuse.g = 1.0F;
            matr->diffuse.b = 1.0F;
        }

        matr->shinstrength = 0.0F;
        matr->blur = 0.2F;

        matr->transfalloff = 0.0F;
        matr->selfillumpct = 0.0F;
        matr->wiresize = 1.0F;
        matr->shading = Phong;
        matr->useblur = True3ds;
        matr->texture.map.percent = 1.0F;
        matr->texture.map.ignorealpha = False3ds;
        matr->texture.map.filter = (filtertype3ds)False3ds;
        matr->texture.map.mirror = False3ds;
        matr->texture.map.negative =False3ds;
        matr->texture.map.source = RGB;
        matr->texture.map.blur = 0.07F;
        PutMaterial3ds(db, matr);
        ON_ERROR_RETURN;
        ReleaseMaterial3ds(&matr);
        ON_ERROR_RETURN;
    }

    tmpIndice=0;
    /* do the job */
    saveIn3DSsubObject(object,db);
    printf("\nend\n");
    /* Always remember to release the memory used by a mesh3ds pointer prior
     to using the pointer again so that the values of the last mesh will
     not be carried over. */
    InitViewport3ds(&view);
    ON_ERROR_RETURN;
    PutViewport3ds(db, view);
    ON_ERROR_RETURN;
    ReleaseViewport3ds(&view);
    ON_ERROR_RETURN;
    WriteDatabase3ds(file, db);
    PRINT_ERRORS_EXIT(stderr);
    CloseAllFiles3ds();
    PRINT_ERRORS_EXIT(stderr);
    ReleaseDatabase3ds(&db);
    PRINT_ERRORS_EXIT(stderr);

}
#endif

int printOb(ob_t * ob)
{
    int i = 0;
    int multitex = 0;

    if (ob->numsurf == 0)
        return 0;
    if (extendedStrips == 0 && normalMapping != 1)
        if (!(isobjectacar && collapseObject))
            stripifyOb(ob, 0);
    ob->saved = 1;
    fprintf(ofile, "OBJECT poly\n");
    fprintf(ofile, "name \"%s\"\n", ob->name);
    if (ob->texture1 || ob->texture2 || ob->texture3)
    {
        multitex = 1;
        fprintf(ofile, "texture \"%s\" base\n", ob->texture);
        if (ob->texture1)
            fprintf(ofile, "texture \"%s\" tiled\n", ob->texture1);
        else
            fprintf(ofile, "texture empty_texture_no_mapping tiled\n");
        if (ob->texture2)
            fprintf(ofile, "texture \"%s\" skids\n", ob->texture2);
        else
            fprintf(ofile, "texture empty_texture_no_mapping skids\n");
        if (ob->texture3)
            fprintf(ofile, "texture \"%s\" shad\n", ob->texture3);
        else
            fprintf(ofile, "texture empty_texture_no_mapping shad\n");
    }
    else
    {
        fprintf(ofile, "texture \"%s\"\n", ob->texture);
    }
    fprintf(ofile, "numvert %d\n", ob->numvert);
    for (i = 0; i < ob->numvert; i++)
    {
        if ((typeConvertion == _AC3DTOAC3DS
                && (extendedStrips == 1 || extendedTriangles == 1))
                || typeConvertion == _AC3DTOAC3DGROUP
                || (typeConvertion == _AC3DTOAC3D && (extendedTriangles == 1)))
        {
            fprintf(ofile, "%lf %lf %lf %lf %lf %lf\n", ob->vertex[i].x,
                    ob->vertex[i].z, -ob->vertex[i].y, ob->snorm[i].x,
                    ob->snorm[i].z, -ob->snorm[i].y);
        }
        else
        {
            fprintf(ofile, "%lf %lf %lf\n", ob->vertex[i].x, ob->vertex[i].z,
                    -ob->vertex[i].y);
        }
    }
    if (extendedStrips == 0)
    {
        fprintf(ofile, "numsurf %d\n", ob->numsurf);
        for (i = 0; i < ob->numsurf; i++)
        {
            if (ob->attrSurf != 0)
                fprintf(ofile, "SURF 0x%2x\n", ob->attrSurf);
            else
                fprintf(ofile, "SURF 0x20\n");
            fprintf(ofile, "mat %d\n", ob->attrMat);
            fprintf(ofile, "refs 3\n");
            /* GUIONS */
            if (multitex == 0)
            {
                fprintf(ofile, "%d %.5f %.5f\n", ob->vertexarray[i * 3].indice,
                        ob->textarray[ob->vertexarray[i * 3].indice * 2],
                        ob->textarray[ob->vertexarray[i * 3].indice * 2 + 1]);
                fprintf(ofile, "%d %.5f %.5f\n",
                        ob->vertexarray[i * 3 + 1].indice,
                        ob->textarray[ob->vertexarray[i * 3 + 1].indice * 2],
                        ob->textarray[ob->vertexarray[i * 3 + 1].indice * 2 + 1]);
                fprintf(ofile, "%d %.5f %.5f\n",
                        ob->vertexarray[i * 3 + 2].indice,
                        ob->textarray[ob->vertexarray[i * 3 + 2].indice * 2],
                        ob->textarray[ob->vertexarray[i * 3 + 2].indice * 2 + 1]);

            }
            else
            {
                fprintf(ofile, "%d %.5f %.5f ", ob->vertexarray[i * 3].indice,
                        ob->textarray[ob->vertexarray[i * 3].indice * 2],
                        ob->textarray[ob->vertexarray[i * 3].indice * 2 + 1]);

                if (ob->texture1)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray1[ob->vertexarray[i * 3].indice * 2],
                            ob->textarray1[ob->vertexarray[i * 3].indice * 2 + 1]);

                if (ob->texture2)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray2[ob->vertexarray[i * 3].indice * 2],
                            ob->textarray2[ob->vertexarray[i * 3].indice * 2 + 1]);
                if (ob->texture3)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray3[ob->vertexarray[i * 3].indice * 2],
                            ob->textarray3[ob->vertexarray[i * 3].indice * 2 + 1]);
                fprintf(ofile, "\n");

                fprintf(ofile, "%d %.5f %.5f ",
                        ob->vertexarray[i * 3 + 1].indice,
                        ob->textarray[ob->vertexarray[i * 3 + 1].indice * 2],
                        ob->textarray[ob->vertexarray[i * 3 + 1].indice * 2 + 1]);
                if (ob->texture1)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray1[ob->vertexarray[i * 3 + 1].indice * 2],
                            ob->textarray1[ob->vertexarray[i * 3 + 1].indice * 2
                                    + 1]);

                if (ob->texture2)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray2[ob->vertexarray[i * 3 + 1].indice * 2],
                            ob->textarray2[ob->vertexarray[i * 3 + 1].indice * 2
                                    + 1]);
                if (ob->texture3)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray3[ob->vertexarray[i * 3 + 1].indice * 2],
                            ob->textarray3[ob->vertexarray[i * 3 + 1].indice * 2
                                    + 1]);
                fprintf(ofile, "\n");

                fprintf(ofile, "%d %.5f %.5f ",
                        ob->vertexarray[i * 3 + 2].indice,
                        ob->textarray[ob->vertexarray[i * 3 + 2].indice * 2],
                        ob->textarray[ob->vertexarray[i * 3 + 2].indice * 2 + 1]);
                if (ob->texture1)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray1[ob->vertexarray[i * 3 + 2].indice * 2],
                            ob->textarray1[ob->vertexarray[i * 3 + 2].indice * 2
                                    + 1]);

                if (ob->texture2)
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray2[ob->vertexarray[i * 3 + 2].indice * 2],
                            ob->textarray2[ob->vertexarray[i * 3 + 2].indice * 2
                                    + 1]);
                if (ob->texture3)
                {
                    fprintf(ofile, "%.5f %.5f ",
                            ob->textarray3[ob->vertexarray[i * 3 + 2].indice * 2],
                            ob->textarray3[ob->vertexarray[i * 3 + 2].indice * 2
                                    + 1]);
                    if (ob->textarray3[ob->vertexarray[i * 3 + 2].indice * 2]
                            != ob->textarray1[ob->vertexarray[i * 3 + 2].indice
                                    * 2])
                    {
                        printf("error in text\n");
                    }
                }
                fprintf(ofile, "\n");

            }
        }
    }
    else
    {
        stripifyOb(ob, 1);
    }
    fprintf(ofile, "kids 0\n");
    return 0;
}
int foundNear(ob_t * object, ob_t *allobjects, int dist, int print)
{
    ob_t * tmpob;
    double x;
    double y;
    int numfound = 0;
    tmpob = allobjects;

    x = (object->x_max - object->x_min) / 2 + object->x_min;
    y = (object->y_max - object->y_min) / 2 + object->y_min;

    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strnicmp(tmpob->name, "tkmn", 4))
        {
            tmpob = tmpob->next;
            continue;
        }

        if (tmpob->inkids_o == 1)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (tmpob->numsurf == 0)
        {
            tmpob = tmpob->next;
            continue;
        }
        if ((tmpob->x_min - x) * (tmpob->x_min - x)
                + (tmpob->y_min - y) * (tmpob->y_min - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d) \n", tmpob->name , object->name, dist);*/
            numfound++;
            tmpob->inkids_o = 1;
            if (print)
            {
                printOb(tmpob);
            }
            tmpob = tmpob->next;
            continue;
        }
        if ((tmpob->x_max - x) * (tmpob->x_max - x)
                + (tmpob->y_max - y) * (tmpob->y_max - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d) \n", tmpob->name , object->name, dist);*/
            numfound++;
            tmpob->inkids_o = 1;
            if (print)
            {
                printOb(tmpob);
            }
            tmpob = tmpob->next;
            continue;
        }
        if ((tmpob->x_min - x) * (tmpob->x_min - x)
                + (tmpob->y_max - y) * (tmpob->y_max - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d) \n", tmpob->name , object->name, dist);*/
            numfound++;
            tmpob->inkids_o = 1;
            if (print)
            {
                printOb(tmpob);
            }
            tmpob = tmpob->next;
            continue;
        }
        if ((tmpob->x_max - x) * (tmpob->x_max - x)
                + (tmpob->y_min - y) * (tmpob->y_min - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d) \n", tmpob->name , object->name, dist);*/
            numfound++;
            tmpob->inkids_o = 1;
            if (print)
            {
                printOb(tmpob);
            }
            tmpob = tmpob->next;
            continue;
        }
        tmpob = tmpob->next;
    }

    object->kids_o = numfound++;

    /*printf(" object %s (dist=%d) found %d objects \n", object->name, dist, numfound);*/
    return (0);

}

void normalize(point_t *t)
{
    double dd;
    dd = sqrt(t->x * t->x + t->y * t->y + t->z * t->z);
    if (dd != 0.0)
    {
        t->x = t->x / dd;
        t->y = t->y / dd;
        t->z = t->z / dd;
    }
    else
    {
        t->x = 0.0;
        t->y = 1.0;
        t->z = 0.0;
    }
}

void computeTriNorm(ob_t * object)
{
    ob_t * tmpob = NULL;
    int i = 0;
    point_t norm;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numsurf; i++)
        {
            /* compute the same normal for each points in the surface */
            computeNorm(&tmpob->vertex[tmpob->vertexarray[i * 3].indice],
                    &tmpob->vertex[tmpob->vertexarray[i * 3 + 1].indice],
                    &tmpob->vertex[tmpob->vertexarray[i * 3 + 2].indice],
                    &norm);
            tmpob->norm[tmpob->vertexarray[i * 3].indice].x += norm.x;
            tmpob->norm[tmpob->vertexarray[i * 3].indice].y += norm.y;
            tmpob->norm[tmpob->vertexarray[i * 3].indice].z += norm.z;

            tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice].x += norm.x;
            tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice].y += norm.y;
            tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice].z += norm.z;

            tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice].x += norm.x;
            tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice].y += norm.y;
            tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice].z += norm.z;
        }
        for (i = 0; i < tmpob->numsurf; i++)
        {
            normalize(&tmpob->norm[tmpob->vertexarray[i * 3].indice]);
            normalize(&tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice]);
            normalize(&tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice]);
        }

        tmpob = tmpob->next;
    }
    return;
}

void computeObjectTriNorm(ob_t * object)
{
    ob_t * tmpob = NULL;
    int i = 0;
    point_t norm;

    tmpob = object;
    if (tmpob->name == NULL)
    {
        tmpob = tmpob->next;
        return;
    }
    if (!strcmp(tmpob->name, "root"))
    {
        tmpob = tmpob->next;
        return;
    }
    if (!strcmp(tmpob->name, "world"))
    {
        tmpob = tmpob->next;
        return;
    }
    for (i = 0; i < tmpob->numsurf; i++)
    {
        /* compute the same normal for each points in the surface */
        computeNorm(&tmpob->vertex[tmpob->vertexarray[i * 3].indice],
                &tmpob->vertex[tmpob->vertexarray[i * 3 + 1].indice],
                &tmpob->vertex[tmpob->vertexarray[i * 3 + 2].indice], &norm);

        tmpob->norm[tmpob->vertexarray[i * 3].indice].x += norm.x;
        tmpob->norm[tmpob->vertexarray[i * 3].indice].y += norm.y;
        tmpob->norm[tmpob->vertexarray[i * 3].indice].z += norm.z;

        tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice].x += norm.x;
        tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice].y += norm.y;
        tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice].z += norm.z;

        tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice].x += norm.x;
        tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice].y += norm.y;
        tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice].z += norm.z;
    }
    for (i = 0; i < tmpob->numsurf; i++)
    {
        normalize(&tmpob->norm[tmpob->vertexarray[i * 3].indice]);
        normalize(&tmpob->norm[tmpob->vertexarray[i * 3 + 1].indice]);
        normalize(&tmpob->norm[tmpob->vertexarray[i * 3 + 2].indice]);
    }

    return;
}

int checkMustSmoothVector(point_t *n1, point_t *n2, point_t *t1, point_t *t2)
{
    return FALSE;
/*
    double dot, cos_angle;
    cos_angle = cos(smooth_angle * M_PI / 180.0);
    if (fabs(t1->x - t2->x) <= 0.05 && fabs(t1->y - t2->y) <= 0.05
            && fabs(t1->z - t2->z) <= 0.05)
    {
        // GUIONS
        dot = n1->x * n2->x + n1->y * n2->y + n1->z * n2->z;
        if (dot > cos_angle)
        {
            return TRUE;
        }

    }
    return FALSE;
*/
}

void smoothTriNorm(ob_t * object)
{
    ob_t * tmpob = NULL;
    ob_t * tmpob1 = NULL;
    int i = 0;
    double dd;
    double nx, ny, nz;
    int j = 0;

    printf("Smooth called on %s\n", object->name);
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            tmpob->snorm[i].x = tmpob->norm[i].x;
            tmpob->snorm[i].y = tmpob->norm[i].y;
            tmpob->snorm[i].z = tmpob->norm[i].z;
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    tmpob1 = object;

    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (tmpob->data)
            if (!strstr(tmpob->data, "nosmooth"))
            {
                tmpob = tmpob->next;
                continue;
            }
        tmpob1 = object;
        while (tmpob1 != NULL)
        {
            if (tmpob1->name == NULL)
            {
                tmpob1 = tmpob1->next;
                continue;
            }
            if (!strcmp(tmpob1->name, "root"))
            {
                tmpob1 = tmpob1->next;
                continue;
            }
            if (!strcmp(tmpob1->name, "world"))
            {
                tmpob1 = tmpob1->next;
                continue;
            }
            if (tmpob1->data)
                if (!strstr(tmpob1->data, "nosmooth"))
                {
                    tmpob1 = tmpob1->next;
                    continue;
                }

            for (i = 0; i < tmpob->numvert; i++)
            {
                for (j = 0; j < tmpob1->numvert; j++)
                {
                    if (checkMustSmoothVector(&tmpob->norm[i], &tmpob1->norm[j],
                            &tmpob->vertex[i], &tmpob1->vertex[j]))
                    {

                        point_t p;
                        p.x = tmpob1->norm[j].x + tmpob->norm[i].x;
                        p.y = tmpob1->norm[j].y + tmpob->norm[i].y;
                        p.z = tmpob1->norm[j].z + tmpob->norm[i].z;
                        normalize(&p);

                        tmpob->snorm[i].x = p.x;
                        tmpob->snorm[i].y = p.y;
                        tmpob->snorm[i].z = p.z;

                        tmpob1->snorm[j].x = p.x;
                        tmpob1->snorm[j].y = p.y;
                        tmpob1->snorm[j].z = p.z;

                        /*          tmpob->snorm[i].x+=tmpob1->norm[j].x; */
                        /*          tmpob->snorm[i].y+=tmpob1->norm[j].y; */
                        /*          tmpob->snorm[i].z+=tmpob1->norm[j].z; */

                        /*          tmpob1->snorm[j].x+=tmpob->norm[i].x; */
                        /*          tmpob1->snorm[j].y+=tmpob->norm[i].y; */
                        /*          tmpob1->snorm[j].z+=tmpob->norm[i].z; */
                    }
                }
            }

            tmpob1 = tmpob1->next;
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            nx = tmpob->snorm[i].x;
            ny = tmpob->snorm[i].y;
            nz = tmpob->snorm[i].z;
            dd = sqrt(nx * nx + ny * ny + nz * nz);
            if (dd != 0.0)
            {
                tmpob->snorm[i].x = nx / dd;
                tmpob->snorm[i].y = ny / dd;
                tmpob->snorm[i].z = nz / dd;
            }
            else
            {
                tmpob->snorm[i].x = 0;
                tmpob->snorm[i].y = 0;
                tmpob->snorm[i].z = 1;
            }
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (tmpob->data)
            if (!strstr(tmpob->data, "nosmooth"))
            {
                tmpob = tmpob->next;
                continue;
            }
        tmpob1 = object;
        while (tmpob1 != NULL)
        {
            if (tmpob1->name == NULL)
            {
                tmpob1 = tmpob1->next;
                continue;
            }
            if (!strcmp(tmpob1->name, "root"))
            {
                tmpob1 = tmpob1->next;
                continue;
            }
            if (!strcmp(tmpob1->name, "world"))
            {
                tmpob1 = tmpob1->next;
                continue;
            }
            if (tmpob1->data)
                if (!strstr(tmpob1->data, "nosmooth"))
                {
                    tmpob1 = tmpob1->next;
                    continue;
                }

            tmpob1 = tmpob1->next;
        }
        tmpob = tmpob->next;
    }

    return;
}

void smoothFaceTriNorm(ob_t * object)
{
    ob_t * tmpob = NULL;
    int i = 0;
    int j = 0;

    tmpob = object;
    if (tmpob->name == NULL)
    {
        tmpob = tmpob->next;
        return;
    }
    if (!strcmp(tmpob->name, "root"))
    {
        tmpob = tmpob->next;
        return;
    }
    if (!strcmp(tmpob->name, "world"))
    {
        tmpob = tmpob->next;
        return;
    }

    for (i = 0; i < tmpob->numvert; i++)
    {
        /* compute the same normal for each points in the surface */
        for (j = 0; j < tmpob->numvert; j++)
        {
            if ((tmpob->vertex[i].x - tmpob->vertex[j].x) <= 0.01
                    && (tmpob->vertex[i].y - tmpob->vertex[j].y) <= 0.01
                    && (tmpob->vertex[i].z - tmpob->vertex[j].z) <= 0.1)
            {
                /*same point */
                tmpob->snorm[i].x += tmpob->norm[j].x;
                tmpob->snorm[i].y += tmpob->norm[j].y;
                tmpob->snorm[i].z += tmpob->norm[j].z;
                tmpob->snorm[j].x = tmpob->snorm[i].x;
                tmpob->snorm[j].y = tmpob->snorm[i].y;
                tmpob->snorm[j].z = tmpob->snorm[i].z;
            }
        }
    }

    for (i = 0; i < tmpob->numvert; i++)
    {
        normalize(&tmpob->snorm[i]);
    }
    return;
}

void smoothObjectTriNorm(ob_t * object)
{
    ob_t * tmpob = NULL;
    int i = 0;
    int j = 0;

    tmpob = object;

    for (i = 0; i < tmpob->numvert; i++)
    {
        /* compute the same normal for each points in the surface */
        for (j = 0; j < tmpob->numvert; j++)
        {
            if ((tmpob->vertex[i].x - tmpob->vertex[j].x) <= 0.001
                    && (tmpob->vertex[i].y - tmpob->vertex[j].y) <= 0.001
                    && (tmpob->vertex[i].z - tmpob->vertex[j].z) <= 0.001)
            {
                /*same point */
                tmpob->snorm[i].x += tmpob->norm[j].x;
                tmpob->snorm[i].y += tmpob->norm[j].y;
                tmpob->snorm[i].z += tmpob->norm[j].z;
                tmpob->snorm[j].x = tmpob->snorm[i].x;
                tmpob->snorm[j].y = tmpob->snorm[i].y;
                tmpob->snorm[j].z = tmpob->snorm[i].z;
            }
        }
    }
    for (i = 0; i < tmpob->numvert; i++)
    {
        normalize(&tmpob->snorm[i]);
    }
    return;
}

void computeSaveAC3D(char * OutputFilename, ob_t * object)
{

    int i = 0;
    int j = 0;
    char name2[256];
    char *p, *q;
    ob_t * tmpob = NULL;
    mat_t * tmat = NULL;
    int numg = 0;
    int lastpass = FALSE;
    int nborder = 0;
    int ordering = FALSE;
    int ik;

    if (normalMapping == 1)
    {
        normalMap(object);
    }

    if ((ofile = fopen(OutputFilename, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename);
        return;
    }
    if (extendedTriangles)
    {
        smoothTriNorm(object);
        if (isobjectacar)
            mapNormalToSphere2(object);
        if (isobjectacar && extendedEnvCoord)
            mapTextureEnv(object);
        if (collapseObject)
            mergeSplitted(&object);
    }

    fprintf(ofile, "AC3Db\n");
    tmat = root_material;
    while (tmat != NULL)
    {
        if (strcmp(tmat->name, "root") == 0)
        {
            tmat = tmat->next;
            continue;
        }
        fprintf(ofile,
                "MATERIAL %s rgb %1.2f %1.2f %1.2f amb %1.2f %1.2f %1.2f emis %1.2f %1.2f %1.2f spec %1.2f %1.2f %1.2f shi %d trans 0 \n",
                tmat->name, tmat->rgb.r, tmat->rgb.g, tmat->rgb.b, tmat->amb.r,
                tmat->amb.g, tmat->amb.b, tmat->emis.r, tmat->emis.g,
                tmat->emis.b, tmat->spec.r, tmat->spec.g, tmat->spec.b,
                (int) tmat->shi);
        tmat = tmat->next;
    }

    fprintf(ofile, "OBJECT world\n");

    if (OrderString && isobjectacar)
    {
        fprintf(stderr, "ordering objects according to  %s\n", OrderString);
        p = OrderString;
        ordering = TRUE;
        nborder = 1;
        while (TRUE)
        {
            q = strstr(p, ";");
            if (q != NULL)
                nborder++;
            else
                break;
            p = q + 1;
            if (*p == '\0')
            {
                nborder--;
                break;
            }
        }
    }
    else
    {
        ordering = FALSE;
        nborder = 0;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (isobjectacar == 0)
        {
            if (strnicmp(tmpob->name, "tkmn", 4) == 0)
            {
                tmpob = tmpob->next;
                numg++;
                continue;
            }
        }
        else
        {
            if (tmpob->type)
                if (!strcmp(tmpob->type, "group"))
                {
                    tmpob = tmpob->next;
                    continue;
                }
            if (!strcmp(tmpob->name, "root"))
            {
                tmpob = tmpob->next;
                continue;
            }
            numg++;
        }
        tmpob = tmpob->next;
    }

    fprintf(ofile, "kids %d\n", numg);

    texnum = 0;
    tmpob = object;
    while (tmpob != NULL)
    {
        int texnofound = 0;
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        texnofound = 1;
        for (i = 0; i < texnum; i++)
        {
            if (tmpob->texture == NULL)
            {
                texnofound = 0;
                break;

            }
            if (!strncmp(tex[i], tmpob->texture, 13))
            {
                texnofound = 0;
                break;

            }
            else
                texnofound = 1;
        }
        if (texnofound == 1)
        {
            if (tmpob->texture != NULL)
            {
                strcpy(tex[texnum], tmpob->texture);
                tex[texnum][13] = '\0';
                /*sprintf(tex[texnum],"%s",tmpob->texture);*/
            }
            texnum++;
        }
        tmpob->saved = 0;
        printf("name=%s x_min=%.1f y_min=%.1f x_max=%.1f y_max=%.1f\n",
                tmpob->name, tmpob->x_min, tmpob->y_min, tmpob->x_max,
                tmpob->y_max);

        tmpob = tmpob->next;
    }

    tmpob = object;
    tmpob->kids_o = 0;
    while (tmpob != NULL)
    {
        tmpob->kids_o = 0;
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strnicmp(tmpob->name, "tkmn", 4))
        {
            foundNear(tmpob, object, far_dist, FALSE);
            printf("object =%s num kids_o=%d\n", tmpob->name, tmpob->kids_o);
        }

        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        tmpob->inkids_o = 0;
        tmpob = tmpob->next;
    }

    p = OrderString;
    q = OrderString;
    nborder++;
    for (ik = 0; ik < nborder; ik++)
    {
        if (ordering)
        {
            /* look to the current object name to save */
            if (p == NULL)
                lastpass = TRUE;
            else
            {
                q = p;
                p = strstr(p, ";");
                if (p != NULL)
                {
                    *p = '\0';
                    p++;
                }
            }
        }
        tmpob = object;
        while (tmpob != NULL)
        {
            if (tmpob->name == NULL)
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!strcmp(tmpob->name, "root"))
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!strcmp(tmpob->name, "world"))
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!isobjectacar)
            {
                if (!strnicmp(tmpob->name, "tkmn", 4))
                {
                    fprintf(ofile, "OBJECT group\n");
                    fprintf(ofile, "name \"%s_g\"\n", tmpob->name);
                    fprintf(ofile, "kids %d\n", tmpob->kids_o + 1);
                    printOb(tmpob);
                    foundNear(tmpob, object, far_dist, TRUE);
                    printf("object =%s num kids_o=%d\n", tmpob->name,
                            tmpob->kids_o);
                }
            }
            else
            {
                if (tmpob->saved == 0)
                {
                    if (ordering && !lastpass)
                    {
                        if (!strcmp(tmpob->name, q))
                        {
                            printOb(tmpob);
                            printf("object =%s num kids_o=%d test with %s\n",
                                    tmpob->name, tmpob->kids_o, q);
                        }
                        else
                        {
                            char nameBuf[1024];
                            sprintf(nameBuf, "%ss", q);
                            if (!strncmp(tmpob->name, nameBuf, strlen(nameBuf)))
                            {
                                printOb(tmpob);
                                printf("object =%s num kids_o=%d\n",
                                        tmpob->name, tmpob->kids_o);
                            }

                        }
                    }
                    else
                    {
                        printOb(tmpob);
                        printf("object =%s num kids_o=%d\n", tmpob->name,
                                tmpob->kids_o);
                    }
                }

            }

            tmpob = tmpob->next;
        }
    }

    for (i = 0; i < texnum; i++)
    {
        printf("analysing  %s \n", tex[i]);
        p = tex[i];
        q = name2;
        while (*p)
        {
            if ((*p <= 'Z' && *p >= 'A'))
            {
                *p = (*p - 'A') + 'a';
            }

            if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                    || (*p == '.'))
            {
                *q = *p;
                q++;
                *q = '\0';
            }
            p++;
        }
        j = 0;
        while (name2[j] != '\0')
        {
            if (name2[j] == '.')
            {
                name2[j] = '\0';
                break;
            }
            j++;
            if (j == 8)
            {
                name2[j] = '\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png \n", tex[i], name2);
    }

    tmpIndice = 0;
    /* do the job */
    printf("\nend\n");

}

void computeSaveOBJ(char * OutputFilename, ob_t * object)
{

    int i = 0;
    int j = 0;
    char name2[256];
    char *p, *q;
    ob_t * tmpob = NULL;
    mat_t * tmat = NULL;
    int deltav = 1;
    int ind = 0;
    char tname[256];
    FILE * tfile;

    if ((ofile = fopen(OutputFilename, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename);
        return;
    }

    fprintf(ofile, "mtllib ./%s.mtl\n", OutputFilename);
    sprintf(tname, "%s.mtl", OutputFilename);

    if ((tfile = fopen(tname, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", tname);
        return;
    }

    tmat = root_material;
    while (tmat != NULL)
    {
        if (strcmp(tmat->name, "root") != 0)
        {
        }
        else
        {
            tmpob = object;
            while (tmpob != NULL)
            {
                if (tmpob->name == NULL)
                {
                    tmpob = tmpob->next;
                    continue;
                }
                if (!strcmp(tmpob->name, "root"))
                {
                    tmpob = tmpob->next;
                    continue;
                }
                if (!strcmp(tmpob->name, "world"))
                {
                    tmpob = tmpob->next;
                    continue;
                }

                if (tmpob->texture != NULL)
                    if (*tmpob->texture != '\0')
                    {
                        fprintf(tfile, "newmtl default\n");
                        fprintf(tfile, "Ka %lf %lf %lf\n", tmat->amb.r,
                                tmat->amb.g, tmat->amb.b);
                        fprintf(tfile, "Kd %lf %lf %lf\n", tmat->emis.r,
                                tmat->emis.g, tmat->emis.b);
                        fprintf(tfile, "Ks %lf %lf %lf\n", tmat->spec.r,
                                tmat->spec.g, tmat->spec.b);
                        fprintf(tfile, "Ns %d\n", tmat->shi);
                        fprintf(tfile, "map_kd %s\n", tmpob->texture);
                        break;
                    }

                tmpob = tmpob->next;
            }

        }
        tmat = tmat->next;
    }

    texnum = 0;
    tmpob = object;
    while (tmpob != NULL)
    {
        int texnofound = 0;
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        texnofound = 1;
        for (i = 0; i < texnum; i++)
        {
            if (tmpob->texture == NULL)
            {
                texnofound = 0;
                break;

            }
            if (!strncmp(tex[i], tmpob->texture, 13))
            {
                texnofound = 0;
                break;

            }
            else
                texnofound = 1;
        }
        if (texnofound == 1)
        {
            if (tmpob->texture != NULL)
            {
                strcpy(tex[texnum], tmpob->texture);
                tex[texnum][13] = '\0';
            }
            texnum++;
        }
        printf("name=%s x_min=%.1f y_min=%.1f x_max=%.1f y_max=%.1f\n",
                tmpob->name, tmpob->x_min, tmpob->y_min, tmpob->x_max,
                tmpob->y_max);

        tmpob = tmpob->next;
    }

    for (i = 0; i < texnum; i++)
    {

        printf("analysing  %s \n", tex[i]);
        p = tex[i];
        q = name2;
        while (*p)
        {
            if ((*p <= 'Z' && *p >= 'A'))
            {
                *p = (*p - 'A') + 'a';
            }

            if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                    || (*p == '.'))
            {
                *q = *p;
                q++;
                *q = '\0';
            }
            p++;
        }
        j = 0;
        while (name2[j] != '\0')
        {
            if (name2[j] == '.')
            {
                name2[j] = '\0';
                break;
            }
            j++;
            if (j == 8)
            {
                name2[j] = '\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png \n", tex[i], name2);
    }

    tmpob = object;
    computeTriNorm(tmpob);
    tmpob = object;
    smoothTriNorm(tmpob);
    fprintf(ofile, "g\n");

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            fprintf(ofile, "v %lf %lf %lf\n", tmpob->vertex[i].x,
                    tmpob->vertex[i].y, tmpob->vertex[i].z);
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            fprintf(ofile, "vt %lf %lf 0.0\n", tmpob->textarray[i * 2],
                    tmpob->textarray[i * 2 + 1]);
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            fprintf(ofile, "vn %lf %lf %lf\n", tmpob->snorm[i].x,
                    tmpob->snorm[i].y, tmpob->snorm[i].z);
        }
        tmpob = tmpob->next;
    }
    fprintf(ofile, "g OB1\n");
    fprintf(ofile, "s 1\n");
    fprintf(ofile, "usemtl default\n");
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        ind = tmpob->numvert;
        printf("making obj face for %s \n", tmpob->name);

        for (int i = 0; i < tmpob->numsurf; i++)
        {
            int v1, v2, v3;
            v1 = tmpob->vertexarray[i * 3].indice;
            v2 = tmpob->vertexarray[i * 3 + 1].indice;
            v3 = tmpob->vertexarray[i * 3 + 2].indice;
            fprintf(ofile, "f %d/%d/%d %d/%d/%d %d/%d/%d \n", v1 + deltav,
                    v1 + deltav, v1 + deltav, v2 + deltav, v2 + deltav,
                    v2 + deltav, v3 + deltav, v3 + deltav, v3 + deltav);
        }
        deltav += ind;
        tmpob = tmpob->next;
    }
    fprintf(ofile, "end \n");
}

void stripifyOb(ob_t * object, int writeit)
{
    FILE *stripeout, *stripein;
    char line[256];
    char filename[50];
    char command[256];
    unsigned int j;
    unsigned int NumStrips;
    unsigned int NumStripPoints;
    void *mem;
    unsigned int * StripPoint;
    unsigned int * StripStart;
    unsigned int * StripLength;
    unsigned int CurrentStripNumber;
    unsigned int CurrentStripStart;
    unsigned int CurrentStripLength;
    unsigned int CurrentStripPoint;
    int i = 0;
    int debj = 0;
    int dege = 0;
    tcoord_t * stripvertexarray;
    int k, v1, v2, v0;
    k = 0;
    int tri = 0;
    int tritotal = 0;
    int multitex = 0;

    if (object->numsurf < 3 && writeit == 0)
        return;
    fprintf(stderr, "stripifying %s                    \r", object->name);
    sprintf(filename, "temp.obj");
    stripeout = fopen(filename, "w");
    for (i = 0; i < object->numvert; i++)
        fprintf(stripeout, "v 0.0 0.0 0.0\n");

    for (i = 0; i < object->numsurf; i++)
        fprintf(stripeout, "f %d %d %d\n",
                object->vertexarray[i * 3].indice + 1,
                object->vertexarray[i * 3 + 1].indice + 1,
                object->vertexarray[i * 3 + 2].indice + 1);

    fclose(stripeout);
#ifdef WIN32
    sprintf(command, "stripe.exe %s >shudup", filename);
    system(command);
    sprintf(command, "erase shudup");
    system(command);
    strcat(filename, "f");
    stripein = fopen(filename,"r");
#else
    sprintf(command, "stripe %s >/dev/null", filename);
    if (system(command) < 0)
        printf("Calling stripe failed");
    sprintf(command, "rm %s", filename);
    if (system(command) < 0)
        printf("Calling stripe failed");
    strcat(filename, "f");
    stripein = fopen("temp.objf", "r");
#endif

    /* Count the number of strip points and initialize PolyList */
    NumStrips = 0;
    NumStripPoints = 0;

    while (fscanf(stripein, "%s", line) != EOF)
    {
        switch (line[0])
        {
        case '#':
        case 'v':
            /* # is a comment, v is a vertex, we ignore both */
            while (fgetc(stripein) != '\n')
                ;
            break;

        case 't':
            /* t is the start of a new triangle strip */
            NumStrips++;
            break;

        case 'q':
        case '%':
            /* q is another entry in the current strip, % is a STRIPE type */
            break;

        default:
            /* Anything else is a point in the current triangle strip */
            NumStripPoints++;
            break;
        }
    }

    if (object->name != NULL)
        printf("name=%s stripnumber =%d \n", object->name, NumStrips);
    /* Allocate enough memory for what we just read */
    if ((mem = malloc(sizeof(unsigned int) * NumStripPoints)) == 0)
    {
        printf("Problem mallocing while stripifying\n");
        exit(-1);
    }
    StripPoint = (unsigned int *) mem;
    if ((mem = malloc(sizeof(unsigned int) * NumStrips)) == 0)
    {
        printf("Problem mallocing while stripifying\n");
        exit(-1);
    }
    StripStart = (unsigned int *) mem;
    if ((mem = malloc(sizeof(unsigned int) * NumStrips)) == 0)
    {
        printf("Problem mallocing while stripifying\n");
        exit(-1);
    }
    StripLength = (unsigned int *) mem;

    /* Fill the triangle strip lists with the STRIPE data */
    rewind(stripein);

    CurrentStripNumber = 0;
    CurrentStripStart = 0;
    CurrentStripLength = 0;
    CurrentStripPoint = 0;

    for (j = 0; j < NumStrips; j++)
    {
        StripStart[j] = 0;
        StripLength[j] = 0;
    }

    while (fscanf(stripein, "%s", line) != EOF)
    {
        switch (line[0])
        {
        case '#':
        case 'v':
            /* # is a comment, v is a vertex, we ignore both */
            while (fgetc(stripein) != '\n')
                ;
            break;

        case 't':
            /* t is the start of a new triangle strip */
            if (CurrentStripNumber > 0)
            {
                StripStart[CurrentStripNumber - 1] = CurrentStripStart;
                StripLength[CurrentStripNumber - 1] = CurrentStripLength;
                printf("striplength %d \n",
                        StripLength[CurrentStripNumber - 1]);
            }
            CurrentStripNumber++;
            CurrentStripStart = CurrentStripPoint;
            CurrentStripLength = 0;
            printf("new strip\n");
            break;

        case 'q':
        case '%':
            /* q is another entry in the current strip, % is a STRIPE type */
            break;

        default:
            /* Anything else is a point in the current triangle strip */
            StripPoint[CurrentStripPoint] = (unsigned int) (atoi(line) - 1);
            CurrentStripPoint++;
            CurrentStripLength++;
            break;
        }
    }
    if (CurrentStripNumber > 0)
    {
        StripStart[CurrentStripNumber - 1] = CurrentStripStart;
        StripLength[CurrentStripNumber - 1] = CurrentStripLength;
    }

    fclose(stripein);

    stripvertexarray = (tcoord_t *) malloc(
            sizeof(tcoord_t) * object->numvertice * 10);
    k = 0;
    dege = 0;
    if (writeit == 1)
    {
        fprintf(ofile, "numsurf %d\n", NumStrips);

    }
    if (object->texture1 || object->texture2 || object->texture3)
        multitex = 1;
    else
        multitex = 0;

    for (i = 0; i < (int) NumStrips; i++)
    {
        /* get the first triangle */
        v1 = StripPoint[StripStart[i]];
        v2 = StripPoint[StripStart[i] + 1];
        debj = 2;
        tri = 0;
        if (writeit == 1)
        {
            /** For some reason the surf attribute is modified for the output.
             *  The surfaces are made double-sided, although stripification doesn't
             *  introduce this property.
             *  Thus, instead of the whole if-condition the actual code for outputting
             *  this attribute should simply be:
             *
             *  fprintf(ofile, "SURF 0x%2x\n", object->attrSurf)
             *
             *  However this causes huge artifacts in the generated tracks. Thus, the
             *  following is not correct behavior but works for whatever reason.
             *  It is a legacy from TORCS and no details are known why it is done.
             *
             *  A proper solution would be to remove the attribute modification and
             *  rework all tracks to fit the correct behavior.
             */
            if (object->attrSurf)
            {
                fprintf(ofile, "SURF 0x%2x\n",
                        (object->attrSurf & 0xF0) | 0x04);
            }
            else
            {
                fprintf(ofile, "SURF 0x24\n");
            }
            fprintf(ofile, "mat %d\n", object->attrMat);
            fprintf(ofile, "refs %d\n", StripLength[i]);
            if (multitex == 0)
            {
                fprintf(ofile, "%d %.5f %.5f\n", v1, object->textarray[v1 * 2],
                        object->textarray[v1 * 2 + 1]);
            }
            else
            {
                fprintf(ofile, "%d %.5f %.5f ", v1, object->textarray[v1 * 2],
                        object->textarray[v1 * 2 + 1]);
                if (object->texture1)
                {
                    fprintf(ofile, "%.5f %.5f ", object->textarray1[v1 * 2],
                            object->textarray1[v1 * 2 + 1]);
                }
                if (object->texture2)
                {
                    fprintf(ofile, "%.5f %.5f ", object->textarray2[v1 * 2],
                            object->textarray2[v1 * 2 + 1]);
                }
                if (object->texture3)
                {
                    fprintf(ofile, "%.5f %.5f ", object->textarray3[v1 * 2],
                            object->textarray3[v1 * 2 + 1]);
                }
                fprintf(ofile, "\n");
            }
            if (multitex == 0)
            {
                fprintf(ofile, "%d %.5f %.5f\n", v2, object->textarray[v2 * 2],
                        object->textarray[v2 * 2 + 1]);
            }
            else
            {
                fprintf(ofile, "%d %.5f %.5f ", v2, object->textarray[v2 * 2],
                        object->textarray[v2 * 2 + 1]);
                if (object->texture1)
                {
                    fprintf(ofile, "%.5f %.5f ", object->textarray1[v2 * 2],
                            object->textarray1[v2 * 2 + 1]);
                }
                if (object->texture2)
                {
                    fprintf(ofile, "%.5f %.5f ", object->textarray2[v2 * 2],
                            object->textarray2[v2 * 2 + 1]);
                }
                if (object->texture3)
                {
                    fprintf(ofile, "%.5f %.5f ", object->textarray3[v2 * 2],
                            object->textarray3[v2 * 2 + 1]);
                }
                fprintf(ofile, "\n");

            }

        }
        for (j = debj; j < StripLength[i]; j++)
        {
            v0 = StripPoint[StripStart[i] + j];
            /*printf("adding point %d \n",v0);*/

            if (writeit == 0)
            {
                stripvertexarray[k].indice = v1;
                stripvertexarray[k].u = object->textarray[v1 * 2];
                stripvertexarray[k].v = object->textarray[v1 * 2 + 1];
                stripvertexarray[k].saved = 0;
                k++;
                stripvertexarray[k].indice = v2;
                stripvertexarray[k].u = object->textarray[v2 * 2];
                stripvertexarray[k].v = object->textarray[v2 * 2 + 1];
                stripvertexarray[k].saved = 0;
                k++;
                stripvertexarray[k].indice = v0;
                stripvertexarray[k].u = object->textarray[v0 * 2];
                stripvertexarray[k].v = object->textarray[v0 * 2 + 1];
                stripvertexarray[k].saved = 0;
                k++;
                if ((tri % 2) == 0)
                {
                    v1 = v0;
                }
                else
                {
                    v2 = v0;
                }
            }
            else
            {
                if (multitex == 0)
                    fprintf(ofile, "%d %.5f %.5f\n", v0,
                            object->textarray[v0 * 2],
                            object->textarray[v0 * 2 + 1]);
                else
                {
                    fprintf(ofile, "%d %.5f %.5f ", v0,
                            object->textarray[v0 * 2],
                            object->textarray[v0 * 2 + 1]);
                    if (object->texture1)
                    {
                        fprintf(ofile, "%.5f %.5f ", object->textarray1[v0 * 2],
                                object->textarray1[v0 * 2 + 1]);
                    }
                    if (object->texture2)
                    {
                        fprintf(ofile, "%.5f %.5f ", object->textarray2[v0 * 2],
                                object->textarray2[v0 * 2 + 1]);
                    }
                    if (object->texture3)
                    {
                        fprintf(ofile, "%.5f %.5f ", object->textarray3[v0 * 2],
                                object->textarray3[v0 * 2 + 1]);
                    }
                    fprintf(ofile, "\n");
                }
            }

            tri++;
        }
        tritotal += tri;

    }

    printf(
            "strips for %s : number of strips %d : average of points triangles by strips %.2f\n",
            object->name, NumStrips,
            (float) ((float) tritotal - (float) dege) / ((float) NumStrips));
    if (writeit == 0)
    {
        if (tritotal != object->numsurf)
        {
            printf(
                    "warning: error nb surf= %d != %d  degenerated triangles %d  tritotal=%d for %s \n",
                    tritotal, object->numsurf, dege, tritotal - dege,
                    object->name);
        }
        free(object->vertexarray);
        object->vertexarray = stripvertexarray;
        object->numvertice = k;
        object->numsurf = k / 3;
        free(StripPoint);
        free(StripStart);
        free(StripLength);
    }
}

void computeSaveAC3DM(char * OutputFilename, ob_t * object)
{

    int i = 0;
    int j = 0;
    char name2[256];
    char *p, *q;
    ob_t * tmpob = NULL;
    mat_t * tmat = NULL;
    int deltav = 1;
    int ind = 0;

    if ((ofile = fopen(OutputFilename, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename);
        return;
    }

    tmat = root_material;
    while (tmat != NULL)
    {
        if (strcmp(tmat->name, "root") == 0)
        {
            tmat = tmat->next;
            continue;
        }

        fprintf(ofile,
                "MATERIAL %s rgb %lf %lf %lf amb %lf %lf %lf emis %lf %lf %lf spec %lf %lf %lf shi %d trans0 \n",
                tmat->name, tmat->rgb.r, tmat->rgb.g, tmat->rgb.b, tmat->amb.r,
                tmat->amb.g, tmat->amb.b, tmat->emis.r, tmat->emis.g,
                tmat->emis.b, tmat->spec.r, tmat->spec.g, tmat->spec.b,
                (int) tmat->shi);

        tmat = tmat->next;
    }

    texnum = 0;
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        tmpob = tmpob->next;
    }

    for (i = 0; i < texnum; i++)
    {

        printf("analysing  %s \n", tex[i]);
        p = tex[i];
        q = name2;
        while (*p)
        {
            if ((*p <= 'Z' && *p >= 'A'))
            {
                *p = (*p - 'A') + 'a';
            }

            if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                    || (*p == '.'))
            {
                *q = *p;
                q++;
                *q = '\0';
            }
            p++;
        }
        j = 0;
        while (name2[j] != '\0')
        {
            if (name2[j] == '.')
            {
                name2[j] = '\0';
                break;
            }
            j++;
            if (j == 8)
            {
                name2[j] = '\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png \n", tex[i], name2);
    }

    tmpob = object;
    computeTriNorm(tmpob);
    tmpob = object;
    smoothTriNorm(tmpob);
    fprintf(ofile, "g\n");

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            fprintf(ofile, "v %lf %lf %lf\n", tmpob->vertex[i].x,
                    tmpob->vertex[i].y, tmpob->vertex[i].z);
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            fprintf(ofile, "vt %lf %lf 0.0\n", tmpob->textarray[i * 2],
                    tmpob->textarray[i * 2 + 1]);
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            fprintf(ofile, "vn %lf %lf %lf\n", tmpob->snorm[i].x,
                    tmpob->snorm[i].y, tmpob->snorm[i].z);
        }
        tmpob = tmpob->next;
    }
    fprintf(ofile, "g OB1\n");
    fprintf(ofile, "s 1\n");
    fprintf(ofile, "usemtl default\n");
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        ind = tmpob->numvert;
        printf("making obj face for %s \n", tmpob->name);

        for (int i = 0; i < tmpob->numsurf; i++)
        {
            int v1, v2, v3;
            v1 = tmpob->vertexarray[i * 3].indice;
            v2 = tmpob->vertexarray[i * 3 + 1].indice;
            v3 = tmpob->vertexarray[i * 3 + 2].indice;
            fprintf(ofile, "f %d/%d/%d %d/%d/%d %d/%d/%d \n", v1 + deltav,
                    v1 + deltav, v1 + deltav, v2 + deltav, v2 + deltav,
                    v2 + deltav, v3 + deltav, v3 + deltav, v3 + deltav);
        }
        deltav += ind;
        tmpob = tmpob->next;
    }
    fprintf(ofile, "end \n");
}

void mapNormalToSphere(ob_t *object)
{
    ob_t * tmpob = NULL;
    double xmin = 9999;
    double ymin = 9999;
    double zmin = 9999;
    double xmax = -9999;
    double ymax = -9999;
    double zmax = -9999;
    double pospt = 0;
    double ddmax = 0;
    double ddmin = 10000;
    int i = 0;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (tmpob->x_min < xmin)
            xmin = tmpob->x_min;
        if (tmpob->y_min < ymin)
            ymin = tmpob->y_min;
        if (tmpob->z_min < zmin)
            zmin = tmpob->z_min;

        if (tmpob->x_max > xmax)
            xmax = tmpob->x_max;
        if (tmpob->y_max > ymax)
            ymax = tmpob->y_max;
        if (tmpob->z_max > zmax)
            zmax = tmpob->z_max;

        for (i = 0; i < tmpob->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            pospt = sqrt(
                    tmpob->vertex[i].x * tmpob->vertex[i].x
                            + tmpob->vertex[i].y * tmpob->vertex[i].y
                            + tmpob->vertex[i].z * tmpob->vertex[i].z);
            if (pospt > ddmax)
                ddmax = pospt;
            if (pospt < ddmin)
                ddmin = pospt;
        }
        tmpob = tmpob->next;
    }
    ddmin = (ddmax - ddmin) / 2 + ddmin;
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (i = 0; i < tmpob->numvert; i++)
        {
            double fact = 0;
            /* compute the same normal for each points in the surface */
            pospt = sqrt(
                    tmpob->vertex[i].x * tmpob->vertex[i].x
                            + tmpob->vertex[i].y * tmpob->vertex[i].y
                            + tmpob->vertex[i].z * tmpob->vertex[i].z);
            fact = ddmin / pospt;
            if (fact > 1.0)
                fact = 1.0;
            tmpob->snorm[i].x *= fact;
            tmpob->snorm[i].y *= fact;
            tmpob->snorm[i].z *= fact;

        }

        tmpob = tmpob->next;
    }

}
void mapTextureEnv(ob_t *object)
{
    ob_t * tmpob = NULL;
    double x, y, z, zt, lg;
    double z_min = 10000;
    double z_max = -10000;
    int i, j;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (j = 0; j < tmpob->numvert; j++)
        {
            z = tmpob->vertex[j].z + tmpob->snorm[j].z / 3.0;
            if (z > z_max)
                z_max = z;
            if (z < z_min)
                z_min = z;
        }
        tmpob = tmpob->next;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        /* create the new vertex array */
        tmpob->textarray1 = (double *) malloc(
                sizeof(double) * tmpob->numvert * 2);
        tmpob->textarray2 = (double *) malloc(
                sizeof(double) * tmpob->numvert * 2);
        memcpy(tmpob->textarray1, tmpob->textarray,
                tmpob->numvert * sizeof(double) * 2);
        memcpy(tmpob->textarray2, tmpob->textarray,
                tmpob->numvert * sizeof(double) * 2);
        tmpob->texture1 = tmpob->texture;
        tmpob->texture2 = tmpob->texture;
        for (i = 0; i < tmpob->numvert; i++)
        {
            x = tmpob->vertex[i].x;
            y = tmpob->vertex[i].y;
            z = tmpob->vertex[i].z;
            lg = sqrt(x * x + y * y + z * z);
            if (lg != 0.0)
            {
                x /= lg;
                y /= lg;
                z /= lg;
            }
            else
            {
                x = 0;
                y = 0;
                z = 1;
            }
            //z_min = 0;
            tmpob->textarray1[i * 2] = 0.5 + x / 2.0;
            zt = (z + tmpob->snorm[i].z / 3.0 - z_min) / (z_max - z_min);
            tmpob->textarray1[i * 2 + 1] = zt;

            if (tmpob->textarray1[i * 2 + 1] > 1.0)
                tmpob->textarray1[i * 2 + 1] = 0.999;
            else if (tmpob->textarray1[i * 2 + 1] < 0.0)
                tmpob->textarray1[i * 2 + 1] = 0.001;

            tmpob->textarray2[i * 2] = 0.5 + y / 2.0;
            tmpob->textarray2[i * 2 + 1] = z;

        }
        tmpob = tmpob->next;
    }
}

void mapTextureEnvOld(ob_t *object)
{
    ob_t * tmpob = NULL;
    int i = 0;
    int j = 0;
    double x_min = 10000;
    double x_max = -10000;
    double y_min = 10000;
    double y_max = -10000;
    double z_min = 10000;
    double z_max = -10000;
    double u_min = 10000;
    double u_max = -10000;
    double v_min = 10000;
    double v_max = -10000;

    double u2_min = 10000;
    double u2_max = -10000;
    double v2_min = 10000;
    double v2_max = -10000;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (j = 0; j < tmpob->numvert; j++)
        {
            if (tmpob->vertex[j].x > x_max)
                x_max = tmpob->vertex[j].x;
            if (tmpob->vertex[j].x < x_min)
                x_min = tmpob->vertex[j].x;

            if (tmpob->vertex[j].y > y_max)
                y_max = tmpob->vertex[j].y;
            if (tmpob->vertex[j].y < y_min)
                y_min = tmpob->vertex[j].y;

            if (tmpob->vertex[j].z > z_max)
                z_max = tmpob->vertex[j].z;
            if (tmpob->vertex[j].z < z_min)
                z_min = tmpob->vertex[j].z;
        }
        tmpob = tmpob->next;
    }
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        /* create the new vertex array */
        tmpob->textarray1 = (double *) malloc(
                sizeof(double) * tmpob->numvert * 2);
        tmpob->textarray2 = (double *) malloc(
                sizeof(double) * tmpob->numvert * 2);
        memcpy(tmpob->textarray1, tmpob->textarray,
                tmpob->numvert * sizeof(double) * 2);
        memcpy(tmpob->textarray2, tmpob->textarray,
                tmpob->numvert * sizeof(double) * 2);
        tmpob->texture1 = tmpob->texture;
        tmpob->texture2 = tmpob->texture;
        for (i = 0; i < tmpob->numvert; i++)
        {

            tmpob->textarray1[i * 2] = (tmpob->vertex[i].x - x_min)
                    / (x_max - x_min) + (tmpob->snorm[i].x) / 2;
            tmpob->textarray1[i * 2 + 1] = ((tmpob->vertex[i].z - z_min)
                    / (z_max - z_min)) + (tmpob->snorm[i].z) / 2;
            tmpob->textarray2[i * 2] = ((tmpob->vertex[i].x - x_min)
                    / (x_max - x_min)) + (tmpob->snorm[i].x) / 2;
            tmpob->textarray2[i * 2 + 1] = ((tmpob->vertex[i].y - y_min)
                    / (x_max - x_min)) + (tmpob->snorm[i].y) / 2;

            if (tmpob->textarray1[i * 2] > u_max)
                u_max = tmpob->textarray1[i * 2];

            if (tmpob->textarray1[i * 2 + 1] > v_max)
                v_max = tmpob->textarray1[i * 2 + 1];

            if (tmpob->textarray1[i * 2] < u_min)
                u_min = tmpob->textarray1[i * 2];

            if (tmpob->textarray1[i * 2] < v_min)
                v_min = tmpob->textarray1[i * 2];

            if (tmpob->textarray2[i * 2] > u2_max)
                u2_max = tmpob->textarray2[i * 2];

            if (tmpob->textarray2[i * 2 + 1] > v2_max)
                v2_max = tmpob->textarray2[i * 2 + 1];

            if (tmpob->textarray2[i * 2] < u2_min)
                u2_min = tmpob->textarray2[i * 2];

            if (tmpob->textarray2[i * 2] < v2_min)
                v2_min = tmpob->textarray2[i * 2];

        }
        tmpob = tmpob->next;
    }

    /* clamp the texture coord */
    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }

        for (i = 0; i < tmpob->numvert; i++)
        {
            tmpob->textarray1[i * 2] = (tmpob->textarray1[i * 2] - u_min)
                    / (u_max - u_min);
            tmpob->textarray1[i * 2 + 1] =
                    (tmpob->textarray1[i * 2 + 1] - v_min) / (v_max - v_min);

            tmpob->textarray2[i * 2] = (tmpob->textarray2[i * 2] - u2_min)
                    / (u2_max - u2_min) - 0.5;
            tmpob->textarray2[i * 2 + 1] = (tmpob->textarray2[i * 2 + 1]
                    - v2_min) / (v2_max - v2_min) - 0.5;
        }
        tmpob = tmpob->next;
    }

}

void mapNormalToSphere2(ob_t *object)
{
    ob_t * tmpob = NULL;
    int i = 0;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }

        for (i = 0; i < tmpob->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            /*      tmpob->norm[i].x = tmpob->vertex[i].x; */
            /*      tmpob->norm[i].y = tmpob->vertex[i].y; */
            /*      tmpob->norm[i].z = tmpob->vertex[i].z; */
            /*      normalize(&tmpob->norm[i]); */
            /*      tmpob->snorm[i].x += tmpob->norm[i].x; */
            /*      tmpob->snorm[i].y += tmpob->norm[i].y; */
            /*      tmpob->snorm[i].z += tmpob->norm[i].z; */
            normalize(&tmpob->snorm[i]);
        }
        tmpob = tmpob->next;
    }

}

void normalMap(ob_t * object)
{
    ob_t * tmpob = NULL;
    double x_min = 99999;
    double y_min = 99999;
    double z_min = 99999;
    double x_max = -99999;
    double y_max = -99999;
    double z_max = -99999;
    int i = 0;
    int j = 0;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }

        for (j = 0; j < tmpob->numvert; j++)
        {
            if (tmpob->vertex[j].x > x_max)
                x_max = tmpob->vertex[j].x;
            if (tmpob->vertex[j].x < x_min)
                x_min = tmpob->vertex[j].x;

            if (tmpob->vertex[j].y > y_max)
                y_max = tmpob->vertex[j].y;
            if (tmpob->vertex[j].y < y_min)
                y_min = tmpob->vertex[j].y;

            if (tmpob->vertex[j].z > z_max)
                z_max = tmpob->vertex[j].z;
            if (tmpob->vertex[j].z < z_min)
                z_min = tmpob->vertex[j].z;
        }
        tmpob = tmpob->next;
    }
    tmpob = object;

    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        printf("normalMap : handling %s \n", tmpob->name);
        for (i = 0; i < tmpob->numvert; i++)
        {
            tmpob->textarray[i * 2] = (tmpob->vertex[i].x - x_min)
                    / (x_max - x_min);
            tmpob->textarray[i * 2 + 1] = (tmpob->vertex[i].y - y_min)
                    / (y_max - y_min);
        }
        tmpob->texture = shadowtexture;
        tmpob = tmpob->next;
    }

}

void normalMap01(ob_t * object)
{
    ob_t * tmpob = NULL;
    double x_min = 99999;
    double y_min = 99999;
    double z_min = 99999;
    double x_max = -99999;
    double y_max = -99999;
    double z_max = -99999;
    int i = 0;
    int j = 0;

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        for (j = 0; j < tmpob->numvert; j++)
        {
            if (tmpob->vertex[j].x > x_max)
                x_max = tmpob->vertex[j].x;
            if (tmpob->vertex[j].x < x_min)
                x_min = tmpob->vertex[j].x;

            if (tmpob->vertex[j].y > y_max)
                y_max = tmpob->vertex[j].y;
            if (tmpob->vertex[j].y < y_min)
                y_min = tmpob->vertex[j].y;

            if (tmpob->vertex[j].z > z_max)
                z_max = tmpob->vertex[j].z;
            if (tmpob->vertex[j].z < z_min)
                z_min = tmpob->vertex[j].z;
        }

        tmpob = tmpob->next;
    }
    tmpob = object;

    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        tmpob->textarray3 = (double *) malloc(
                sizeof(double) * tmpob->numvert * 2);
        printf("normalMap : handling %s \n", tmpob->name);
        for (i = 0; i < tmpob->numvert; i++)
        {
            tmpob->textarray3[i * 2] = (tmpob->vertex[i].x - x_min)
                    / (x_max - x_min) - 0.5;
            tmpob->textarray3[i * 2 + 1] = (tmpob->vertex[i].y - y_min)
                    / (y_max - y_min) - 0.5;
        }
        tmpob->texture3 = tmpob->texture;
        tmpob = tmpob->next;
    }
}

void computeSaveAC3DStrip(char * OutputFilename, ob_t * object)

{

    int i = 0;
    int ik = 0;
    ob_t * tmpob = NULL;
    mat_t * tmat = NULL;
    int numg = 0;
    char *p;
    char *q = NULL;
    int lastpass = FALSE;
    int nborder = 0;
    int ordering = FALSE;

    if ((ofile = fopen(OutputFilename, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename);
        return;
    }
    smoothTriNorm(object);
    if (isobjectacar)
    {
        mapNormalToSphere2(object);
        normalMap01(object);
    }
    if (isobjectacar && extendedEnvCoord)
        mapTextureEnv(object);
    if (isobjectacar && collapseObject)
        mergeSplitted(&object);

    fprintf(ofile, "AC3Db\n");
    tmat = root_material;
    while (tmat != NULL)
    {
        if (strcmp(tmat->name, "root") == 0)
        {
            tmat = tmat->next;
            continue;
        }
        fprintf(ofile,
                "MATERIAL %s rgb %1.2f %1.2f %1.2f amb %1.2f %1.2f %1.2f emis %1.2f %1.2f %1.2f spec %1.2f %1.2f %1.2f shi %d trans 0 \n",
                tmat->name, tmat->rgb.r, tmat->rgb.g, tmat->rgb.b, tmat->amb.r,
                tmat->amb.g, tmat->amb.b, tmat->emis.r, tmat->emis.g,
                tmat->emis.b, tmat->spec.r, tmat->spec.g, tmat->spec.b,
                (int) tmat->shi);
        tmat = tmat->next;
    }

    fprintf(ofile, "OBJECT world\n");

    if (OrderString)
    {
        fprintf(stderr, "ordering objects according to  %s\n", OrderString);
        p = OrderString;
        ordering = TRUE;
        nborder = 1;
        while (TRUE)
        {
            q = strstr(p, ";");
            if (q != NULL)
                nborder++;
            else
                break;
            p = q + 1;
            if (*p == '\0')
            {
                nborder--;
                break;
            }
        }
    }
    else
    {
        ordering = FALSE;
        nborder = 0;
    }

    tmpob = object;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        numg++;
        tmpob->saved = 0;
        tmpob = tmpob->next;
    }

    fprintf(ofile, "kids %d\n", numg);

    texnum = 0;

    p = OrderString;
    q = OrderString;
    nborder++;
    for (ik = 0; ik < nborder; ik++)
    {
        if (ordering)
        {
            /* look to the current object name to save */
            if (p == NULL)
                lastpass = TRUE;
            else
            {
                q = p;
                p = strstr(p, ";");
                if (p != NULL)
                {
                    *p = '\0';
                    p++;
                }
            }
        }
        tmpob = object;
        while (tmpob != NULL)
        {
            int texnofound = 0;
            if (tmpob->name == NULL)
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!strcmp(tmpob->name, "root"))
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!strcmp(tmpob->name, "world"))
            {
                tmpob = tmpob->next;
                continue;
            }
            texnofound = 1;
            for (i = 0; i < texnum; i++)
            {
                if (tmpob->texture == NULL)
                {
                    texnofound = 0;
                    break;

                }
                if (!strncmp(tex[i], tmpob->texture, 13))
                {
                    texnofound = 0;
                    break;

                }
                else
                    texnofound = 1;
            }
            if (texnofound == 1)
            {
                if (tmpob->texture != NULL)
                {
                    strcpy(tex[texnum], tmpob->texture);
                    tex[texnum][13] = '\0';
                    /*sprintf(tex[texnum],"%s",tmpob->texture);*/
                }
                texnum++;
            }
            printf("name=%s x_min=%.1f y_min=%.1f x_max=%.1f y_max=%.1f\n",
                    tmpob->name, tmpob->x_min, tmpob->y_min, tmpob->x_max,
                    tmpob->y_max);

            tmpob = tmpob->next;
        }

        tmpob = object;
        tmpob->kids_o = 0;

        tmpob = object;
        while (tmpob != NULL)
        {
            tmpob->inkids_o = 0;
            tmpob = tmpob->next;
        }

        tmpob = object;
        while (tmpob != NULL)
        {
            if (tmpob->name == NULL)
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!strcmp(tmpob->name, "root"))
            {
                tmpob = tmpob->next;
                continue;
            }
            if (!strcmp(tmpob->name, "world"))
            {
                tmpob = tmpob->next;
                continue;
            }
            if (tmpob->saved == 0)
            {

                if (ordering && !lastpass)
                {
                    if (!strcmp(tmpob->name, q))
                    {
                        printOb(tmpob);
                        printf("object =%s num kids_o=%d test with %s\n",
                                tmpob->name, tmpob->kids_o, q);
                    }
                    else
                    {
                        char nameBuf[1024];
                        sprintf(nameBuf, "%ss", q);
                        if (!strncmp(tmpob->name, nameBuf, strlen(nameBuf)))
                        {
                            printOb(tmpob);
                            printf("object =%s num kids_o=%d\n", tmpob->name,
                                    tmpob->kids_o);
                        }

                    }
                }
                else
                {
                    printOb(tmpob);
                    printf("object =%s num kids_o=%d\n", tmpob->name,
                            tmpob->kids_o);
                }
            }
            tmpob = tmpob->next;
        }
    }
    tmpIndice = 0;
    /* do the job */
    printf("\nend\n");

}

ob_t * mergeObject(ob_t *ob1, ob_t * ob2, char * nameS)
{
    ob_t * tobS = NULL;
    int oldva1[10000];
    int oldva2[10000];
    int n = 0;
    int i = 0;
    int j = 0;

    int numtri = (ob1)->numsurf + (ob2)->numsurf;
    ;
    printf("merging %s with %s  tri=%d\n", ob1->name, ob2->name, numtri);
    memset(oldva1, -1, sizeof(oldva1));
    memset(oldva2, -1, sizeof(oldva2));
    tobS = (ob_t *) malloc(sizeof(ob_t));
    memset(tobS, 0, sizeof(ob_t));
    tobS->x_min = 1000000;
    tobS->y_min = 1000000;
    tobS->z_min = 1000000;
    tobS->numsurf = ob1->numsurf;
    tobS->vertexarray = (tcoord_t *) malloc(sizeof(tcoord_t) * numtri * 3);
    tobS->norm = (point_t*) malloc(sizeof(point_t) * numtri * 3);
    tobS->snorm = (point_t*) malloc(sizeof(point_t) * numtri * 3);
    tobS->vertex = (point_t*) malloc(sizeof(point_t) * numtri * 3);
    memset(tobS->snorm, 0, sizeof(point_t) * numtri * 3);
    memset(tobS->norm, 0, sizeof(point_t) * numtri * 3);
    tobS->textarray = (double *) malloc(sizeof(tcoord_t) * numtri * 2 * 3);
    tobS->textarray1 = (double *) malloc(sizeof(tcoord_t) * numtri * 2 * 3);
    tobS->textarray2 = (double *) malloc(sizeof(tcoord_t) * numtri * 2 * 3);
    tobS->textarray3 = (double *) malloc(sizeof(tcoord_t) * numtri * 2 * 3);
    tobS->attrSurf = ob1->attrSurf;
    tobS->attrMat = ob1->attrMat;
    tobS->name = (char *) malloc(strlen(nameS) + 1);
    tobS->texture = strdup(nameS);
    tobS->type = ob1->type ? strdup(ob1->type) : NULL;
    tobS->data = ob1->data ? strdup(ob1->data) : NULL;

    memcpy(tobS->vertex, ob1->vertex, ob1->numvert * sizeof(point_t));
    memcpy(tobS->vertexarray, ob1->vertexarray,
            ob1->numsurf * sizeof(tcoord_t) * 3);
    memcpy(tobS->textarray, ob1->textarray,
            ob1->numvert * sizeof(tcoord_t) * 2);
    memcpy(tobS->norm, ob1->norm, ob1->numvert * sizeof(point_t));
    memcpy(tobS->snorm, ob1->snorm, ob1->numvert * sizeof(point_t));

    if (ob1->texture1)
    {
        memcpy(tobS->textarray1, ob1->textarray1,
                ob1->numvert * 2 * sizeof(tcoord_t));
    }
    if (ob1->texture2)
    {
        memcpy(tobS->textarray2, ob1->textarray2,
                ob1->numvert * 2 * sizeof(tcoord_t));
    }
    if (ob1->texture3)
    {
        memcpy(tobS->textarray3, ob1->textarray3,
                ob1->numvert * 2 * sizeof(tcoord_t));
    }

    n = ob1->numvert;
    for (i = 0; i < ob2->numvert; i++)
    {
        for (j = 0; j < ob1->numvert; j++)
        {
            if (ob2->vertex[i].x == ob1->vertex[j].x
                    && ob2->vertex[i].y == ob1->vertex[j].y
                    && ob2->vertex[i].z == ob1->vertex[j].z
                    && ob2->textarray[i * 2] == ob1->textarray[j * 2]
                    && ob2->textarray[i * 2 + 1] == ob1->textarray[j * 2 + 1])
            {
                oldva1[i] = j;
            }
        }
    }

    for (i = 0; i < ob2->numvert; i++)
    {
        if (oldva1[i] == -1)
        {
            oldva1[i] = n;
            tobS->textarray[n * 2] = ob2->textarray[i * 2];
            tobS->textarray[n * 2 + 1] = ob2->textarray[i * 2 + 1];
            if (ob2->texture1)
            {
                tobS->textarray1[n * 2] = ob2->textarray1[i * 2];
                tobS->textarray1[n * 2 + 1] = ob2->textarray1[i * 2 + 1];
            }
            if (ob2->texture2)
            {
                tobS->textarray2[n * 2] = ob2->textarray2[i * 2];
                tobS->textarray2[n * 2 + 1] = ob2->textarray2[i * 2 + 1];
            }
            if (ob2->texture3)
            {
                tobS->textarray3[n * 2] = ob2->textarray3[i * 2];
                tobS->textarray3[n * 2 + 1] = ob2->textarray3[i * 2 + 1];
            }
            tobS->snorm[n].x = ob2->snorm[i].x;
            tobS->snorm[n].y = ob2->snorm[i].y;
            tobS->snorm[n].z = ob2->snorm[i].z;
            tobS->norm[n].x = ob2->norm[i].x;
            tobS->norm[n].y = ob2->norm[i].y;
            tobS->norm[n].z = ob2->norm[i].z;
            tobS->vertex[n].x = ob2->vertex[i].x;
            tobS->vertex[n].y = ob2->vertex[i].y;
            tobS->vertex[n].z = ob2->vertex[i].z;

            n++;
        }
    }
    tobS->numvert = n;
    for (i = 0; i < ob2->numsurf; i++)
    {
        int found = FALSE;
        for (j = 0; j < ob1->numsurf; j++)
        {
            if (tobS->vertexarray[j * 3].indice
                    == oldva1[ob2->vertexarray[i * 3].indice]
                    && tobS->vertexarray[j * 3 + 1].indice
                            == oldva1[ob2->vertexarray[i * 3 + 1].indice]
                    && tobS->vertexarray[j * 3 + 2].indice
                            == oldva1[ob2->vertexarray[i * 3 + 2].indice])
            {
                /* this face is OK */
                found = TRUE;
                break;
            }
        }
        if (found == FALSE)
        {
            int k = tobS->numsurf;
            /* add the triangle */
            tobS->vertexarray[k * 3].indice =
                    oldva1[ob2->vertexarray[i * 3].indice];
            tobS->vertexarray[k * 3 + 1].indice = oldva1[ob2->vertexarray[i * 3
                    + 1].indice];
            tobS->vertexarray[k * 3 + 2].indice = oldva1[ob2->vertexarray[i * 3
                    + 2].indice];
            tobS->numsurf++;
        }
    }

    ob1->numsurf = tobS->numsurf;
    ob1->numvert = tobS->numvert;
    freez(ob1->vertexarray);
    ob1->vertexarray = tobS->vertexarray;
    freez(ob1->norm);
    ob1->norm = tobS->norm;
    freez(ob1->snorm);
    ob1->snorm = tobS->snorm;
    freez(ob1->vertex);
    ob1->vertex = tobS->vertex;
    freez(ob1->textarray);
    ob1->textarray = tobS->textarray;
    freez(ob1->textarray1);
    ob1->textarray1 = tobS->textarray1;
    freez(ob1->textarray2);
    ob1->textarray2 = tobS->textarray2;
    freez(ob1->textarray3);
    ob1->textarray3 = tobS->textarray3;
    return ob1;
}

int mergeSplitted(ob_t **object)
{

    int k = 0;
    char nameS[256];
    char *p;
    ob_t * tob = NULL;
    ob_t * tob0 = NULL;
    ob_t * tobP = NULL;
#ifdef NEWSRC
    int numtri;
#endif
    int reduced = 0;

    tob = *object;
    while (tob)
    {
        if (isobjectacar)
        {
            if (tob->name == NULL)
            {
                tob = tob->next;
                continue;
            }
            if (strstr(tob->name, "_s_") == NULL)
            {
                tob = tob->next;
                continue;
            }
        }
        else if (strstr(tob->name, "__split__") == NULL)
        {
            tob = tob->next;
            continue;
        }
        tobP = tob;
        tob0 = tob->next;
        sprintf(nameS, "%s", tob->name);
        if (isobjectacar)
        {
            p = strstr(nameS, "_s_");
        }
        else
            p = strstr(nameS, "__split__");
        if (p == NULL)
        {
            tob = tob->next;
            continue;
        }
        printf("looking for merge : %s\n", nameS);
        if (isobjectacar)
            p = p + strlen("_s_");
        else
            p = p + strlen("__split__");
        *p = '\0';
        k = 0;
#ifdef NEWSRC
        numtri = tob->numsurf;
#endif
        while (tob0)
        {
            if (tob0->name == NULL)
            {
                tobP = tob0;
                tob0 = tob0->next;
                continue;
            }
            if (!strcmp(tob0->name, "world") || !strcmp(tob0->name, "root"))
            {
                tobP = tob0;
                tob0 = tob0->next;
                continue;
            }
            if (tob0->type != NULL)
                if (!strcmp(tob0->type, "group"))
                {
                    tobP = tob0;
                    tob0 = tob0->next;
                    continue;
                }

            if (!strnicmp(tob0->name, nameS, strlen(nameS)))
            {
                ob_t *oo;
                mergeObject(tob, tob0, nameS);
                /*printf("merging %s with %s\n",nameS, tob0->name);*/
                reduced++;
                tobP->next = tob0->next;
                oo = tob0;
                tob0 = tob0->next;
                obFree(oo);
                k++;
                continue;
            }
            tobP = tob0;
            tob0 = tob0->next;
        }

        if (k == 0)
        {
            tob = tob->next;
            continue;
        }
        printf("need merge for %s : %d objects found \n", tob->name, k + 1);

        /* we know that nameS has k+1 objects and need to be merged */

        /* allocate the new object */
#ifdef NEWSRC
        tobS=(ob_t *)malloc(sizeof(ob_t ));
        memset(tobS,0,sizeof(ob_t));
        tobS->x_min=1000000;
        tobS->y_min=1000000;
        tobS->z_min=1000000;
        tobS->numsurf=numtri;
        tobS->vertexarray=(tcoord_t *) malloc(sizeof(tcoord_t)*numtri*3);
        tobS->norm=(point_t*)malloc(sizeof(point_t)*numtri*3);
        tobS->snorm=(point_t*)malloc(sizeof(point_t)*numtri*3);
        tobS->vertex=(point_t*)malloc(sizeof(point_t)*numtri*3);
        memset(tobS->snorm,0,sizeof(point_t )*numtri*3);
        memset(tobS->norm,0,sizeof(point_t )*numtri*3);
        tobS->textarray=(double *) malloc(sizeof(tcoord_t)* numtri*2*3);
        tobS->attrSurf=tob->attrSurf;
        tobS->attrMat=tob->attrMat;
        tobS->name=(char *) malloc(strlen(nameS)+1);
        tobS->texture=strdup(nameS);
        tobS->type= tob->type ? strdup(tob->type) : NULL;
        tobS->data=strdup(tob->data);

        memcpy(tobS->vertex, tob->vertex,tob->numvert*sizeof(point_t));
        memcpy(tobS->vertexarray, tob->vertexarray,tob->numsurf*sizeof(tcoord_t ));
        memcpy(tobS->textarray, tob->textarray,tob->numvert*sizeof(tcoord_t )*2);

        if (tob->texture1)
        {
            memcpy(tobS->textarray1, tob->textarray1,tob->numvert*2*sizeof(tcoord_t ));
            memcpy(tobS->vertexarray1, tob->vertexarray1,tob->numsurf*sizeof(tcoord_t ));
            tobS->texture1=strdup(tob->texture1);
        }
        if (tob->texture2)
        {
            memcpy(tobS->textarray2, tob->textarray2,tob->numvert*2*sizeof(tcoord_t ));
            memcpy(tobS->vertexarray2, tob->vertexarray2,tob->numsurf*sizeof(tcoord_t ));
            tobS->texture2=strdup(tob->texture2);
        }
        if (tob->texture3)
        {
            memcpy(tobS->textarray3, tob->textarray3,tob->numvert*2*sizeof(tcoord_t ));
            memcpy(tobS->vertexarray3, tob->vertexarray3,tob->numsurf*sizeof(tcoord_t ));
            tobS->texture3=strdup(tob->texture3);
        }

        n=tob->numvert;

        /* now add the new points */
        tob0=tob->next;
        while (tob0)
        {

            if (strnicmp(tob0->name,nameS,strlen(nameS)))
            {
                tob0=tob0->next;
                continue;
            }

            for (j=0; j<tob0->numvert; j++)
            {

                tobS->vertex[j].x=tob->vertex[j].x;
                tobS->vertex[j].y=tob->vertex[j].y;
                tobS->vertex[j].z=tob->vertex[j].z;
            }
            for (j=0; j<tob->numsurf; j++)
            {
                tobS->vertexarray[j*3].indice=tob->vertexarray[j*3].indice;
                tobS->vertexarray[j*3].u=tob->vertexarray[j*3].u;
                tobS->vertexarray[j*3].v=tob->vertexarray[j*3].v;
                tobS->vertexarray[j*3+1].indice=tob->vertexarray[j*3+1].indice;
                tobS->vertexarray[j*3+1].u=tob->vertexarray[j*3+1].u;
                tobS->vertexarray[j*3+1].v=tob->vertexarray[j*3+1].v;
                tobS->vertexarray[j*3+2].indice=tob->vertexarray[j*3+2].indice;
                tobS->vertexarray[j*3+2].u=tob->vertexarray[j*3+2].u;
                tobS->vertexarray[j*3+2].v=tob->vertexarray[j*3+2].v;
            }
            for (j=0; j<tob->numsurf*3; j++)
            {
                tobS->textarray[tobS->vertexarray[j].indice*2]=tob->vertexarray[j].u;
                tobS->textarray[tobS->vertexarray[j].indice*2+1]=tob->vertexarray[j].v;
                tobS->textarray1[tobS->vertexarray[j].indice*2]=tob->vertexarray1[j].u;
                tobS->textarray1[tobS->vertexarray[j].indice*2+1]=tob->vertexarray1[j].v;
            }
            tob0=tob0->next;
        }
        for (j=0; j<tobS->numvert; j++)
        {
            if (tobS->vertex[j].x>tobS->x_max)
            tobS->x_max=tobS->vertex[j].x;
            if (tobS->vertex[j].x<tobS->x_min)
            tobS->x_min=tobS->vertex[j].x;

            if (tobS->vertex[j].y>tobS->y_max)
            tobS->y_max=tobS->vertex[j].y;
            if (tobS->vertex[j].y<tobS->y_min)
            tobS->y_min=tobS->vertex[j].y;

            if (tobS->vertex[j].z>tobS->z_max)
            tobS->z_max=tobS->vertex[j].z;
            if (tobS->vertex[j].z<tobS->z_min)
            tobS->z_min=tobS->vertex[j].z;

        }

#endif
        tob = tob->next;
    }

    return reduced;
}

int findPoint(point_t * vertexArray, int sizeVertexArray, point_t * theVertex)
{

    return -1;
}

#define P2(x) ((x)*(x))

double findDistmin(ob_t * ob1, ob_t *ob2)
{

    double di[16];
    double d = 100000;
    int i;

    for (i = 0; i < ob1->numvert; i++)
        for (int j = 0; j < ob2->numvert; j++)
        {
            double a1 = ob1->vertex[i].x;
            double b1 = ob1->vertex[i].y;
            double a2 = ob2->vertex[j].x;
            double b2 = ob2->vertex[j].y;
            di[0] = P2(a1-a2) + P2(b1-b2);
            if (di[0] < d)
                d = di[0];
        }

    return d;

}
