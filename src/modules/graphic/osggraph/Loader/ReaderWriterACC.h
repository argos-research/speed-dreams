/***************************************************************************

    file                 : ReaderWriterACC.h
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


#include <string.h>
#include <map>

#include <osgDB/FileNameUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/fstream>

#include <osg/Math>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/Material>
#include <osg/Math>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/StateSet>
#include <osg/ShadeModel>
#include <osg/Math>
#include <osg/Notify>

#include <osgUtil/Tessellator>
#include <osgUtil/SmoothingVisitor>

#include <tgf.h>

#include "AccException.h"
#include "AccGeode.h"

namespace acc3d
{
enum
{
    ObjectTypeNormal = 0,
    ObjectTypeGroup = 1,
    ObjectTypeLight = 2,

    SurfaceTypePolygon = 0,
    SurfaceTypeLineLoop = 1,
    SurfaceTypeLineStrip = 2,

    SurfaceShaded = 1<<4,
    SurfaceTwoSided = 1<<5
};
}

void setTranslucent(osg::StateSet* stateSet);
std::string readString(std::istream& stream);
void setAlphaClamp(osg::StateSet* stateSet,float clamp);
unsigned int GetUVCount(unsigned flags);

// Just a container to store an ac3d material
class MaterialData
{
public:
    MaterialData() :
        mMaterial(new osg::Material),
        mColorArray(new osg::Vec4Array(1))
    {
        mMaterial->setDataVariance(osg::Object::STATIC);
        mColorArray->setDataVariance(osg::Object::STATIC);
    }

    void readMaterial(std::istream& stream)
    {
        // note that this might be quoted
        std::string name = readString(stream);
        mMaterial->setName(name);
        std::string tmp;
        stream >> tmp;
        osg::Vec4 diffuse;
        stream >> diffuse[0] >> diffuse[1] >> diffuse[2];
        mMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
        stream >> tmp;
        osg::Vec4 ambient;
        stream >> ambient[0] >> ambient[1] >> ambient[2];
        mMaterial->setAmbient(osg::Material::FRONT_AND_BACK, ambient);
        stream >> tmp;
        osg::Vec4 emmissive;
        stream >> emmissive[0] >> emmissive[1] >> emmissive[2];
        mMaterial->setEmission(osg::Material::FRONT_AND_BACK, emmissive);
        stream >> tmp;
        osg::Vec4 specular;
        stream >> specular[0] >> specular[1] >> specular[2];
        mMaterial->setSpecular(osg::Material::FRONT_AND_BACK, specular);
        stream >> tmp;
        float shininess;
        stream >> shininess;
        mMaterial->setShininess(osg::Material::FRONT_AND_BACK, shininess);
        stream >> tmp;
        float transparency;
        stream >> transparency;
        mMaterial->setTransparency(osg::Material::FRONT_AND_BACK, transparency);
        mTranslucent = 0 < transparency;

        // must correspond to the material we use for the color array below
        mMaterial->setColorMode(osg::Material::DIFFUSE);
        // this must be done past the transparency setting ...
        (*mColorArray)[0] = mMaterial->getDiffuse(osg::Material::FRONT_AND_BACK);
    }

    void toStateSet(osg::StateSet* stateSet) const
    {
        stateSet->setAttribute(mMaterial.get());
        if (mTranslucent)
            setTranslucent(stateSet);
    }

    osg::Vec4Array* getColorArray() const
    {
        return mColorArray.get();
    }

private:
    osg::ref_ptr<osg::Material> mMaterial;
    osg::ref_ptr<osg::Vec4Array> mColorArray;
    bool mTranslucent;
};

class TextureData
{

private:
    osg::ref_ptr<osg::TexEnv> mModulateTexEnv;
    osg::ref_ptr<osg::Texture2D> mTexture2DClamp;
    osg::ref_ptr<osg::Texture2D> mTexture2DRepeat;
    osg::ref_ptr<osg::Texture2D> mTexture2DRepeat1;
    osg::ref_ptr<osg::Texture2D> mTexture2DClamp1;

    osg::ref_ptr<osg::Texture2D> mTexture2DRepeat2;
    osg::ref_ptr<osg::Texture2D> mTexture2DClamp2;
    osg::ref_ptr<osg::Texture2D> mTexture2DRepeat3;
    osg::ref_ptr<osg::Texture2D> mTexture2DClamp3;

    osg::ref_ptr<osg::Image> mImage;
    osg::ref_ptr<osg::Image> mImage1;
    osg::ref_ptr<osg::Image> mImage2;
    osg::ref_ptr<osg::Image> mImage3;

    bool mTranslucent;
    bool mRepeat;
    float mAlphaClamp;
    bool mCar;

public:
    typedef std::map<std::string, osg::ref_ptr<osg::Image> > TextureImageMap;
    static TextureImageMap mTextureImageMap;
    TextureData() :
        mTranslucent(false),
        mRepeat(true),
        mAlphaClamp(0.0f)
    {
    }

    void SetBlockTransparent(bool bStatus) { mTranslucent = bStatus;}

    bool setTexture(const std::string& name, const std::string& name1, const std::string& name2, const std::string& name3, const osgDB::ReaderWriter::Options* options, osg::TexEnv* modulateTexEnv,const bool bBlockTransparent)
    {
        mTexture2DRepeat = new osg::Texture2D;
        mTexture2DRepeat->setDataVariance(osg::Object::STATIC);
        mTexture2DRepeat->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        mTexture2DRepeat->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        mTexture2DRepeat->setMaxAnisotropy(16);

        mTexture2DClamp = new osg::Texture2D;
        mTexture2DClamp->setDataVariance(osg::Object::STATIC);
        mTexture2DClamp->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
        mTexture2DClamp->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
        mTexture2DClamp->setMaxAnisotropy(16);

        std::string absFileName = osgDB::findDataFile(name, options);

        if (absFileName.empty())
        {
            osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: could not find texture \"" << name << "\"" << std::endl;
            return false;
        }

        TextureData::TextureImageMap::iterator i = TextureData::mTextureImageMap.find(absFileName);

        if (i == TextureData::mTextureImageMap.end())
        {
            mImage = osgDB::readRefImageFile(absFileName, options);
            GfLogDebug("Loaded image %s \n", absFileName.c_str());

            if (!mImage.valid())
            {
                osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: could not read texture \"" << name << "\"" << std::endl;
                return false;
            }

            TextureData::mTextureImageMap[absFileName] = mImage;
        }
        else
        {
            mImage = TextureData::mTextureImageMap[absFileName];
            GfLogDebug("Take in Cache image %s \n", absFileName.c_str());
        }

        mTexture2DRepeat->setImage(mImage.get());
        mTexture2DClamp->setImage(mImage.get());
        mTranslucent = mImage->isImageTranslucent();

        if (bBlockTransparent)
            mTranslucent = false;

        // Use a shared modulate TexEnv
        mModulateTexEnv = modulateTexEnv;

        if (name1!="")
        {
            std::string absFileName1 = osgDB::findDataFile(name1, options);
            TextureData::TextureImageMap::iterator i = TextureData::mTextureImageMap.find(absFileName1);

            if (i == TextureData::mTextureImageMap.end())
            {
                mImage1 = osgDB::readRefImageFile(absFileName1, options);
                GfLogDebug("Loaded image 1 %s \n", absFileName1.c_str());

                if (!mImage1.valid())
                {
                    //osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: could not read texture \"" << name1 << "\"" << std::endl;
                    return false;
                }

                TextureData::mTextureImageMap[absFileName1] = mImage1;
            }
            else
            {
                mImage1 = TextureData::mTextureImageMap[absFileName1];
                GfLogDebug("Take in Cache image1 %s \n", absFileName1.c_str());
            }

            mTexture2DRepeat1 = new osg::Texture2D;
            mTexture2DRepeat1->setDataVariance(osg::Object::STATIC);
            mTexture2DRepeat1->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
            mTexture2DRepeat1->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
            mTexture2DRepeat1->setMaxAnisotropy(16);
            mTexture2DRepeat1->setImage(mImage1.get());

            mTexture2DClamp1 = new osg::Texture2D;
            mTexture2DClamp1->setDataVariance(osg::Object::STATIC);
            mTexture2DClamp1->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
            mTexture2DClamp1->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
            mTexture2DClamp1->setMaxAnisotropy(16);
            mTexture2DClamp1->setImage(mImage1.get());
        }

        if (name2!="")
        {
            std::string absFileName2 = osgDB::findDataFile(name2, options);
            TextureData::TextureImageMap::iterator i = TextureData::mTextureImageMap.find(absFileName2);

            if (i == TextureData::mTextureImageMap.end())
            {
                mImage2 = osgDB::readRefImageFile(absFileName2, options);
                GfLogDebug("Loaded image 2 %s \n", absFileName2.c_str());

                if (!mImage2.valid())
                {
                    //osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: could not read texture \"" << name2 << "\"" << std::endl;
                    return false;
                }

                TextureData::mTextureImageMap[absFileName2] = mImage2;
            }
            else
            {
                mImage2 = TextureData::mTextureImageMap[absFileName2];
                GfLogDebug("Take in Cache image2 %s \n", absFileName2.c_str());
            }

            mTexture2DRepeat2 = new osg::Texture2D;
            mTexture2DRepeat2->setDataVariance(osg::Object::STATIC);
            mTexture2DRepeat2->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
            mTexture2DRepeat2->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
            mTexture2DRepeat2->setMaxAnisotropy(16);
            mTexture2DRepeat2->setImage(mImage2.get());

            mTexture2DClamp2 = new osg::Texture2D;
            mTexture2DClamp2->setDataVariance(osg::Object::STATIC);
            mTexture2DClamp2->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
            mTexture2DClamp2->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
            mTexture2DClamp2->setMaxAnisotropy(16);
            mTexture2DClamp2->setImage(mImage2.get());
        }

        if (name3!="")
        {
            std::string absFileName3 = osgDB::findDataFile(name3, options);
            TextureData::TextureImageMap::iterator i = TextureData::mTextureImageMap.find(absFileName3);

            if (i == TextureData::mTextureImageMap.end())
            {
                mImage3 = osgDB::readRefImageFile(absFileName3, options);
                GfLogDebug("Loaded  image3 %s \n", absFileName3.c_str());
                if (!mImage3.valid())
                {
                    //osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: could not read texture \"" << name3 << "\"" << std::endl;
                    return false;
                }

                TextureData::mTextureImageMap[absFileName3] = mImage3;
            }
            else
            {
                mImage3 = TextureData::mTextureImageMap[absFileName3];
                GfLogDebug("Take in Cache image3 %s \n", absFileName3.c_str());
            }

            mTexture2DRepeat3 = new osg::Texture2D;
            mTexture2DRepeat3->setDataVariance(osg::Object::STATIC);
            mTexture2DRepeat3->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
            mTexture2DRepeat3->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
            mTexture2DRepeat3->setMaxAnisotropy(16);
            mTexture2DRepeat3->setImage(mImage3.get());

            mTexture2DClamp3 = new osg::Texture2D;
            mTexture2DClamp3->setDataVariance(osg::Object::STATIC);
            mTexture2DClamp3->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
            mTexture2DClamp3->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
            mTexture2DClamp3->setMaxAnisotropy(16);
            mTexture2DClamp3->setImage(mImage3.get());
        }

        if (strstr(name.c_str(),"tree")!=NULL || strstr(name.c_str(),"trans-")!=NULL || strstr(name.c_str(),"arbor")!=NULL)
        {
            mAlphaClamp=0.65f;
        }
        return true;
    }

    void setRepeat(bool repeat)
    {
        mRepeat = repeat;
    }

    bool valid() const
    {
        return mImage.valid();
    }

    std::string getFileName() const
    {
        if (!mImage.valid())
            return std::string();
        return mImage->getFileName();
    }

    void toTextureStateSet(osg::StateSet* stateSet) const
    {
        if (!valid())
            return;

        stateSet->setTextureAttribute(0, mModulateTexEnv.get());

        if (mRepeat)
            stateSet->setTextureAttribute(0, mTexture2DRepeat.get());
        else
            stateSet->setTextureAttribute(0, mTexture2DClamp.get());

        stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);

        if(mTexture2DRepeat1.valid())
        {
            if (mRepeat)
                stateSet->setTextureAttribute(1, mTexture2DRepeat1.get());
            else
                stateSet->setTextureAttribute(1, mTexture2DClamp1.get());

            stateSet->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);
        }

        if(mTexture2DRepeat2.valid())
        {
            if (mRepeat)
                stateSet->setTextureAttribute(2, mTexture2DRepeat2.get());
            else
                stateSet->setTextureAttribute(2, mTexture2DClamp2.get());

            stateSet->setTextureMode(2, GL_TEXTURE_2D, osg::StateAttribute::ON);
        }

        if(mTexture2DRepeat3.valid())
        {
            if (mRepeat)
                stateSet->setTextureAttribute(3, mTexture2DRepeat3.get());
            else
                stateSet->setTextureAttribute(3, mTexture2DClamp3.get());

            stateSet->setTextureMode(3, GL_TEXTURE_2D, osg::StateAttribute::ON);
        }


        if (mTranslucent)
            setTranslucent(stateSet);

        if (mAlphaClamp>0.0f)
            setAlphaClamp(stateSet,mAlphaClamp);
    }
};

class FileData
{
public:
    FileData(const osgDB::ReaderWriter::Options* options) : mOptions(options), mLightIndex(1)
    {
        mModulateTexEnv = new osg::TexEnv;
        mModulateTexEnv->setDataVariance(osg::Object::STATIC);
        mModulateTexEnv->setMode(osg::TexEnv::MODULATE);
    }

    TextureData toTextureData(const std::string& texName0, const std::string& texName1, const std::string& texName2, const std::string& texName3, const bool bBlockTransparent)
    {
        std::string strTrans = "_NO";
        if (bBlockTransparent)
            strTrans = "_YES";

        std::string texName = texName0+texName1+texName2+texName3+strTrans;
        TextureDataMap::iterator i = mTextureStates.find(texName);
        if (i == mTextureStates.end())
            mTextureStates[texName].setTexture(texName0, texName1, texName2, texName3, mOptions.get(), mModulateTexEnv.get(), bBlockTransparent);

        return mTextureStates[texName];
    }

    osg::Light* getNextLight()
    {
        osg::Light* light = new osg::Light;
        light->setDataVariance(osg::Object::STATIC);
        light->setLightNum(mLightIndex++);
        return light;
    }

    void addMaterial(const MaterialData& material)
    {
        mMaterials.push_back(material);
    }
    unsigned getNumMaterials() const
    {
        return mMaterials.size();
    }
    const MaterialData& getMaterial(unsigned idx) const
    {
        return mMaterials[idx];
    }

private:
    /// Stores the acc3d file reader options, only used for reading texture files
    osg::ref_ptr<osgDB::ReaderWriter::Options const> mOptions;

    /// The list of acc3d MATERIALS
    std::vector<MaterialData> mMaterials;

    /// Local per model texture attribute cache.
    /// ... images are usualy cached in the registries object cache
    typedef std::map<std::string, TextureData> TextureDataMap;
    TextureDataMap mTextureStates;
    /// A common shared TexEnv set to modulate
    osg::ref_ptr<osg::TexEnv> mModulateTexEnv;

    /// Hack to include light nodes from acc3d into the scenegraph
    unsigned mLightIndex;
};

struct RefData 
{
    RefData(const osg::Vec2& _texCoord, const osg::Vec2& _texCoord1, const osg::Vec2& _texCoord2, const osg::Vec2& _texCoord3)
    {
        texCoord  = _texCoord;
        texCoord1 = _texCoord1;
        texCoord2 = _texCoord2;
        texCoord3 = _texCoord3;
    }

    osg::Vec2 texCoord;
    osg::Vec2 texCoord1;
    osg::Vec2 texCoord2;
    osg::Vec2 texCoord3;
    osg::Vec3 normal;
};

struct VertexData 
{
    VertexData(const osg::Vec3& vertex) : _vertex(vertex) {}
    unsigned addRefData(const RefData& refData)
    {
        unsigned index = _refs.size();
        _refs.push_back(refData);
        return index;
    }

    osg::Vec3 _vertex;
    std::vector<RefData> _refs;
};

struct VertexIndex 
{
    VertexIndex(unsigned _vertexIndex = 0, unsigned _refIndex = 0) :
        vertexIndex(_vertexIndex), refIndex(_refIndex)
    {}
    unsigned vertexIndex;
    unsigned refIndex;
};

class VertexSet : public osg::Referenced 
{
public:
    VertexSet() : _dirty(true)
    {}

    void reserve(unsigned n)
    {
        _vertices.reserve(n);
    }

    unsigned size() const
    {
        return _vertices.size();
    }
    
    void addVertex(const osg::Vec3& vertex)
    {
        _dirty = true;
        _vertices.push_back(vertex);
    }

    const osg::Vec3& getVertex(unsigned index)
    {
        return _vertices[index]._vertex;
    }

    const osg::Vec3& getVertex(const VertexIndex& vertexIndex)
    {
        return _vertices[vertexIndex.vertexIndex]._vertex;
    }

    const osg::Vec3& getNormal(const VertexIndex& vertexIndex)
    {
        return _vertices[vertexIndex.vertexIndex]._refs[vertexIndex.refIndex].normal;
    }

    const osg::Vec2& getTexCoord(const VertexIndex& vertexIndex)
    {
        return _vertices[vertexIndex.vertexIndex]._refs[vertexIndex.refIndex].texCoord;
    }

    const osg::Vec2& getTexCoord1(const VertexIndex& vertexIndex)
    {
        return _vertices[vertexIndex.vertexIndex]._refs[vertexIndex.refIndex].texCoord1;
    }

    const osg::Vec2& getTexCoord2(const VertexIndex& vertexIndex)
    {
        return _vertices[vertexIndex.vertexIndex]._refs[vertexIndex.refIndex].texCoord2;
    }

    const osg::Vec2& getTexCoord3(const VertexIndex& vertexIndex)
    {
        return _vertices[vertexIndex.vertexIndex]._refs[vertexIndex.refIndex].texCoord3;
    }


    VertexIndex addRefData(unsigned i, const RefData& refData)
    {
        if (_vertices.size() <= i)
        {
            osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: internal error, got invalid vertex index!" << std::endl;
            return VertexIndex(0, 0);
        }
        return VertexIndex(i, _vertices[i].addRefData(refData));
    }

private:
    std::vector<VertexData> _vertices;
    bool _dirty;
};

class PrimitiveBin : public osg::Referenced
{
public:
    PrimitiveBin(unsigned flags, VertexSet* vertexSet) :
        _geode(new osg::Geode),
        _vertexSet(vertexSet),
        _flags(flags)
    {
        _geode->setDataVariance(osg::Object::STATIC);
    }

    virtual bool beginPrimitive(unsigned nRefs) = 0;
    virtual bool vertex(unsigned vertexIndex, const osg::Vec2& texCoord, const osg::Vec2& texCoord1, const osg::Vec2& texCoord2, const osg::Vec2& texCoord3) = 0;
    virtual bool endPrimitive() = 0;
    virtual osg::Geode* finalize(const MaterialData& material, const TextureData& textureData) = 0;

protected:
    bool isLineLoop() const
    {
        return (_flags & acc3d::SurfaceTypeLineLoop)!=0;
    }

    bool isLineStrip() const
    {
        return (_flags & acc3d::SurfaceTypeLineStrip)!=0;
    }

    bool isTwoSided() const
    {
        return (_flags & acc3d::SurfaceTwoSided)!=0;
        //return true;
    }

    bool isSmooth() const
    {
        return (_flags & acc3d::SurfaceShaded)!=0;
    }

    bool isTriangleStrip() const
    {
        if (_flags == 0x14)
            return true;

        if(_flags == 0x24)
            return true;

        if (_flags == 0x34)
            return true;

        if (_flags == 0x44)
            return true;

        return false;
    }

    bool isTriangleFan() const
    {
        if (_flags == 0x30)
            return true;

        return false;
    }

    osg::ref_ptr<osg::Geode> _geode;
    osg::ref_ptr<VertexSet> _vertexSet;

private:
    unsigned _flags;
};

class LineBin : public PrimitiveBin
{
private:
    osg::ref_ptr<osg::Geometry> _geometry;
    osg::ref_ptr<osg::Vec3Array> _vertices;
    osg::ref_ptr<osg::Vec2Array> _texCoords;
    osg::ref_ptr<osg::Vec2Array> _texCoords1;
    osg::ref_ptr<osg::Vec2Array> _texCoords2;
    osg::ref_ptr<osg::Vec2Array> _texCoords3;

    struct Ref
    {
        osg::Vec2 texCoord;
        osg::Vec2 texCoord1;
        osg::Vec2 texCoord2;
        osg::Vec2 texCoord3;

        unsigned index;
    };
    std::vector<Ref> _refs;

public:
    LineBin(unsigned flags, VertexSet* vertexSet) :
        PrimitiveBin(flags, vertexSet),
        _geometry(new osg::Geometry),
        _vertices(new osg::Vec3Array),
        _texCoords(new osg::Vec2Array),
        _texCoords1(new osg::Vec2Array),
        _texCoords2(new osg::Vec2Array),
        _texCoords3(new osg::Vec2Array)
    {
        _geometry->setDataVariance(osg::Object::STATIC);
        _vertices->setDataVariance(osg::Object::STATIC);
        _texCoords->setDataVariance(osg::Object::STATIC);
        _texCoords1->setDataVariance(osg::Object::STATIC);
        _texCoords2->setDataVariance(osg::Object::STATIC);
        _texCoords3->setDataVariance(osg::Object::STATIC);
        _geometry->setVertexArray(_vertices.get());
        _geometry->setTexCoordArray(0, _texCoords.get());
        _geometry->setTexCoordArray(1, _texCoords1.get());
        _geometry->setTexCoordArray(2, _texCoords2.get());
        _geometry->setTexCoordArray(3, _texCoords3.get());

        osg::StateSet* stateSet = _geode->getOrCreateStateSet();
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    }

    virtual bool beginPrimitive(unsigned nRefs)
    {
        // Check if we have enough for a line or someting broken ...
        if (nRefs < 2)
        {
            osg::notify(osg::WARN) << "osgDB SPEED DREAMS reader: detected line with less than 2 vertices!" << std::endl;
            return false;
        }

        _refs.reserve(nRefs);
        _refs.resize(0);
        return true;
    }

    virtual bool vertex(unsigned vertexIndex, const osg::Vec2& texCoord, const osg::Vec2& texCoord1, const osg::Vec2& texCoord2, const osg::Vec2& texCoord3)
    {
        Ref ref;
        ref.index = vertexIndex;
        ref.texCoord = texCoord;
        ref.texCoord1 = texCoord1;
        ref.texCoord2 = texCoord2;
        ref.texCoord3 = texCoord3;
        _refs.push_back(ref);
        return true;
    }

    virtual bool endPrimitive()
    {
        GLint type;
        if (isLineLoop())
            type = osg::PrimitiveSet::LINE_LOOP;
        else if (isLineStrip())
            type = osg::PrimitiveSet::LINE_STRIP;
        else
        {
            osg::notify(osg::FATAL) << "osgDB SPEED DREAMS reader: non surface flags in surface bin!" << std::endl;
            return false;
        }
        unsigned nRefs = _refs.size();
        unsigned start = _vertices->size();

        for (unsigned i = 0; i < nRefs; ++i)
        {
            osg::Vec3 vertex = _vertexSet->getVertex(_refs[i].index);
            _vertices->push_back(vertex);
            _texCoords->push_back(_refs[i].texCoord);
            _texCoords1->push_back(_refs[i].texCoord1);
            _texCoords2->push_back(_refs[i].texCoord2);
            _texCoords3->push_back(_refs[i].texCoord3);
        }
        _geometry->addPrimitiveSet(new osg::DrawArrays(type, start, nRefs));

        return true;
    }

    virtual osg::Geode* finalize(const MaterialData& material, const TextureData& textureData)
    {
        _geode->addDrawable(_geometry.get());
        material.toStateSet(_geode->getOrCreateStateSet());
        _geometry->setColorArray(material.getColorArray());
        _geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        _geometry->setNormalBinding(osg::Geometry::BIND_OFF);
        return _geode.get();
    }
};

class SurfaceBin : public PrimitiveBin 
{
private:
    struct Ref
    {
        osg::Vec2 texCoord;
        osg::Vec2 texCoord1;
        osg::Vec2 texCoord2;
        osg::Vec2 texCoord3;
        unsigned index;
    };
    std::vector<Ref> _refs;

    struct TriangleData
    {
        VertexIndex index[3];
    };
    std::vector<TriangleData> _triangles;

    std::vector<std::vector<VertexIndex> > _trianglestrips;
    //std::vector<VertexIndex> _trianglestrip;

    struct QuadData
    {
        VertexIndex index[4];
    };

    std::vector<QuadData> _quads;

    struct PolygonData
    {
        std::vector<VertexIndex> index;
    };

    std::vector<PolygonData> _polygons;
    std::vector<PolygonData> _toTessellatePolygons;

public:
    SurfaceBin(unsigned flags, VertexSet *vertexSet) :
        PrimitiveBin(flags, vertexSet)
    {}

    virtual bool beginPrimitive(unsigned nRefs)
    {
        _refs.reserve(nRefs);
        _refs.clear();

        // Check if we have enough for a line or someting broken ...
        if (nRefs < 3)
        {
            osg::notify(osg::WARN) << "osgDB SPEED DREAMS reader: detected surface with less than 3 vertices!" << std::endl;
            return false;
        }
        return true;
    }

    virtual bool vertex(unsigned vertexIndex, const osg::Vec2& texCoord, const osg::Vec2& texCoord1, const osg::Vec2& texCoord2, const osg::Vec2& texCoord3)
    {
        Ref ref;
        ref.index = vertexIndex;
        ref.texCoord = texCoord;
        ref.texCoord1 = texCoord1;
        ref.texCoord2 = texCoord2;
        ref.texCoord3 = texCoord3;
        _refs.push_back(ref);

        return true;
    }

    virtual bool endPrimitive()
    {
        unsigned nRefs = _refs.size();

        // Compute the normal times the enclosed area.
        // During that check if the surface is convex. If so, put in the surface as such.
        bool needTessellation = true;
        osg::Vec3 prevEdgeNormal;
        osg::Vec3 weightedNormal(0, 0, 0);
        osg::Vec3 v0 = _vertexSet->getVertex(_refs[0].index);

        for (unsigned i = 2; i < nRefs; ++i)
        {
            osg::Vec3 side1 = _vertexSet->getVertex(_refs[i-1].index) - v0;
            osg::Vec3 side2 = _vertexSet->getVertex(_refs[i].index) - v0;
            osg::Vec3 newNormal = side1^side2;
            if (!needTessellation)
            {
                if (3 < nRefs && newNormal*weightedNormal < 0)
                {
                    needTessellation = true;
                }
                if (i < 3)
                {
                    prevEdgeNormal = newNormal;
                }
                else // if (3 <= i) // due to the for loop
                {
                    osg::Vec3 sideim1 = _vertexSet->getVertex(_refs[i-1].index) - _vertexSet->getVertex(_refs[i-2].index);
                    osg::Vec3 sidei = _vertexSet->getVertex(_refs[i].index) - _vertexSet->getVertex(_refs[i-2].index);
                    osg::Vec3 edgeNormal = sideim1^sidei;
                    if (edgeNormal*prevEdgeNormal < 0)
                    {
                        needTessellation = true;
                    }
                    prevEdgeNormal = edgeNormal;
                }
            }

            weightedNormal += newNormal;
        }
        
        if (needTessellation)
        {
            if (isTriangleStrip())
            {
                int index = _trianglestrips.size();
                _trianglestrips.resize(index+1);
                for (unsigned i = 0; i < nRefs; ++i)
                {
                    RefData refData(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                    VertexIndex vertexIndex = _vertexSet->addRefData(_refs[i].index, refData);
                    _trianglestrips[index].push_back( vertexIndex );
                }
            }
            else if (isTriangleFan())
            {
                //Convert fan to triangles
                int i = 0;
                int tricount=nRefs-2;

                RefData refData(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                VertexIndex vertexIndex = _vertexSet->addRefData(_refs[i].index, refData);
                i++;

                while (i<tricount)
                {
                    TriangleData tri;

                    RefData refData1(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                    VertexIndex vertexIndex1 = _vertexSet->addRefData(_refs[i].index, refData1);

                    RefData refData2(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                    VertexIndex vertexIndex2 = _vertexSet->addRefData(_refs[i+1].index, refData2);

                    tri.index[0] = vertexIndex;
                    tri.index[1] = vertexIndex1;
                    tri.index[2] = vertexIndex2;

                    if ((tri.index[0].vertexIndex!=tri.index[1].vertexIndex)&&(tri.index[1].vertexIndex!=tri.index[2].vertexIndex)&&(tri.index[0].vertexIndex!=tri.index[2].vertexIndex))
                        _triangles.push_back(tri);

                    i++;
                }
            }
            else
            {
                unsigned polygonIndex = _toTessellatePolygons.size();
                _toTessellatePolygons.resize(polygonIndex + 1);
                for (unsigned i = 0; i < nRefs; ++i)
                {
                    RefData refData(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                    VertexIndex vertexIndex = _vertexSet->addRefData(_refs[i].index, refData);
                    _toTessellatePolygons[polygonIndex].index.push_back(vertexIndex);
                }
            }
        }
        else if (nRefs == 3)
        {
            unsigned triangleIndex = _triangles.size();
            _triangles.resize(triangleIndex + 1);
            for (unsigned i = 0; i < 3; ++i)
            {
                RefData refData(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                VertexIndex vertexIndex = _vertexSet->addRefData(_refs[i].index, refData);
                _triangles[triangleIndex].index[i] = vertexIndex;
            }
        }
        else if (nRefs == 4)
        {
            unsigned quadIndex = _quads.size();
            _quads.resize(quadIndex + 1);
            for (unsigned i = 0; i < 4; ++i)
            {
                RefData refData(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                VertexIndex vertexIndex = _vertexSet->addRefData(_refs[i].index, refData);
                _quads[quadIndex].index[i] = vertexIndex;
            }
        }
        else
        {
            unsigned polygonIndex = _polygons.size();
            _polygons.resize(polygonIndex + 1);
            for (unsigned i = 0; i < nRefs; ++i) {
                RefData refData(_refs[i].texCoord, _refs[i].texCoord1, _refs[i].texCoord2,  _refs[i].texCoord3);
                VertexIndex vertexIndex = _vertexSet->addRefData(_refs[i].index, refData);
                _polygons[polygonIndex].index.push_back(vertexIndex);
            }
        }
        return true;
    }

    void pushVertex(const VertexIndex& vertexIndex, osg::Vec3Array* vertexArray,
                    osg::Vec3Array* normalArray, osg::Vec2Array* texcoordArray, osg::Vec2Array* texcoordArray1, osg::Vec2Array* texcoordArray2, osg::Vec2Array* texcoordArray3)
    {
        vertexArray->push_back(_vertexSet->getVertex(vertexIndex));
        normalArray->push_back(_vertexSet->getNormal(vertexIndex));
        if (texcoordArray)
            texcoordArray->push_back(_vertexSet->getTexCoord(vertexIndex));
        if (texcoordArray1)
            texcoordArray1->push_back(_vertexSet->getTexCoord1(vertexIndex));
        if (texcoordArray2)
            texcoordArray2->push_back(_vertexSet->getTexCoord2(vertexIndex));
        if (texcoordArray3)
            texcoordArray3->push_back(_vertexSet->getTexCoord3(vertexIndex));

    }

    virtual osg::Geode* finalize(const MaterialData& material, const TextureData& textureData)
    {
        osg::StateSet* stateSet = _geode->getOrCreateStateSet();
        material.toStateSet(stateSet);
        textureData.toTextureStateSet(stateSet);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);

        // Single- or doublesided culling
        if (isTwoSided())
        {
            stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        } else
        {
            osg::CullFace* cullFace = new osg::CullFace;
            cullFace->setDataVariance(osg::Object::STATIC);
            cullFace->setMode(osg::CullFace::BACK);
            stateSet->setAttribute(cullFace);
            stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        }

        // Flat or smooth shading
        osg::ShadeModel* shadeModel = new osg::ShadeModel;
        shadeModel->setDataVariance(osg::Object::STATIC);
        shadeModel->setMode(osg::ShadeModel::SMOOTH);
        stateSet->setAttribute(shadeModel);
        
        // Set up the arrays, allways store texture coords, may be we need them later ...
        osg::Geometry* geometry = new osg::Geometry;
        _geode->addDrawable(geometry);
        geometry->setDataVariance(osg::Object::STATIC);
        geometry->setColorArray(material.getColorArray());
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        osg::Vec3Array* normalArray = new osg::Vec3Array;
        normalArray->setDataVariance(osg::Object::STATIC);
        geometry->setNormalArray(normalArray);
        osg::Vec3Array* vertexArray = new osg::Vec3Array;
        vertexArray->setDataVariance(osg::Object::STATIC);
        geometry->setVertexArray(vertexArray);
        osg::Vec2Array* texcoordArray = 0;
        osg::Vec2Array* texcoordArray1 = 0;
        osg::Vec2Array* texcoordArray2 = 0;
        osg::Vec2Array* texcoordArray3 = 0;

        if (textureData.valid())
        {
            texcoordArray = new osg::Vec2Array;
            texcoordArray->setDataVariance(osg::Object::STATIC);
            geometry->setTexCoordArray(0, texcoordArray);

            texcoordArray1 = new osg::Vec2Array;
            texcoordArray1->setDataVariance(osg::Object::STATIC);
            geometry->setTexCoordArray(1, texcoordArray1);

            texcoordArray2 = new osg::Vec2Array;
            texcoordArray2->setDataVariance(osg::Object::STATIC);
            geometry->setTexCoordArray(2, texcoordArray2);

            texcoordArray3 = new osg::Vec2Array;
            texcoordArray3->setDataVariance(osg::Object::STATIC);
            geometry->setTexCoordArray(3, texcoordArray3);
        }

        // At first handle the the polygons to tessellate, fix them and append the other polygons later
        if (!_toTessellatePolygons.empty())
        {
            unsigned start = vertexArray->size();
            osg::DrawArrayLengths* drawArrayLengths = new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON, start);
            drawArrayLengths->reserve(_toTessellatePolygons.size());
            for (unsigned i = 0; i < _toTessellatePolygons.size(); ++i)
            {
                for (unsigned j = 0; j < _toTessellatePolygons[i].index.size(); ++j)
                {
                    pushVertex(_toTessellatePolygons[i].index[j], vertexArray, normalArray, texcoordArray, texcoordArray1, texcoordArray2, texcoordArray3);
                }
                drawArrayLengths->push_back(_toTessellatePolygons[i].index.size());
            }
            geometry->addPrimitiveSet(drawArrayLengths);
            osgUtil::Tessellator Tessellator;
            Tessellator.retessellatePolygons(*geometry);
        }

        // handle triangles
        if (!_triangles.empty())
        {
            unsigned start = vertexArray->size();
            for (unsigned i = 0; i < _triangles.size(); ++i)
            {
                for (unsigned j = 0; j < 3; ++j)
                {
                    pushVertex(_triangles[i].index[j], vertexArray, normalArray, texcoordArray, texcoordArray1, texcoordArray2, texcoordArray3);
                }
            }
            osg::DrawArrays* drawArray = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, start, 3*_triangles.size());
            geometry->addPrimitiveSet(drawArray);
        }

        // handle quads
        if (!_quads.empty())
        {
            unsigned start = vertexArray->size();
            for (unsigned i = 0; i < _quads.size(); ++i)
            {
                for (unsigned j = 0; j < 4; ++j)
                {
                    pushVertex(_quads[i].index[j], vertexArray, normalArray, texcoordArray, texcoordArray1, texcoordArray2, texcoordArray3);
                }
            }
            osg::DrawArrays* drawArray = new osg::DrawArrays(osg::PrimitiveSet::QUADS, start, 4*_quads.size());
            geometry->addPrimitiveSet(drawArray);
        }

        // handle polygons
        if (!_polygons.empty())
        {
            unsigned start = vertexArray->size();
            osg::DrawArrayLengths* drawArrayLengths = new osg::DrawArrayLengths(osg::PrimitiveSet::POLYGON, start);
            drawArrayLengths->reserve(_polygons.size());

            for (unsigned i = 0; i < _polygons.size(); ++i)
            {
                for (unsigned j = 0; j < _polygons[i].index.size(); ++j)
                {
                    pushVertex(_polygons[i].index[j], vertexArray, normalArray, texcoordArray, texcoordArray1, texcoordArray2, texcoordArray3);
                }
                drawArrayLengths->push_back(_polygons[i].index.size());
            }
            geometry->addPrimitiveSet(drawArrayLengths);
        }

        //handle triangle strips
        if (!_trianglestrips.empty())
        {
            for (unsigned j=0;j<_trianglestrips.size();j++)
            {
                unsigned start = vertexArray->size();
                for (unsigned i = 0; i < _trianglestrips[j].size(); ++i)
                {
                    pushVertex(_trianglestrips[j][i], vertexArray, normalArray, texcoordArray, texcoordArray1, texcoordArray2, texcoordArray3);
                }

                osg::DrawArrays* drawArray = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, start, _trianglestrips[j].size());
                geometry->addPrimitiveSet(drawArray);
            }
        }

        osgUtil::SmoothingVisitor v;
        v.smooth(*geometry);
        return _geode.get();
    }
};

struct Bins
{
    PrimitiveBin* getOrCreatePrimitiveBin(unsigned flags, VertexSet* vertexSet)
    {
        if ((flags & acc3d::SurfaceTypeLineLoop) || (flags & acc3d::SurfaceTypeLineStrip))
        {
            if (!lineBin.valid())
            {
                lineBin = new LineBin(flags, vertexSet);
            }
            return lineBin.get();
        }
        else if (flags & acc3d::SurfaceShaded)
        {
            if (flags & acc3d::SurfaceTwoSided)
            {
                if (!smoothDoubleSurfaceBin.valid())
                {
                    smoothDoubleSurfaceBin = new SurfaceBin(flags, vertexSet);
                }
                return smoothDoubleSurfaceBin.get();
            }
            else
            {
                if (!smoothSingleSurfaceBin.valid())
                {
                    smoothSingleSurfaceBin = new SurfaceBin(flags, vertexSet);
                }
                return smoothSingleSurfaceBin.get();
            }
        }
        else
        {
            if (flags & acc3d::SurfaceTwoSided)
            {
                if (!flatDoubleSurfaceBin.valid())
                {
                    flatDoubleSurfaceBin = new SurfaceBin(flags, vertexSet);
                }
                return flatDoubleSurfaceBin.get();
            }
            else
            {
                if (!flatSingleSurfaceBin.valid())
                {
                    flatSingleSurfaceBin = new SurfaceBin(flags, vertexSet);
                }
                return flatSingleSurfaceBin.get();
            }
        }
    }

    void finalize(osg::Group* group, const MaterialData& material, const TextureData& textureData)
    {
        if (lineBin.valid())
        {
            group->addChild(lineBin->finalize(material, textureData));
        }
        if (smoothDoubleSurfaceBin.valid())
        {
            group->addChild(smoothDoubleSurfaceBin->finalize(material, textureData));
        }
        if (smoothSingleSurfaceBin.valid())
        {
            osg::Geode *pGeode = smoothSingleSurfaceBin->finalize(material, textureData);
            group->addChild(pGeode);
        }
        if (flatDoubleSurfaceBin.valid())
        {
            group->addChild(flatDoubleSurfaceBin->finalize(material, textureData));
        }
        if (flatSingleSurfaceBin.valid())
        {
            group->addChild(flatSingleSurfaceBin->finalize(material, textureData));
        }
    }

private:
    osg::ref_ptr<LineBin> lineBin;
    osg::ref_ptr<SurfaceBin> flatDoubleSurfaceBin;
    osg::ref_ptr<SurfaceBin> flatSingleSurfaceBin;
    osg::ref_ptr<SurfaceBin> smoothDoubleSurfaceBin;
    osg::ref_ptr<SurfaceBin> smoothSingleSurfaceBin;
};

class ReaderWriterACC : public osgDB::ReaderWriter
{
public:
    ReaderWriterACC();
    virtual const char* className() const { return "ACC Speed Dreams Database Reader"; }
    virtual osgDB::ReaderWriter::ReadResult readNode(const std::string& file,const Options* options);
    virtual osgDB::ReaderWriter::ReadResult readNode(std::istream& fin, const Options* options);
    virtual osgDB::ReaderWriter::WriteResult writeNode(const osg::Node& node,const std::string& fileName, const Options* /*options*/);
    virtual osgDB::ReaderWriter::WriteResult writeNode(const osg::Node& node,std::ostream& fout, const Options* opts);
    void SetCar(bool b);
	void SetSkin(std::string& name);
    osg::Node* readFile(std::istream& stream, const osgDB::ReaderWriter::Options* options);
    osg::Node* readObject(std::istream& stream, FileData& fileData, const osg::Matrix& parentTransform, TextureData textureData);

protected:
    //Used for cars
    osg::Group *m_transparentGroup;
    osg::Group *m_opaqueGroup;
    bool m_bCar;
    bool m_bBlockTransparent;
	std::string m_skinName;
};
