/***************************************************************************

    file                 : graphic.h
    created              : Sun Jan 30 22:58:45 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: graphic.h 6071 2015-08-14 16:27:16Z kakukri $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
 
#ifndef _GRAPHV1_H_
#define _GRAPHV1_H_

#define GRX_IDENT	0

#define GR_PARAM_FILE		"config/graph.xml"

#define GR_SCT_DISPMODE		"Display Mode"
#define GR_ATT_CAM          "camera"
#define GR_ATT_CAM_HEAD		"camera head list"
#define GR_ATT_MIRROR		"enable mirror"
#define GR_ATT_MAP          "map mode"
#define GR_ATT_FOVY         "fovy"
#define GR_ATT_BOARD		"driver board"
#define GR_ATT_COUNTER		"driver counter"
#define GR_ATT_LEADER		"leader board"
#define GR_ATT_DEBUG		"debug info"
#define GR_ATT_GGRAPH		"G graph"
#define GR_ATT_ARCADE		"arcade"
#define GR_ATT_BOARDWIDTH	"board width"
#define GR_ATT_SPEEDORISE	"speedometer vertical position"
#define GR_ATT_NBLEADER		"Max leaders entries"
#define GR_ATT_DASHBOARD    "dashboard"

#define GR_SCT_TVDIR		"TV Director View"
#define GR_ATT_CHGCAMINT	"change camera interval"
#define GR_ATT_EVTINT		"event interval"
#define GR_ATT_PROXTHLD		"proximity threshold"

#define GR_SCT_GRAPHIC		"Graphic"
#define GR_ATT_SMOKENB		"smoke value"
#define GR_ATT_SMOKEDELTAT	"smoke interval"
#define GR_ATT_SMOKEDLIFE	"smoke duration"

#define GR_ATT_MAXSTRIPBYWHEEL	"skid value"
#define GR_ATT_MAXPOINTBYSTRIP	"skid length"
#define GR_ATT_SKIDDELTAT	"skid interval"
#define GR_ATT_FOVFACT		"fov factor"
#define GR_ATT_LODFACTOR	"LOD Factor"
#define GR_ATT_SKYDOMEDISTANCE		"sky dome distance"
#define GR_ATT_DYNAMICSKYDOME	"dynamic sky dome"
#define GR_ATT_DYNAMICSKYDOME_ENABLED	"enabled"
#define GR_ATT_DYNAMICSKYDOME_DISABLED	"disabled"
#define GR_ATT_PRECIPDENSITY	"precipitation density"
#define GR_ATT_VISIBILITY	"visibility"
#define GR_ATT_BGSKY		"background skydome"
#define GR_ATT_BGSKYTYPE	"background type"
#define GR_ATT_BGSKY_RING	"background"
#define GR_ATT_BGSKY_LAND	"land"
#define GR_ATT_BGSKY_DISABLED	"disabled"
#define GR_ATT_BGSKY_ENABLED	"enabled"
#define GR_ATT_CLOUDLAYER	"cloudlayer"
#define GR_ATT_AGR_FOREST	"forest LOD"
#define GR_ATT_AGR_TREE		"tree LOD"
#define GR_ATT_AGR_QUALITY	"shadow quality"
//#define GR_ATT_DYNAMICWEATHER	"dynamic weather"

#define GR_ATT_SHADOW_TYPE  "shadow type"
#define GR_ATT_SHADOW_NONE  "shadow static"
#define GR_ATT_SHADOW_SM    "shadowmap"
#define GR_ATT_SHADOW_SSM   "soft shadowmap"
#define GR_ATT_SHADOW_PSSM  "paralele shadowmap"
#define GR_ATT_SHADOW_LSPM  "light shadowmap"
#define GR_ATT_SHADOW_VOLUME "shadow volume"
#define GR_ATT_SHADOW_VDSM  "view dependent shadowmap"

#define GR_ATT_SHADOW_SIZE  "texture shadow size"
#define GR_ATT_SHADOW_512   "512"
#define GR_ATT_SHADOW_1024  "1024"
#define GR_ATT_SHADOW_2048  "2048"
#define GR_ATT_SHADOW_4096  "4096"
#define GR_ATT_SHADOW_8192  "8192"

#define GR_ATT_SHADERS      "shaders"

#define GR_ATT_ENV_TYPE     "texture environment"
#define GR_ATT_AGR_NULL		"none"
#define GR_ATT_AGR_LITTLE	"little"
#define GR_ATT_AGR_MEDIUM	"medium"
#define GR_ATT_AGR_FULL		"full"
#define GR_ATT_AGR_HIGH		"high"
#define GR_ATT_ENV_SIZE     "environment size"

#define GR_ATT_NB_SCREENS	"number of screens"
#define GR_ATT_ARR_SCREENS	"arrangement of screens"
#define GR_ATT_CUR_SCREEN	"current screen"
#define GR_ATT_CUR_DRV		"current driver"

#define GR_SCT_PLAYABLE_DOV     "Playable Cameras Distance of Views"
#define GR_ATT_FRONT_GLOBAL     "Front Level Group Global"
#define GR_ATT_FRONT_LEVEL3     "Front Level Group 3"
#define GR_ATT_FRONT_LEVEL2     "Front Level Group 2"
#define GR_ATT_FRONT_LEVEL1     "Front Level Group 1"
#define GR_ATT_REAR_GLOBAL      "Rear Level Group Global"
#define GR_ATT_REAR_LEVEL3      "Rear Level Group 3"
#define GR_ATT_REAR_LEVEL2      "Rear Level Group 2"
#define GR_ATT_REAR_LEVEL1      "Rear Level Group 1"

#define GR_ATT_FRONT_MAP1       "Front Level Map 1" 
#define GR_ATT_FRONT_MAP2       "Front Level Map 2"
#define GR_ATT_FRONT_MAP3       "Front Level Map 3"
#define GR_ATT_REAR_MAP1        "Rear Level Map 1"
#define GR_ATT_REAR_MAP2        "Rear Level Map 2"
#define GR_ATT_REAR_MAP3        "Rear Level Map 3"

#define GR_SCT_MONITOR          "Monitor"
#define GR_ATT_MONITOR          "monitor type"
#define GR_VAL_MONITOR_16BY9	"16:9"
#define GR_VAL_MONITOR_4BY3		"4:3"
#define GR_VAL_MONITOR_NONE		"none"
#define GR_ATT_SPANSPLIT        "span splits"
#define GR_VAL_YES              "yes"
#define GR_VAL_NO               "no"
#define GR_ATT_BEZELCOMP        "bezel compensation"
#define GR_ATT_SCREENDIST       "screen distance"
#define GR_ATT_ARCRATIO         "arc ratio"

// Dash colours
#define GFSCR_SECT_DASHSETTINGS	"Dash settings"

#define GFSCR_DASH_COLORS       "colors"

#define GFSCR_ELT_NORMALCLR		"normal color"
#define GFSCR_ELT_DANGERCLR		"danger color"
#define GFSCR_ELT_OKCLR			"OK color"
#define GFSCR_ELT_ERRORCLR		"error color"
#define GFSCR_ELT_INACTIVECLR	"inactive text color"
#define GFSCR_ELT_EMPHASIZEDCLR	"emphasized color"
#define GFSCR_ELT_AHEADCLR		"car ahead color"
#define GFSCR_ELT_BEHINDCLR		"car behind color"
#define GFSCR_ELT_ARCADECLR		"arcade color"
#define GFSCR_ELT_BACKGROUNDCLR	"dash panel background color"
// end Dash colours

#endif /* _GRAPHV1_H_ */
