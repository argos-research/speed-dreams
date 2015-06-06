/***************************************************************************

    file                 : OsgScenery.cpp
    created              : Mon Aug 21 20:13:56 CEST 2012
    copyright            : (C) 2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr
    version              : $Id: OsgScenery.cpp 2436 2010-05-08 14:22:43Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/Camera>

#include "../OsgMain.h"
#include "OsgScenery.h"
#include "../OsgLoader/OsgLoader.h"

#include <glfeatures.h>	//gluXXX
#include <robottools.h>	//RtXXX()
#include <portability.h>

SDScenery::SDScenery(void)
{
	_grWrldX = 0;
	_grWrldY = 0;
	_grWrldZ = 0;
	_grWrldMaxSize = 0;
    grWrldX = 0;
    grWrldY = 0;
    grWrldZ = 0;
    grWrldMaxSize = 0;
    _max_visibility = 0;
    _nb_cloudlayer = 0;
    _DynamicSkyDome = 0;
    _SkyDomeDistance = 0;
    _SkyDomeDistThresh = 12000;

	_bgtype = false;
	_bgsky =  false;

    _scenery = NULL;
	//_spectators = 0;
	//_trees = 0;
	//_pits = 0;

    SDTrack = NULL;
}

SDScenery::~SDScenery(void)
{
	delete	m_background;
	//delete	m_spectators;
	//delete	m_trees;
	//delete	m_pits;

    delete SDTrack;

    _scenery = NULL;
    SDTrack = NULL;
}

void SDScenery::LoadScene(tTrack *track)
{
	void		*hndl = grTrackHandle;
	const char	*acname;
	char 		buf[256];

	GfOut("Initialisation class SDScenery\n");

    m_background = new SDBackground;
    _scenery = new osg::Group;
    SDTrack = track;

	// Load graphics options.
	LoadGraphicsOptions();

	if(grHandle == NULL)
	{
		sprintf(buf, "%s%s", GetLocalDir(), GR_PARAM_FILE);
		grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
	}//if grHandle

	/* Determine the world limits */
    _grWrldX = (int)(SDTrack->max.x - SDTrack->min.x + 1);
    _grWrldY = (int)(SDTrack->max.y - SDTrack->min.y + 1);
    _grWrldZ = (int)(SDTrack->max.z - SDTrack->min.z + 1);
	_grWrldMaxSize = (int)(MAX(MAX(_grWrldX, _grWrldY), _grWrldZ));

    grWrldX = _grWrldX;
    grWrldY = _grWrldY;
    grWrldZ = _grWrldZ;
    grWrldMaxSize = _grWrldMaxSize;

        acname = GfParmGetStr(hndl, TRK_SECT_GRAPH, TRK_ATT_3DDESC, "track.ac");
        GfOut("ACname = %s\n", acname);
        if (strlen(acname) == 0)
        {
                GfLogError("No specified track 3D model file\n");
        }

        std::string PathTmp = GetDataDir();

    if (_SkyDomeDistance > 0 && SDTrack->skyversion > 0)
	{
		_bgsky = strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKY, GR_ATT_BGSKY_DISABLED), GR_ATT_BGSKY_ENABLED) == 0;
		if (_bgsky)
		{
			_bgtype = strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKYTYPE, GR_ATT_BGSKY_RING), GR_ATT_BGSKY_LAND) == 0;
			std::string strPath = PathTmp;
            sprintf(buf, "tracks/%s/%s", SDTrack->category, SDTrack->internalname);
			strPath += buf;
            m_background->build(_bgtype, _grWrldX, _grWrldY, _grWrldZ, strPath);
			GfOut("Background loaded\n");
		}
	}

	std::string strPath = GetDataDir();
    sprintf(buf, "tracks/%s/%s", SDTrack->category, SDTrack->internalname);
	
	std::string ext = osgDB::getFileExtension(acname);
	
	if (ext == "acc")
	{
		strPath+=buf;
        _strTexturePath = strPath;
		strPath+="/";	
		strPath+=acname;

		LoadTrack(strPath);
    }
	else
	{
		strPath+=buf;
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options();
        options->getDatabasePathList().push_back(strPath);
        std::string strTPath = GetDataDir();
        snprintf(buf, 4096, "data/textures/");
        strTPath += buf;
        options->getDatabasePathList().push_back(strTPath);
        osg::ref_ptr<osg::Node> pTrack = osgDB::readNodeFile(acname, options);
        	
        if (ext =="ac")
        {
            osg::ref_ptr<osg::MatrixTransform> rot = new osg::MatrixTransform;
            osg::Matrix mat( 1.0f,  0.0f, 0.0f, 0.0f,
                     			 0.0f,  0.0f, 1.0f, 0.0f,
                     			 0.0f, -1.0f, 0.0f, 0.0f,
                     			 0.0f,  0.0f, 0.0f, 1.0f);
            rot->setMatrix(mat);
            rot->addChild(pTrack);
            _scenery->addChild(rot.get());
        }
        else
        {
            _scenery->addChild(pTrack.get());
        }
	}
}

void SDScenery::LoadSkyOptions()
{
	// Sky dome / background.
	_SkyDomeDistance = (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE, 0, 0) + 0.5);
	if (_SkyDomeDistance > 0 && _SkyDomeDistance < _SkyDomeDistThresh)
		_SkyDomeDistance = _SkyDomeDistThresh; // If user enabled it (>0), must be at least the threshold.

	_DynamicSkyDome = _SkyDomeDistance > 0 && strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, GR_ATT_DYNAMICSKYDOME_DISABLED),
						GR_ATT_DYNAMICSKYDOME_ENABLED) == 0;

	GfLogInfo("Graphic options : Sky dome : distance = %u m, dynamic = %s\n",
			  _SkyDomeDistance, _DynamicSkyDome ? "true" : "false");

	// Cloud layers.
	//grNbCloudLayers = (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_CLOUDLAYER, 0, 0) + 0.5);

	//GfLogInfo("Graphic options : Number of cloud layers : %u\n", grNbCloudLayers);

	_max_visibility = (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_VISIBILITY, 0, 0));
}

void SDScenery::LoadGraphicsOptions()
{
	char buf[256];

	if (!grHandle)
	{
		sprintf(buf, "%s%s", GfLocalDir(), GR_PARAM_FILE);
		grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
	}//if grHandle

	LoadSkyOptions();
}

void SDScenery::ShutdownScene(void)
{
	_scenery->removeChildren(0, _scenery->getNumChildren());
    _scenery = NULL;

    SDTrack = NULL;
}

bool SDScenery::LoadTrack(std::string strTrack)
{
	char buf[4096];
	GfOut("Chemin Track : %s\n", strTrack.c_str());
    osgLoader loader;
	GfOut("Chemin Textures : %s\n", _strTexturePath.c_str());
	loader.AddSearchPath(_strTexturePath);
	
	std::string strTPath = GetDataDir();
	snprintf(buf, 4096, "data/textures/");
    strTPath += buf;
    loader.AddSearchPath(strTPath);
    	
    osg::ref_ptr<osg::Node> pTrack = loader.Load3dFile(strTrack, false);

	if (pTrack)
	{
        pTrack->getOrCreateStateSet()->setRenderBinDetails(TRACKBIN,"RenderBin");
        _scenery->addChild(pTrack.get());
        osgDB::writeNodeFile(*pTrack,"/home/xavier/track.osgt");
	}
	else
		return false;

	return true;
}

void CreatePit(tTrack *track)
{
    /*char buf[512];
    osg::ref_ptr<osg::Node> ThePits = new osg::Node;
    osg::ref_ptr<osg::Group> PitsAnchor = new osg::Group;
    PitsAnchor->addChild(ThePits);

    tTrackPitInfo *pits = &(SDTrack->pits);

    // Draw the pit identification on the wall if any.
    switch (pits->type)
    {
      case TR_PIT_ON_TRACK_SIDE:
        GfLogTrace("Creating track side pit buildings (%d slots) ...\n", pits->nMaxPits);
        for (int i = 0; i < pits->nMaxPits; i++)
        {
            //GfLogDebug("Pit Nbr: %d\n", i);
          osg::Vec3Array *pit_vtx = new osg::Vec3Array;
          osg::Vec2Array *pit_tex = new osg::Vec2Array;
          osg::Vec4Array *pit_clr = new osg::Vec4Array;
          osg::Vec3Array *pit_nrm = new osg::Vec3Array;

          osg::Vec4 clr = {0, 0, 0, 1};
          pit_clr = clr;

          // Default driver logo file name (pit door).
          std::string strLogoFileName("logo");

          // If we have at least one car in the pit, use the team pit logo of driver 0.
          osg::ref_ptr<osg::StateSet> statePit = new osg::StateSet;
          //statePit->get
          //ssgState *st = 0;
          if (pits->driversPits[i].car[0])
          {
            snprintf(buf, sizeof(buf),
                     "%sdrivers/%s/%d;%sdrivers/%s;drivers/%s/%d;drivers/%s",
                     GfLocalDir(),
                     pits->driversPits[i].car[0]->_modName,
                     pits->driversPits[i].car[0]->_driverIndex,
                     GfLocalDir(),
                     pits->driversPits[i].car[0]->_modName,
                     pits->driversPits[i].car[0]->_modName,
                     pits->driversPits[i].car[0]->_driverIndex,
                     pits->driversPits[i].car[0]->_modName);

            // If a custom skin was selected, and it can apply to the pit door,
            // update the logo file name accordingly.
            int skinnedLogo = FALSE;
            if (strlen(pits->driversPits[i].car[0]->_skinName) != 0
                && pits->driversPits[i].car[0]->_skinTargets & RM_CAR_SKIN_TARGET_PIT_DOOR)
            {
              skinnedLogo = TRUE;
              strLogoFileName += '-';
              strLogoFileName += pits->driversPits[i].car[0]->_skinName;
              GfLogTrace("Using skinned pit door logo %s\n", strLogoFileName.c_str());
            }

            // Load logo texture (only rgbs - pngs cause pit transparency bug # 387)
  #ifdef PNG_LOGO_SUPPORT
            const std::string strPNGLogoFileName = strLogoFileName + ".png";
            st = grSsgLoadTexStateEx(strPNGLogoFileName.c_str(), buf, FALSE, FALSE, FALSE);
  #endif
            if (!st)
            {
                const std::string strRGBLogoFileName = strLogoFileName + ".rgb";
                st = grSsgLoadTexStateEx(strRGBLogoFileName.c_str(), buf, FALSE, FALSE, skinnedLogo);
            }
          }  // if pits->driverPits[i].car[0]

          // If no car in the pit, or logo file not found, hope for the .rgb in data/textures.
          if (!st)
          {
            snprintf(buf, sizeof(buf), "data/textures");
            const std::string strRGBLogoFileName = strLogoFileName + ".rgb";
            st = grSsgLoadTexStateEx(strRGBLogoFileName.c_str(), buf, FALSE, FALSE, TRUE);
          }
          st->setOpaque();
          reinterpret_cast<ssgSimpleState*>(st)->setShininess(50);
          reinterpret_cast<ssgSimpleState*>(st)->disable(GL_BLEND);

          // Pit wall texturing : the loaded 'logo*.rgb/.png' image file is supposed to consist
          // of 4 distinct parts :
          //
          //  **************************************************** 1.0
          //  *..................................................*
          //  *.                                                .*
          //  *.           Pit door (dots included)             .*
          //  *.                                                .*
          //  *.                                                .*
          //  *..................................................*
          //  ##########################$$$$$$$$$$$$$$$$$$$$$$$$$$ 0.33
          //  #                        #$                        $
          //  #      Team logo         #$     Team name          $
          //  #                        #$                        $
          //  ##########################$$$$$$$$$$$$$$$$$$$$$$$$$$ 0.0
          //  0.0                     0.5                      1.0
          //
          // - the stars '*' : Left, top and right 1-pixel-wide line :
          //   the pit wall (will be repeated respectively in the left, top and right
          //   direction in order to cover the pit-wall outside of the rectangle of the pit-door
          // - the dots '.' : the real pit door texture (actually includes the stars part)
          // - the hashes '#' : the team name texture
          // - the dollars '$' : the team logo texture
          //
          // More details here : http://www.berniw.org/torcs/robot/ch6/pitlogo.html

          // Determine the position of the pit wall, and its normal vector.
          tdble x, y;
          t3Dd normalvector;
          RtTrackLocal2Global(&(pits->driversPits[i].pos), &x, &y,
                              pits->driversPits[i].pos.type);
          RtTrackSideNormalG(pits->driversPits[i].pos.seg, x, y,
                              pits->side, &normalvector);

          // Normal
          {
              osg::Vec3 nrm = {  normalvector.x, normalvector.y, 0 };
              pit_nrm->add(nrm);
          }

          // Determine the position of the first, bottom vertex of the triangle strip
          tdble x2 = x - pits->width / 2.0 * normalvector.x
              + pits->len / 2.0 * normalvector.y;
          tdble y2 = y - pits->width / 2.0 * normalvector.y
              - pits->len / 2.0 * normalvector.x;
          tdble z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x2, y2);

          // First, bottom vertex of the triangle strip
          {
              osg::Vec2 tex( -0.7, 42.0 / 128 );
              osg::Vec3 vtx( x2, y2, z2 );
              pit_tex->add(tex);
              pit_vtx->add(vtx);
          }

          // First, top vertex of the triangle strip
          {
              osg::Vec2 tex( -0.7, 1.1 );
              osg::Vec3 vtx( x2, y2, z2 + 4.8f );
              pit_tex->add(tex);
              pit_vtx->add(vtx);
          }

          // Determine the position of the second, bottom vertex of the triangle strip
          x2 -= pits->len * normalvector.y;
          y2 += pits->len * normalvector.x;
          z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x2, y2);

          // Second, bottom vertex of the triangle strip
          {
              osg::Vec2 tex( -0.7 + 2.0, 42.0 / 128 );
              osg::Vec3 vtx( x2, y2, z2 );
              pit_tex->add(tex);
              pit_vtx->add(vtx);
          }

          // Second, top vertex of the triangle strip
          {
              osg::Vec2 tex( -0.7 + 2.0, 1.1 );
              osg::Vec3 vtx( x2, y2, z2 + 4.8f );
              pit_tex->add(tex);
              pit_vtx->add(vtx);
          }

          // Build-up the vertex array.
          ssgVtxTable *pit = new ssgVtxTable(GL_TRIANGLE_STRIP, pit_vtx,
                                              pit_nrm, pit_tex, pit_clr);

          // Attach the texture state to it.
          pit->setState(st);
          pit->setCullFace(0);

          // Done.
          ThePits->addKid(pit);
        }  // for i
        break;

      case TR_PIT_NO_BUILDING:
        {
        GfLogTrace("Creating low pit wall (%d slots) ...\n", pits->nMaxPits);
        // This mode draws a white, low wall (about 3ft high) with logos

        // First we load the low wall's texture, as it is the same
        // for all the drivers
        snprintf(buf, sizeof(buf), "data/textures");
        const std::string strWallFileName = "tr-bar-gr.png";
        ssgState *stWall = grSsgLoadTexStateEx(strWallFileName.c_str(), buf,
                                                FALSE, FALSE, FALSE);
        reinterpret_cast<ssgSimpleState*>(stWall)->setShininess(100);

        bool bHasLogo = false;
        // Loop over all the available pits
        for (int i = 0; i < pits->nMaxPits; i++)
        {
          tTrackOwnPit *act_pit = &(pits->driversPits[i]);

          // Get this pit's center coords
          tdble x0, y0;
          t3Dd normalvector;
          RtTrackLocal2Global(&(act_pit->pos), &x0, &y0, act_pit->pos.type);
          RtTrackSideNormalG(act_pit->pos.seg, x0, y0, pits->side, &normalvector);
          // This offset needed so the pit walls start at the correct place
          x0 = x0 - pits->width / 2.0 * normalvector.x
            - pits->len / 4.0 * normalvector.y;

          // Load this pit's own logo
          // Default driver logo filename (pit door)
          std::string strLogoFileName("logo");

          if (act_pit->car[0]) {
            // If we have more than one car in the pit,
            // use the team pit logo of driver 0.
            snprintf(buf, sizeof(buf),
              "%sdrivers/%s/%d;%sdrivers/%s;drivers/%s/%d;drivers/%s",
              GfLocalDir(),
              act_pit->car[0]->_modName,
              act_pit->car[0]->_driverIndex,
              GfLocalDir(),
              act_pit->car[0]->_modName,
              act_pit->car[0]->_modName,
              act_pit->car[0]->_driverIndex,
              act_pit->car[0]->_modName);

            // If a custom skin was selected, and it can apply to the pit door,
            // update the logo file name accordingly
            if (strlen(act_pit->car[0]->_skinName) != 0
                && act_pit->car[0]->_skinTargets & RM_CAR_SKIN_TARGET_PIT_DOOR) {
              strLogoFileName += '-';
              strLogoFileName += act_pit->car[0]->_skinName;
              GfLogTrace("Using skinned pit door logo %s\n",
                          strLogoFileName.c_str());
            }
        bHasLogo = true;
          } else {
            snprintf(buf, sizeof(buf), "data/textures");
        strLogoFileName = "tr-bar-gr";
        bHasLogo = false;
          }  // if act_pit->car[0]

          // Let's draw the low wall
          // It is drawn in 3 parts.
          // First some small wall, then the logo, then some more wall.
          // Small wall bounds: (x1, y1) - (x2, y2)
          // Logo part bounds:  (x2, y2) - (x3, y3)
          // Small wall bounds: (x3, y3) - (x4, y4)

          tdble x1 = x0 - pits->width / 2.0 * normalvector.x
            + pits->len / 2.0 * normalvector.y;
          tdble y1 = y0 - pits->width / 2.0 * normalvector.y
            - pits->len / 2.0 * normalvector.x;
          tdble z1 = RtTrackHeightG(act_pit->pos.seg, x1, y1);

          tdble x2 = x0 - pits->width / 2.0 * normalvector.x
            + pits->len / 4.0 * normalvector.y;
          tdble y2 = y0 - pits->width / 2.0 * normalvector.y
            - pits->len / 4.0 * normalvector.x;
          tdble z2 = RtTrackHeightG(act_pit->pos.seg, x2, y2);

          tdble x3 = x0 - pits->width / 2.0 * normalvector.x
            - pits->len / 4.0 * normalvector.y;
          tdble y3 = y0 - pits->width / 2.0 * normalvector.y
            + pits->len / 4.0 * normalvector.x;
          tdble z3 = RtTrackHeightG(act_pit->pos.seg, x3, y3);

          tdble x4 = x0 - pits->width / 2.0 * normalvector.x
            - pits->len / 2.0 * normalvector.y;
          tdble y4 = y0 - pits->width / 2.0 * normalvector.y
            + pits->len / 2.0 * normalvector.x;
          tdble z4 = RtTrackHeightG(act_pit->pos.seg, x4, y4);

          osg::Vec3Array *pit_vtx1 = new osg::Vec3Array;
          osg::Vec2Array *pit_tex1 = new osg::Vec2Array;
          osg::Vec4Array *pit_clr1 = new osg::Vec4Array;
          osg::Vec3Array *pit_nrm1 = new osg::Vec3Array;

          osg::Vec4 clr1( 1, 1, 1, 1 );
          pit_clr1 = clr1;
          osg::Vec3 nrm1( normalvector.x, normalvector.y, 0.0 );
          pit_nrm1->add(nrm1);

          // First, bottom vertex
          {
              osg::Vec2 tex( 0.0, 0.0 );
              osg::Vec3 vtx( x1, y1, z1 );
              pit_tex1->add(tex);
              pit_vtx1->add(vtx);
          }

          // First, top vertex
          {
              osg::Vec2 tex( 0.0, 0.25 );
              osg::Vec3 vtx( x1, y1, z1 + 0.9f );
              pit_tex1->add(tex);
              pit_vtx1->add(vtx);
          }

          // Second, bottom vertex
          {
              osg::Vec2 tex( 1.0, 0.0 );
              osg::Vec3 vtx( x2, y2, z2 );
              pit_tex1->add(tex);
              pit_vtx1->add(vtx);
          }

          // Second, top vertex
          {
              osg::Vec2 tex( 1.0, 0.25 );
              osg::Vec3 vtx( x2, y2, z2 + 0.9f );
              pit_tex1->add(tex);
              pit_vtx1->add(vtx);
          }

          ssgVtxTable *pit = new ssgVtxTable(GL_TRIANGLE_STRIP, pit_vtx1,
                                                pit_nrm1, pit_tex1, pit_clr1);
          pit->setState(stWall);
          pit->setCullFace(0);
          ThePits->addKid(pit);

          // Let's draw the logo
          // Load logo texture (.png first, then .rgb for backwards compatibility)
          const std::string strPNGLogoFileName = strLogoFileName + ".png";
          ssgState *stLogo = grSsgLoadTexStateEx(strPNGLogoFileName.c_str(),
                                          buf, FALSE, FALSE, FALSE);
          if (!stLogo) {
            const std::string strRGBLogoFileName = strLogoFileName + ".rgb";
            stLogo = grSsgLoadTexStateEx(strRGBLogoFileName.c_str(), buf,
                                          FALSE, FALSE);
          }
          reinterpret_cast<ssgSimpleState*>(stLogo)->setShininess(50);

          osg::Vec3Array *pit_vtx2 = new osg::Vec3Array;
          osg::Vec2Array *pit_tex2 = new osg::Vec2Array;
          osg::Vec4Array *pit_clr2 = new osg::Vec4Array;
          osg::Vec3Array *pit_nrm2 = new osg::Vec3Array;

          osg::Vec4 clr2( 1, 1, 1, 1 );
          pit_clr2 = clr2;
          osg::Vec3 nrm2( normalvector.x, normalvector.y, 0.0 );
          pit_nrm2 = nrm2;

      // If bHasLogo is false, there is no team/driver logo,
      // we should display a plain concrete wall then without decals.
      // In that case strLogoFileName == strWallFileName,
      //but we also must care for different coord mapping.

          // First, bottom vertex
          {
              osg::Vec2 tex( 0.0, 0.0 );
              osg::Vec3 vtx( x2, y2, z2 );
              pit_tex2 = tex;
              pit_vtx2 = vtx;
          }

          // First, top vertex
          {
            osg::Vec2 tex = { 0.0, (bHasLogo ? 0.33f : 0.25f) };
            osg::Vec3 vtx = { x2, y2, z2 + 0.9f };
            pit_tex2 = tex;
            pit_vtx2 = vtx;
          }

          // Second, bottom vertex
          {
              osg::Vec2 tex( 1.0, 0.0 );
              osg::Vec3 vtx( x3, y3, z3 );
              pit_tex2 = tex;
              pit_vtx2 = vtx;
          }

          // Second, top vertex
          {
              osg::Vec2 tex( 1.0, (bHasLogo ? 0.33f : 0.25f));
              osg::Vec3 vtx( x3, y3, z3 + 0.9f );
              pit_tex2 = tex;
              pit_vtx2 = vtx;
          }

          ssgVtxTable *pit2 = new ssgVtxTable(GL_TRIANGLE_STRIP, pit_vtx2,
                                                pit_nrm2, pit_tex2, pit_clr2);
          pit2->setState(stLogo);
          pit2->setCullFace(0);
          ThePits->addKid(pit2);

          // Draw 2nd wall
          osg::Vec3Array *pit_vtx3 = new osg::Vec3Array;
          osg::Vec2Array *pit_tex3 = new osg::Vec2Array;
          osg::Vec4Array *pit_clr3 = new osg::Vec4Array;
          osg::Vec3Array *pit_nrm3 = new osg::Vec3Array;
          osg::Vec4 clr3 = { 1, 1, 1, 1 };
          pit_clr3->add(clr3);
          osg::Vec3 nrm3 = { normalvector.x, normalvector.y, 0.0 };
          pit_nrm3->add(nrm3);
          // First, bottom vertex
          {
            osg::Vec2 tex = { 0.0, 0.0 };
            osg::Vec3 vtx = { x3, y3, z3 };
            pit_tex3->add(tex);
            pit_vtx3->add(vtx);
          }

          // First, top vertex
          {
            osg::Vec2 tex = { 0.0, 0.25 };
            osg::Vec3 vtx = { x3, y3, z3 + 0.9f };
            pit_tex3->add(tex);
            pit_vtx3->add(vtx);
          }

          // Second, bottom vertex
          {
            osg::Vec2 tex = { 1.0, 0.0 };
            osg::Vec3 vtx = { x4, y4, z4 };
            pit_tex3->add(tex);
            pit_vtx3->add(vtx);
          }

          // Second, top vertex
          {
            osg::Vec2 tex = { 1.0, 0.25 };
            osg::Vec3 vtx = { x4, y4, z4 + 0.9f };
            pit_tex3->add(tex);
            pit_vtx3->add(vtx);
          }

          geometry->setUseDisplayList(false);
          geometry->setVertexArray(pit_vtx3);
          geometry->setNormalArray(pit_nrm3);
          geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
          geometry->setTexCoordArray(0, pit_tex3);
          geometry->setColorArray(pit_clr3);
          geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
          geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, pit_vtx3->size()));
          geode->addDrawable(geometry);

          ssgVtxTable *pit3 = new ssgVtxTable(GL_TRIANGLE_STRIP, pit_vtx3,
                                                pit_nrm3, pit_tex3, pit_clr3);
          pit3->setState(stWall);
          pit3->setCullFace(0);
          ThePits->addKid(pit3);
        }  // for i
        }
        break;

      case TR_PIT_ON_SEPARATE_PATH:
        // Not implemented yet
        GfLogTrace("Creating separate path pits (%d slots) ...\n", pits->nMaxPits);
        GfLogWarning("Separate path pits are not yet implemented.\n");
        break;

      case TR_PIT_NONE:
        // Nothing to do
        GfLogTrace("Creating no pits.\n");
        break;
    }  // switch pit->type*/
}
