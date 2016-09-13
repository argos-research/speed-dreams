/***************************************************************************

    file                 : grscene.cpp
    created              : Mon Aug 21 20:13:56 CEST 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: grscene.cpp 6164 2015-10-04 23:14:42Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <ctime>
#include <string>

#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <glfeatures.h>
#include <robottools.h>	//RtXXX()
#include <portability.h> // snprintf

#include "grscene.h"
#include "grbackground.h"
#include "grmain.h"
#include "grloadac.h"
#include "grutil.h"
#include "grrain.h"

// Uncomment to enable support for PNG logos for normal pit-building wall
// (does not apply to the no-building pit, with the low wall, which already supports PNG).
//#define PNG_LOGO_SUPPORT 1

// Some public global variables.
int grWrldX;
int grWrldY;
int grWrldZ;
int grWrldMaxSize;
bool grSpeedway;
bool grSpeedwayshort;
static bool grBGSky = false;
static bool grBGType = false;
tTrack *grTrack;

// TheScene
ssgRoot *TheScene = NULL;

// TheScene kids order (but some others in background.cpp)
ssgBranch *LandAnchor = NULL;
ssgBranch *CarsAnchor = NULL;
ssgBranch *ShadowAnchor = NULL;
ssgBranch *PitsAnchor = NULL;
ssgBranch *SmokeAnchor = NULL;
ssgBranch *SkidAnchor = NULL;
ssgBranch *CarlightAnchor = NULL;
ssgBranch *TrackLightAnchor = NULL;
ssgBranch *ThePits = NULL;
ssgBranch *BackSkyAnchor = NULL;
ssgTransform *BackSkyLoc = NULL;

// Must have (Question: What for ?)
int preScene(ssgEntity *e)
{
    return TRUE;
}


/**
 * grInitScene
 * Initialises a scene (ie a new view).
 *
 * @return 0 if OK, -1 if something failed
 */
int
grInitScene(void)
{
    //ssgLight *light = ssgGetLight(0);

    // Load graphic options if not already done.
    if(!grHandle)
    {
        char buf[256];
        sprintf(buf, "%s%s", GfLocalDir(), GR_PARAM_FILE);
        grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    }//if grHandle

    // Initialize the background, sky ...
    grInitBackground();

    // Initialize the rain renderer
    const float precipitationDensity =
            GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_PRECIPDENSITY, "%", 100);
    grRain.initialize(grTrack->local.rain, precipitationDensity);

    /* GUIONS GL_TRUE */
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

#ifdef GL_SEPARATE_SPECULAR_COLOR
    GfLogTrace("Using GL_SEPARATE_SPECULAR_COLOR light model control\n");
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#else
#ifdef GL_SEPARATE_SPECULAR_COLOR_EXT
    GfLogTrace("Using GL_SEPARATE_SPECULAR_COLOR_EXT light model control\n");
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);
#endif
#endif

    return 0;
}//grInitScene

static void
grLoadGraphicsOptions()
{
    char buf[256];

    if (!grHandle)
    {
        sprintf(buf, "%s%s", GfLocalDir(), GR_PARAM_FILE);
        grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
    }//if grHandle

    grLoadBackgroundGraphicsOptions();
}

int
grLoadScene(tTrack *track)
{
    char buf[256];
    void		*hndl = grTrackHandle;
    const char		*acname;
    ssgEntity		*desc;

    // Load graphics options.
    grLoadGraphicsOptions();

    //GfLogDebug("grLoadScene(track=%p)\n", track);
    grTrack = track;

    // Build scene.
    TheScene = new ssgRoot;

    /* Background Sky */
    BackSkyAnchor = new ssgBranch;
    BackSkyLoc = new ssgTransform;
    BackSkyLoc->addKid(BackSkyAnchor);
    TheScene->addKid(BackSkyLoc);

    /* Landscape */
    LandAnchor = new ssgBranch;
    TheScene->addKid(LandAnchor);

    /* Pit stops walls */
    PitsAnchor = new ssgBranch;
    TheScene->addKid(PitsAnchor);

    /* Skid Marks */
    SkidAnchor = new ssgBranch;
    TheScene->addKid(SkidAnchor);

    /* Car shadows */
    ShadowAnchor = new ssgBranch;
    TheScene->addKid(ShadowAnchor);

    /* Car lights */
    CarlightAnchor = new ssgBranch;
    TheScene->addKid(CarlightAnchor);

    /* Cars */
    CarsAnchor = new ssgBranch;
    TheScene->addKid(CarsAnchor);

    /* Smoke */
    SmokeAnchor = new ssgBranch;
    TheScene->addKid(SmokeAnchor);

    /* Anchor for track lights */
    TrackLightAnchor = new ssgBranch;
    TheScene->addKid(TrackLightAnchor);

    /* Load the background (horizon and sky) */
    grLoadBackground();

    /* Determine the world limits */
    grWrldX = (int)(track->max.x - track->min.x + 1);
    grWrldY = (int)(track->max.y - track->min.y + 1);
    grWrldZ = (int)(track->max.z - track->min.z + 1);
    grWrldMaxSize = (int)(MAX(MAX(grWrldX, grWrldY), grWrldZ));

    if (strcmp(track->category, "speedway") == 0)
    {
        grSpeedway = true;

        if (strcmp(track->subcategory, "short") == 0)
            grSpeedwayshort = true;
        else
            grSpeedwayshort = false;
    }
    else
    {
        grSpeedway = false;
        grSpeedwayshort = false;
    }

    /* The track itself, and its landscape */
    acname = GfParmGetStr(hndl, TRK_SECT_GRAPH, TRK_ATT_3DDESC, "track.ac");
    if (strlen(acname) == 0)
    {
        GfLogError("No specified track 3D model file\n");
        return -1;
    }

    if ( grSkyDomeDistance > 0 )
    {
        grBGSky = strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKY, GR_ATT_BGSKY_DISABLED), GR_ATT_BGSKY_ENABLED) == 0;
        if (grBGSky)
        {
            grBGType = strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKYTYPE, GR_ATT_BGSKY_RING), GR_ATT_BGSKY_LAND) == 0;
            if (grBGType)
                grLoadBackgroundLand();
            else
                grLoadBackgroundSky();
        }
    }

    snprintf(buf, sizeof(buf), "tracks/%s/%s;data/textures;data/img;.", grTrack->category, grTrack->internalname);
    ssgTexturePath(buf);
    snprintf(buf, sizeof(buf), "tracks/%s/%s", grTrack->category, grTrack->internalname);
    ssgModelPath(buf);

    desc = grssgLoadAC3D(acname, NULL);
    LandAnchor->addKid(desc);

    return 0;
}//grLoadScene

void
grDrawScene()
{
    TRACE_GL("refresh: ssgCullAndDraw start");
    ssgCullAndDraw(TheScene);
    TRACE_GL("refresh: ssgCullAndDraw");
}//grDrawScene


void
grShutdownScene(void)
{
    grTrack = 0;

    delete TheScene;
    TheScene = 0;

    grShutdownBackground();
}//grShutdownScene


void grCustomizePits(void)
{
    char buf[512];
    ThePits = new ssgBranch();
    PitsAnchor->addKid(ThePits);

    tTrackPitInfo *pits = &(grTrack->pits);

    if (pits->pitindicator > 0)
        GfLogTrace("Pit Indicator ... YES ...\n");
    else
        GfLogTrace("Pit Indicator ... NO ...\n");


    // Draw the pit identification on the wall if any.
    switch (pits->type)
    {
    case TR_PIT_ON_TRACK_SIDE:
        GfLogTrace("Creating track side pit buildings (%d slots) ...\n", pits->nMaxPits);
        for (int i = 0; i < pits->nMaxPits; i++)
        {
            //GfLogDebug("Pit Nbr: %d\n", i);
            ssgVertexArray *pit_vtx = new ssgVertexArray(4);
            ssgTexCoordArray *pit_tex = new ssgTexCoordArray(4);
            ssgColourArray *pit_clr = new ssgColourArray(1);
            ssgNormalArray *pit_nrm = new ssgNormalArray(1);

            sgVec4 clr = {0, 0, 0, 1};
            pit_clr->add(clr);

            // Default driver logo file name (pit door).
            std::string strLogoFileName("logo");

            // If we have at least one car in the pit, use the team pit logo of driver 0.
            ssgState *st = 0;
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
                sgVec3 nrm = {  normalvector.x, normalvector.y, 0 };
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
                sgVec2 tex = { -0.7, 42.0 / 128 };
                sgVec3 vtx = { x2, y2, z2 };
                pit_tex->add(tex);
                pit_vtx->add(vtx);
            }

            // First, top vertex of the triangle strip
            {
                sgVec2 tex = { -0.7, 1.1 };
                sgVec3 vtx = { x2, y2, z2 + 4.8f };
                pit_tex->add(tex);
                pit_vtx->add(vtx);
            }

            // Determine the position of the second, bottom vertex of the triangle strip
            x2 -= pits->len * normalvector.y;
            y2 += pits->len * normalvector.x;
            z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x2, y2);

            // Second, bottom vertex of the triangle strip
            {
                sgVec2 tex = { -0.7 + 2.0, 42.0 / 128 };
                sgVec3 vtx = { x2, y2, z2 };
                pit_tex->add(tex);
                pit_vtx->add(vtx);
            }

            // Second, top vertex of the triangle strip
            {
                sgVec2 tex = { -0.7 + 2.0, 1.1 };
                sgVec3 vtx = { x2, y2, z2 + 4.8f };
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

#if 0
        char buf3[256];
        snprintf(buf3, sizeof(buf3), "%strack-pit.ac", GfLocalDir());

        ssgSaveAC ( buf3, PitsAnchor );
#endif

        break;

    case TR_PIT_NO_BUILDING:
    {
        GfLogTrace("Creating low pit wall (%d slots) ...\n", pits->nMaxPits);
        // This mode draws a white, low wall (about 3ft high) with logos

        // First we load the low wall's texture, as it is the same
        // for all the drivers
        snprintf(buf, sizeof(buf), "%sdata/textures", GfDataDir());
        const std::string strWallFileName = "tr-wall-gr.png";
        ssgState *stWall = grSsgLoadTexStateEx(strWallFileName.c_str(), buf,
                                               FALSE, FALSE, FALSE);
        reinterpret_cast<ssgSimpleState*>(stWall)->setShininess(100);

        bool bHasLogo = false;
        bool bHasPitIndicator = false;

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

            if (act_pit->car[0])
            {
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
                        && act_pit->car[0]->_skinTargets & RM_CAR_SKIN_TARGET_PIT_DOOR)
                {
                    strLogoFileName += '-';
                    strLogoFileName += act_pit->car[0]->_skinName;
                    GfLogTrace("Using skinned pit door logo %s\n",
                               strLogoFileName.c_str());
                }

                bHasLogo = true;

                if (pits->pitindicator > 0 && bHasLogo)
                    bHasPitIndicator = true;
                else
                    bHasPitIndicator = false;

            }
            else
            {
                snprintf(buf, sizeof(buf), "data/textures");
                strLogoFileName = "tr-wall-gr";
                bHasLogo = false;
                bHasPitIndicator = false;
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

            ssgVertexArray *pit_vtx1 = new ssgVertexArray(4);
            ssgTexCoordArray *pit_tex1 = new ssgTexCoordArray(4);
            ssgColourArray *pit_clr1 = new ssgColourArray(1);
            ssgNormalArray *pit_nrm1 = new ssgNormalArray(1);
            sgVec4 clr1 = { 1, 1, 1, 1 };
            pit_clr1->add(clr1);
            sgVec3 nrm1 = { normalvector.x, normalvector.y, 0.0 };
            pit_nrm1->add(nrm1);

            // First, bottom vertex
            {
                sgVec2 tex = { 0.0, 0.0 };
                sgVec3 vtx = { x1, y1, z1 };
                pit_tex1->add(tex);
                pit_vtx1->add(vtx);
            }

            // First, top vertex
            {
                sgVec2 tex = { 0.0, 0.25 };
                sgVec3 vtx = { x1, y1, z1 + 0.9f };
                pit_tex1->add(tex);
                pit_vtx1->add(vtx);
            }

            // Second, bottom vertex
            {
                sgVec2 tex = { 1.0, 0.0 };
                sgVec3 vtx = { x2, y2, z2 };
                pit_tex1->add(tex);
                pit_vtx1->add(vtx);
            }

            // Second, top vertex
            {
                sgVec2 tex = { 1.0, 0.25 };
                sgVec3 vtx = { x2, y2, z2 + 0.9f };
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
            if (!stLogo)
            {
                const std::string strRGBLogoFileName = strLogoFileName + ".rgb";
                stLogo = grSsgLoadTexStateEx(strRGBLogoFileName.c_str(), buf,
                                             FALSE, FALSE);
            }
            reinterpret_cast<ssgSimpleState*>(stLogo)->setShininess(50);

            ssgVertexArray *pit_vtx2 = new ssgVertexArray(4);
            ssgTexCoordArray *pit_tex2 = new ssgTexCoordArray(4);
            ssgColourArray *pit_clr2 = new ssgColourArray(1);
            ssgNormalArray *pit_nrm2 = new ssgNormalArray(1);
            sgVec4 clr2 = { 1, 1, 1, 1 };
            pit_clr2->add(clr2);
            sgVec3 nrm2 = { normalvector.x, normalvector.y, 0.0 };
            pit_nrm2->add(nrm2);

            // If bHasLogo is false, there is no team/driver logo,
            // we should display a plain concrete wall then without decals.
            // In that case strLogoFileName == strWallFileName,
            //but we also must care for different coord mapping.

            // First, bottom vertex
            {
                sgVec2 tex = { 0.0, 0.0 };
                sgVec3 vtx = { x2, y2, z2 };
                pit_tex2->add(tex);
                pit_vtx2->add(vtx);
            }

            // First, top vertex
            {
                sgVec2 tex = { 0.0, (bHasLogo ? 0.33f : 0.25f) };
                sgVec3 vtx = { x2, y2, z2 + 0.9f };
                pit_tex2->add(tex);
                pit_vtx2->add(vtx);
            }

            // Second, bottom vertex
            {
                sgVec2 tex = { 1.0, 0.0 };
                sgVec3 vtx = { x3, y3, z3 };
                pit_tex2->add(tex);
                pit_vtx2->add(vtx);
            }

            // Second, top vertex
            {
                sgVec2 tex = { 1.0, (bHasLogo ? 0.33f : 0.25f) };
                sgVec3 vtx = { x3, y3, z3 + 0.9f };
                pit_tex2->add(tex);
                pit_vtx2->add(vtx);
            }

            ssgVtxTable *pit2 = new ssgVtxTable(GL_TRIANGLE_STRIP, pit_vtx2,
                                                pit_nrm2, pit_tex2, pit_clr2);
            pit2->setState(stLogo);
            pit2->setCullFace(0);
            ThePits->addKid(pit2);

            // Draw 2nd wall
            ssgVertexArray *pit_vtx3 = new ssgVertexArray(4);
            ssgTexCoordArray *pit_tex3 = new ssgTexCoordArray(4);
            ssgColourArray *pit_clr3 = new ssgColourArray(1);
            ssgNormalArray *pit_nrm3 = new ssgNormalArray(1);
            sgVec4 clr3 = { 1, 1, 1, 1 };
            pit_clr3->add(clr3);
            sgVec3 nrm3 = { normalvector.x, normalvector.y, 0.0 };
            pit_nrm3->add(nrm3);
            // First, bottom vertex
            {
                sgVec2 tex = { 0.0, 0.0 };
                sgVec3 vtx = { x3, y3, z3 };
                pit_tex3->add(tex);
                pit_vtx3->add(vtx);
            }

            // First, top vertex
            {
                sgVec2 tex = { 0.0, 0.25 };
                sgVec3 vtx = { x3, y3, z3 + 0.9f };
                pit_tex3->add(tex);
                pit_vtx3->add(vtx);
            }

            // Second, bottom vertex
            {
                sgVec2 tex = { 1.0, 0.0 };
                sgVec3 vtx = { x4, y4, z4 };
                pit_tex3->add(tex);
                pit_vtx3->add(vtx);
            }

            // Second, top vertex
            {
                sgVec2 tex = { 1.0, 0.25 };
                sgVec3 vtx = { x4, y4, z4 + 0.9f };
                pit_tex3->add(tex);
                pit_vtx3->add(vtx);
            }

            ssgVtxTable *pit3 = new ssgVtxTable(GL_TRIANGLE_STRIP, pit_vtx3,
                                                pit_nrm3, pit_tex3, pit_clr3);
            pit3->setState(stWall);
            pit3->setCullFace(0);
            ThePits->addKid(pit3);

            if (bHasPitIndicator)
            {
                int PitInt = pits->pitindicator;
                snprintf(buf, sizeof(buf), "%sdrivers/%s/%d;%sdrivers/%s;drivers/%s/%d;drivers/%s;tracks/%s/%s;data/textures;data/img",
                         GfLocalDir(),
                         act_pit->car[0]->_modName,
                        act_pit->car[0]->_driverIndex,
                        GfLocalDir(),
                        act_pit->car[0]->_modName,
                        act_pit->car[0]->_modName,
                        act_pit->car[0]->_driverIndex,
                        act_pit->car[0]->_modName,
                        grTrack->category, grTrack->internalname);

                grLoadPitsIndicator(x3, y3, z3, buf, PitInt);
            }
        }  // for i
    }

#if 0
        char buf2[256];
        snprintf(buf2, sizeof(buf2), "%strack-pit.ac", GfLocalDir());

        ssgSaveAC ( buf2, PitsAnchor );
#endif

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
    }  // switch pit->type
}  // grCustomizePits

// Load New Pit Indicator in Scene
void grLoadPitsIndicator(tdble x, tdble y, tdble z, char *buf, int Pitind)
{
    char buf2[256];
    ssgEntity		*desc;
    sgCoord			PitIndicatorPos;
    ssgTransform *PitIndicatorLoc = new ssgTransform;

    ssgTexturePath(buf);
    snprintf(buf2, sizeof(buf2), "tracks/%s/%s;data/objects", grTrack->category, grTrack->internalname);
    ssgModelPath(buf2);

    sgSetCoord(&PitIndicatorPos, x, y, z, 0, 0, 0.0);
    PitIndicatorLoc->setTransform( &PitIndicatorPos);

    if (Pitind == 1)
        desc = grssgLoadAC3D("pit_indicator.ac", NULL);
    else
        desc = grssgLoadAC3D("normal_pit_indicator.ac", NULL);

    PitIndicatorLoc->addKid(desc);
    ThePits->addKid(PitIndicatorLoc);
}
