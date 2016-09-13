/***************************************************************************

    file                 : AccGeode.cpp
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2013 by Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id$

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <assert.h>
#include <list>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/Drawable>
#include <osg/Geometry>
#include <limits>
#include <iomanip>

#include "AccException.h"
#include "AccGeode.h"

using namespace acc3d;
using namespace std;

void Geode::OutputVertex(int Index, const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices, ostream& fout)
{
    int LocalTexIndex;
    int LocalVertexIndex;
    if (NULL == pVertexIndices)
        LocalVertexIndex = Index;
    else
        LocalVertexIndex = pVertexIndices->index(Index);
    if (NULL != pTexCoords)
    {
        // Got some tex coords
        // Check for an index
        if (NULL != pTexIndices)
            // Access tex coord array indirectly
            LocalTexIndex = pTexIndices->index(Index);
        else
            LocalTexIndex  = Index;
        fout << LocalVertexIndex << " " << pTexCoords[LocalTexIndex][0] << " " << pTexCoords[LocalTexIndex][1] << std::endl;
    }
    else
        fout << LocalVertexIndex << " 0 0" << std::endl;
}

void Geode::OutputLines(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                        const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();
    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd; vindex+=2)
    {
        OutputSurfHead(iCurrentMaterial,surfaceFlags,2, fout);

        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputLineStrip(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                            const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();
    OutputSurfHead(iCurrentMaterial,surfaceFlags,indexEnd-drawArray->getFirst(), fout);

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd; ++vindex)
    {
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputLineLoop(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                           const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();
    OutputSurfHead(iCurrentMaterial,surfaceFlags,indexEnd-drawArray->getFirst(), fout);

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd; ++vindex)
    {
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}
void Geode::OutputTriangle(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                           const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, ostream& fout)
{
    unsigned int primCount = 0;
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd; ++vindex,++primCount)
    {
        
        if ((primCount%3) == 0)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,3, fout);
        }
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputTriangleStrip(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                                const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, std::ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();
    unsigned int evenodd=0;

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd-2; ++vindex, evenodd++)
    {
        OutputSurfHead(iCurrentMaterial,surfaceFlags,3, fout);

        if (evenodd%2==0)
        {
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
        } else
        {
            OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        }
        OutputVertex(vindex+2, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputTriangleFan(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                              const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, std::ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();

    for(unsigned int vindex=drawArray->getFirst()+1; vindex<indexEnd-1; ++vindex)
    {
        OutputSurfHead(iCurrentMaterial,surfaceFlags,3, fout);

        OutputVertex(drawArray->getFirst(), pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuads(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                        const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, std::ostream& fout)
{
    unsigned int primCount = 0;
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd; ++vindex,++primCount)
    {
        if ((primCount%4) == 0)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,4, fout);
        }
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadStrip(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                            const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, std::ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd-2; vindex+=2)
    {
        OutputSurfHead(iCurrentMaterial,surfaceFlags,4, fout);

        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindex+3, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindex+2, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputPolygon(const int iCurrentMaterial, const unsigned int surfaceFlags, 
                          const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrays* drawArray, std::ostream& fout)
{
    unsigned int indexEnd = drawArray->getFirst() + drawArray->getCount();

    OutputSurfHead(iCurrentMaterial,surfaceFlags,drawArray->getCount(), fout);

    for(unsigned int vindex=drawArray->getFirst(); vindex<indexEnd; vindex++)
    {
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

//=======  draw array length cases
void Geode::OutputLineDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                           const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end(); ++primItr)
    {
        unsigned int localPrimLength;
        localPrimLength = 2;
        
        for(GLsizei primCount = 0; primCount < *primItr; ++primCount)
        {
            if ((primCount%localPrimLength)==0)
            {
                OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
            }
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            ++vindex;
        }
    }
}

void Geode::OutputTriangleDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                               const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end(); ++primItr)
    {
        unsigned int localPrimLength;
        localPrimLength = 3;
        
        for(GLsizei primCount = 0; primCount < *primItr; ++primCount)
        {
            if ((primCount%localPrimLength)==0)
            {
                OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
            }
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            ++vindex;
        }
    }
}

void Geode::OutputQuadsDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                            const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end()-4; primItr+=4)
    {
        unsigned int localPrimLength;
        localPrimLength = 4;
        
        for(GLsizei primCount = 0; primCount < *primItr; ++primCount)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+2, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+3, pVertexIndices, pTexCoords, pTexIndices, fout);

            vindex+=4;
        }
    }
}

void Geode::OutputQuadStripDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end()-2; primItr+=2)
    {
        unsigned int localPrimLength;
        localPrimLength = *primItr;
        
        for(GLsizei primCount = 0; primCount < *primItr; ++primCount)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+1, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+3, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+2, pVertexIndices, pTexCoords, pTexIndices, fout);

            vindex+=2;
        }
    }
}

void Geode::OutputPolygonDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                              const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end(); ++primItr)
    {
        unsigned int localPrimLength;
        localPrimLength = *primItr;
        
        for(GLsizei primCount = 0; primCount < *primItr; ++primCount)
        {
            if ((primCount%localPrimLength)==0)
            {
                OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
            }
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            ++vindex;
        }
    }
}

void Geode::OutputTriangleStripDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                    const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end(); ++primItr)
    {
        // RFJ!!!!!!!!!! fixes for indexing
        int localPrimLength= *primItr;
        bool evenodd=true;
        
        for(GLsizei primCount = 0; primCount < localPrimLength - 2; ++primCount)
        {
            OutputSurfHead(iCurrentMaterial, surfaceFlags, 3, fout);

            if (evenodd)
            {
                OutputVertex(vindex + primCount, pVertexIndices, pTexCoords, pTexIndices, fout);
                OutputVertex(vindex + primCount + 1, pVertexIndices, pTexCoords, pTexIndices, fout);
            } else
            {
                OutputVertex(vindex + primCount + 1 , pVertexIndices, pTexCoords, pTexIndices, fout);
                OutputVertex(vindex + primCount, pVertexIndices, pTexCoords, pTexIndices, fout);
            }
            OutputVertex(vindex + primCount + 2, pVertexIndices, pTexCoords, pTexIndices, fout);
            evenodd=!evenodd;
        }
        vindex += localPrimLength;
    }
}

void Geode::OutputTriangleFanDARR(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                  const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,const osg::DrawArrayLengths* drawArrayLengths, ostream& fout)
{
    unsigned int vindex = drawArrayLengths->getFirst();

    for(osg::DrawArrayLengths::const_iterator primItr = drawArrayLengths->begin(); primItr <drawArrayLengths->end(); ++primItr)
    {
        int localPrimLength = *primItr;
        
        for(GLsizei primCount = 0; primCount < localPrimLength - 2; primCount++)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,3, fout);

            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+1+primCount, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex+2+primCount, pVertexIndices, pTexCoords, pTexIndices, fout);
        }
        vindex += localPrimLength;
    }
}

// DrawElements .... Ubyte
void Geode::OutputTriangleDelsUByte(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                    const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                    const osg::DrawElementsUByte* drawElements, ostream& fout)
{
    unsigned int primLength =3;
    unsigned int primCount = 0;

    for(osg::DrawElementsUByte::const_iterator primItr=drawElements->begin(); primItr<drawElements->end(); ++primCount,++primItr)
    {
        if ((primCount%primLength) == 0)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,primLength, fout);
        }
        
        unsigned int vindex=*primItr;
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputTriangleStripDelsUByte(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                         const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                         const osg::DrawElementsUByte* drawElements, ostream& fout)
{
    unsigned int localPrimLength = 3;
    bool evenodd=true;

    for(osg::DrawElementsUByte::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-2; ++primItr)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

        if (evenodd)
        {
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        } else
        {
            OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        }
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        evenodd=!evenodd;
    }
}

void Geode::OutputTriangleFanDelsUByte(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                       const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                       const osg::DrawElementsUByte* drawElements, ostream& fout)
{
    const unsigned int localPrimLength = 3;
    unsigned int vindex=*(drawElements->begin());

    for(osg::DrawElementsUByte::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-2; ++primItr)
    {
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadStripDelsUByte(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                     const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                     const osg::DrawElementsUByte* drawElements, ostream& fout)
{
    const unsigned int localPrimLength=4;
    for(osg::DrawElementsUByte::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-3; primItr+=2)
    {
        unsigned int vindex=*primItr;
        
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+3);
        unsigned int vindexp3=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp3, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadsDelsUByte(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                 const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                 const osg::DrawElementsUByte* drawElements, ostream& fout)
{
    const unsigned int localPrimLength=4;

    for(osg::DrawElementsUByte::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-3; primItr+=4)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);
        unsigned int vindexp3=*(primItr+3);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp3, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputPolygonDelsUByte(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                   const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                   const osg::DrawElementsUByte* drawElements, ostream& fout)
{
    unsigned int primLength =drawElements->size();
    unsigned int primCount = 0;

    OutputSurfHead(iCurrentMaterial,surfaceFlags,primLength, fout);

    for(osg::DrawElementsUByte::const_iterator primItr=drawElements->begin(); primItr<drawElements->end(); ++primCount,++primItr)
    {
        unsigned int vindex=*primItr;
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

// DrawElements .... UShort
void Geode::OutputTriangleDelsUShort(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                     const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                     const osg::DrawElementsUShort* drawElements, ostream& fout)
{
    unsigned int primLength =3;
    unsigned int primCount = 0;

    for(osg::DrawElementsUShort::const_iterator primItr=drawElements->begin(); primItr<drawElements->end(); ++primCount,++primItr)
    {
        if ((primCount%primLength) == 0)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,primLength, fout);
        }
        
        unsigned int vindex=*primItr;
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputTriangleStripDelsUShort(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                          const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                          const osg::DrawElementsUShort* drawElements, ostream& fout)
{
    unsigned int localPrimLength = 3;
    bool evenodd=true;

    for(osg::DrawElementsUShort::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-2; ++primItr)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

        if (evenodd)
        {
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        } else
        {
            OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        }
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        evenodd=!evenodd;
    }
}

void Geode::OutputTriangleFanDelsUShort(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                        const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                        const osg::DrawElementsUShort* drawElements, ostream& fout)
{
    const unsigned int localPrimLength = 3;
    unsigned int vindex=*(drawElements->begin());

    for(osg::DrawElementsUShort::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-2; ++primItr)
    {
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadStripDelsUShort(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                      const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                      const osg::DrawElementsUShort* drawElements, ostream& fout)
{
    const unsigned int localPrimLength=4;

    for(osg::DrawElementsUShort::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-3; primItr+=2)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+3);
        unsigned int vindexp3=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp3, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadsDelsUShort(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                  const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                  const osg::DrawElementsUShort* drawElements, ostream& fout)
{
    const unsigned int localPrimLength=4;

    for(osg::DrawElementsUShort::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-3; primItr+=4)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);
        unsigned int vindexp3=*(primItr+3);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp3, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputPolygonDelsUShort(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                    const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                    const osg::DrawElementsUShort* drawElements, ostream& fout)
{
    unsigned int primLength =drawElements->size();
    unsigned int primCount = 0;

    OutputSurfHead(iCurrentMaterial,surfaceFlags,primLength, fout);

    for(osg::DrawElementsUShort::const_iterator primItr=drawElements->begin(); primItr<drawElements->end(); ++primCount,++primItr)
    {
        unsigned int vindex=*primItr;
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

// DrawElements .... UInt
void Geode::OutputTriangleDelsUInt(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                   const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                   const osg::DrawElementsUInt* drawElements, ostream& fout)
{
    unsigned int primLength =3;
    unsigned int primCount = 0;

    for(osg::DrawElementsUInt::const_iterator primItr=drawElements->begin(); primItr<drawElements->end(); ++primCount,++primItr)
    {
        if ((primCount%primLength) == 0)
        {
            OutputSurfHead(iCurrentMaterial,surfaceFlags,primLength, fout);
        }
        
        unsigned int vindex=*primItr;
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputTriangleStripDelsUInt(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                        const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                        const osg::DrawElementsUInt* drawElements, ostream& fout)
{
    unsigned int localPrimLength = 3;
    bool evenodd=true;

    for(osg::DrawElementsUInt::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-2; ++primItr)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);

        if (evenodd)
        {
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        } else
        {
            OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
            OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        }
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        evenodd=!evenodd;
    }
}

void Geode::OutputTriangleFanDelsUInt(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                      const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                      const osg::DrawElementsUInt* drawElements, ostream& fout)
{
    const unsigned int localPrimLength = 3;
    unsigned int vindex=*(drawElements->begin());

    for(osg::DrawElementsUInt::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-2; ++primItr)
    {
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadStripDelsUInt(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                    const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                    const osg::DrawElementsUInt* drawElements, ostream& fout)
{
    const unsigned int localPrimLength = 4;
    for(osg::DrawElementsUInt::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-3; primItr+=2)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+3);
        unsigned int vindexp3=*(primItr+2);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp3, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputQuadsDelsUInt(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                const osg::DrawElementsUInt* drawElements, ostream& fout)
{
    const unsigned int localPrimLength=4;

    for(osg::DrawElementsUInt::const_iterator primItr=drawElements->begin(); primItr<drawElements->end()-3; primItr+=4)
    {
        unsigned int vindex=*primItr;
        unsigned int vindexp1=*(primItr+1);
        unsigned int vindexp2=*(primItr+2);
        unsigned int vindexp3=*(primItr+3);

        OutputSurfHead(iCurrentMaterial,surfaceFlags,localPrimLength, fout);
        
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp1, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp2, pVertexIndices, pTexCoords, pTexIndices, fout);
        OutputVertex(vindexp3, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

void Geode::OutputPolygonDelsUInt(const int iCurrentMaterial, const unsigned int surfaceFlags,
                                  const osg::IndexArray *pVertexIndices, const osg::Vec2 *pTexCoords, const osg::IndexArray *pTexIndices,
                                  const osg::DrawElementsUInt* drawElements, ostream& fout)
{
    unsigned int primLength =drawElements->size();
    unsigned int primCount = 0;

    OutputSurfHead(iCurrentMaterial,surfaceFlags,primLength, fout);

    for(osg::DrawElementsUInt::const_iterator primItr=drawElements->begin(); primItr<drawElements->end(); ++primCount,++primItr)
    {
        unsigned int vindex=*primItr;
        OutputVertex(vindex, pVertexIndices, pTexCoords, pTexIndices, fout);
    }
}

int Geode::ProcessMaterial(ostream& fout, const unsigned int igeode)
{
    // outputs materials from one geode
    // extended for multiple geode models, GWM 2003.
    // called before all the geometry as ac3d expects all materials in the header.
    // returns number of materials made
    unsigned int i;
    const unsigned int iNumDrawables = getNumDrawables();
    const osg::StateSet::RefAttributePair* pRAP;
    unsigned int iNumMaterials=0;
    // Let ac3d optimise the file
    // whiz round and get a list of materials
    // these may be duplicates of materials on other Geode/geometry sets.
    // Scan for materials
    for (i = 0; i < iNumDrawables; i++)
    {
        const osg::Drawable* Drawable = getDrawable(i);
        if (Drawable)
        {
            const osg::StateSet* theState = Drawable->getStateSet();
            if (theState)
            {
                pRAP = theState->getAttributePair(osg::StateAttribute::MATERIAL);
                if (NULL != pRAP)
                {
                    const osg::Material *pMaterial = dynamic_cast<const osg::Material*>(pRAP->first.get());
                    if (NULL != pMaterial)
                    {
                        const osg::Vec4& Diffuse = pMaterial->getDiffuse(osg::Material::FRONT_AND_BACK);
                        const osg::Vec4& Ambient = pMaterial->getAmbient(osg::Material::FRONT_AND_BACK);
                        const osg::Vec4& Emissive = pMaterial->getEmission(osg::Material::FRONT_AND_BACK);
                        const osg::Vec4& Specular = pMaterial->getSpecular(osg::Material::FRONT_AND_BACK);
                        fout << "MATERIAL "
                             << "\"osg"<<igeode<<"mat"<<i
                             << "\" rgb " << Diffuse[0] << " " << Diffuse[1] << " " << Diffuse[2] << " "
                             << "amb " << Ambient[0] << " " << Ambient[1] << " " << Ambient[2] << " "
                             << "emis " << Emissive[0] << " " << Emissive[1] << " " << Emissive[2] << " "
                             << "spec " << Specular[0] << " " << Specular[1] << " " << Specular[2] << " "
                             << "shi " << (int)pMaterial->getShininess(osg::Material::FRONT_AND_BACK) << " "
                             << "trans " << 1.0 - Diffuse[3] << std::endl;
                        iNumMaterials++;
                    }
                }
            }
        }
    }
    return iNumMaterials;
}
void Geode::ProcessGeometry(ostream& fout, const unsigned int ioffset)
{
}
