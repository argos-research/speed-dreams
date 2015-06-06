#include <osgDB/ReadFile>

#include "ReaderWriterACC.h"

class osgLoader
{
public:
	osgLoader(void);
	~osgLoader(void);
	
	osg::Node *Load3dFile(std::string strFile, bool car);
	osg::ref_ptr<osg::Image> LoadImageFile(std::string strFile);
	void AddSearchPath(std::string strPath);
	
protected:
	osgDB::ReaderWriter::Options *m_pOpt;
	ReaderWriterACC m_ACCReader;
};
