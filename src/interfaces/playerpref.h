/***************************************************************************

    file                 : playerpref.h
    created              : Wed Mar 21 21:50:23 CET 2001
    copyright            : (C) 2001 by Eric Espi�
    email                : Eric.Espie@torcs.org
    version              : $Id: playerpref.h 6248 2015-11-22 17:05:10Z kakukri $

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
    		
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: playerpref.h 6248 2015-11-22 17:05:10Z kakukri $
*/

#ifndef _PLAYERPREF_H_
#define _PLAYERPREF_H_

#define HM_DRV_FILE  		"drivers/human/human.xml"
#define HM_PREF_FILE		"drivers/human/preferences.xml"

#define	HM_SECT_JSPREF		"joystick"
#define	HM_SECT_MOUSEPREF	"mouse"
#define	HM_SECT_KEYBPREF	"keyboard"

#define HM_SECT_DRVPREF		"Preferences/Drivers"

#define HM_ATT_CONTROL		"control"

#define HM_ATT_UP_SHFT		"up shift"
#define HM_ATT_UP_SHFT_MIN	"up shift min"
#define HM_ATT_UP_SHFT_MAX	"up shift max"
#define HM_ATT_DN_SHFT		"down shift"
#define HM_ATT_DN_SHFT_MIN	"down shift min"
#define HM_ATT_DN_SHFT_MAX	"down shift max"
#define HM_ATT_ASR_CMD		"ASR cmd"
#define HM_ATT_ASR_MIN		"ASR min"
#define HM_ATT_ASR_MAX		"ASR max"
#define HM_ATT_ABS_CMD		"ABS cmd"
#define HM_ATT_ABS_MIN		"ABS min"
#define HM_ATT_ABS_MAX		"ABS max"
#define HM_ATT_EBRAKE_CMD	"ebrake cmd"
#define HM_ATT_EBRAKE_MIN	"ebrake min"
#define HM_ATT_EBRAKE_MAX	"ebrake max"

#define HM_ATT_LIGHT1_CMD	"Light1 cmd"
#define HM_ATT_LIGHT1_MIN	"Light1 min"
#define HM_ATT_LIGHT1_MAX	"Light1 max"
#define HM_ATT_SPDLIM_CMD	"Speed Limiter"
#define HM_ATT_SPDLIM_MIN	"Speed Limiter min"
#define HM_ATT_SPDLIM_MAX	"Speed Limiter max"

#define HM_ATT_AUTOREVERSE	"auto reverse"

#define HM_ATT_GEAR_R		"reverse gear"
#define HM_ATT_GEAR_R_MIN	"reverse gear min"
#define HM_ATT_GEAR_R_MAX	"reverse gear max"
#define HM_ATT_GEAR_N		"neutral gear"
#define HM_ATT_GEAR_N_MIN	"neutral gear min"
#define HM_ATT_GEAR_N_MAX	"neutral gear max"

#define HM_ATT_GEAR_1		"1st gear"
#define HM_ATT_GEAR_2		"2nd gear"
#define HM_ATT_GEAR_3		"3rd gear"
#define HM_ATT_GEAR_4		"4th gear"
#define HM_ATT_GEAR_5		"5th gear"
#define HM_ATT_GEAR_6		"6th gear"

/* used for RS Shifter controller */
#define HM_ATT_REL_BUT_NEUTRAL	"release gear button goes neutral"

/* Tell if the sequential shifter can go to neutral */
#define HM_ATT_SEQSHFT_ALLOW_NEUTRAL "sequential shifter allow neutral"
#define HM_ATT_SEQSHFT_ALLOW_REVERSE "sequential shifter allow reverse"

#define HM_ATT_STEER_SENS	"steer sensitivity"
#define HM_ATT_STEER_POW	"steer power"
#define HM_ATT_STEER_SPD	"steer speed sensitivity"
#define HM_ATT_STEER_DEAD	"steer dead zone"

#define HM_ATT_LEFTSTEER	"left steer"
#define HM_ATT_LEFTSTEER_MIN	"left steer min"
#define HM_ATT_LEFTSTEER_MAX	"left steer max"
#define HM_ATT_LEFTSTEER_POW	"left steer power"
#define HM_ATT_LEFTSTEER_DEAD	"left steer dead zone"

#define HM_ATT_RIGHTSTEER	"right steer"
#define HM_ATT_RIGHTSTEER_MIN	"right steer min"
#define HM_ATT_RIGHTSTEER_MAX	"right steer max"
#define HM_ATT_RIGHTSTEER_POW	"right steer power"
#define HM_ATT_RIGHTSTEER_DEAD	"right steer dead zone"

#define HM_ATT_THROTTLE		"throttle"
#define HM_ATT_THROTTLE_MIN	"throttle min"
#define HM_ATT_THROTTLE_MAX	"throttle max"
#define HM_ATT_THROTTLE_SENS	"throttle sensitivity"
#define HM_ATT_THROTTLE_POW	"throttle power"
#define HM_ATT_THROTTLE_DEAD	"throttle dead zone"

#define HM_ATT_BRAKE		"brake"
#define HM_ATT_BRAKE_MIN	"brake min"
#define HM_ATT_BRAKE_MAX	"brake max"
#define HM_ATT_BRAKE_SENS	"brake sensitivity"
#define HM_ATT_BRAKE_POW	"brake power"
#define HM_ATT_BRAKE_DEAD	"brake dead zone"

#define HM_ATT_CLUTCH		"clutch"
#define HM_ATT_CLUTCH_MIN	"clutch min"
#define HM_ATT_CLUTCH_MAX	"clutch max"
#define HM_ATT_CLUTCH_SENS	"clutch sensitivity"
#define HM_ATT_CLUTCH_POW	"clutch power"
#define HM_ATT_CLUTCH_DEAD	"clutch dead zone"

#define HM_ATT_HBOX_X		"hbox x"
#define HM_ATT_HBOX_X_MIN	"hbox x min"
#define HM_ATT_HBOX_X_MAX	"hbox x max"

#define HM_ATT_HBOX_Y		"hbox y"
#define HM_ATT_HBOX_Y_MIN	"hbox y min"
#define HM_ATT_HBOX_Y_MAX	"hbox y max"

#define HM_ATT_LEFTGLANCE       "left glance"
#define HM_ATT_L_GLANCE_MIN     "left glance min"
#define HM_ATT_L_GLANCE_MAX     "left glance max"
#define HM_ATT_RIGHTGLANCE      "right glance"
#define HM_ATT_R_GLANCE_MIN     "right glance min"
#define HM_ATT_R_GLANCE_MAX     "right glance max"

#define HM_ATT_DASHB_NEXT       "dashboard next item"
#define HM_ATT_DASHB_NEXT_MIN   "dashboard next item min"
#define HM_ATT_DASHB_NEXT_MAX   "dashboard next item max"
#define HM_ATT_DASHB_PREV       "dashboard previous item"
#define HM_ATT_DASHB_PREV_MIN   "dashboard previous item min"
#define HM_ATT_DASHB_PREV_MAX   "dashboard previous item max"
#define HM_ATT_DASHB_INC        "dashboard increase"
#define HM_ATT_DASHB_INC_MIN    "dashboard increase min"
#define HM_ATT_DASHB_INC_MAX    "dashboard increase max"
#define HM_ATT_DASHB_DEC        "dashboard decrease"
#define HM_ATT_DASHB_DEC_MIN    "dashboard decrease min"
#define HM_ATT_DASHB_DEC_MAX    "dashboard decrease max"

#define	HM_SECT_PREF	"Preferences"

#define HM_LIST_DRV	"Drivers"

#define HM_ATT_TRANS	"transmission"
#define HM_ATT_ABS	"ABS on"
#define HM_ATT_ASR	"ASR on"
#define HM_ATT_NBPITS	"programmed pit stops"

#define HM_VAL_AUTO	"auto"
#define HM_VAL_SEQ	"sequential"
#define HM_VAL_GRID	"grid"
#define HM_VAL_HBOX	"hbox"

#define HM_VAL_YES	"yes"
#define HM_VAL_NO	"no"

#define HM_VAL_JOYSTICK	"joystick"
#define HM_VAL_MOUSE	"mouse"
#define HM_VAL_KEYBOARD	"keyboard"

#define HM_ATT_JOY_PREF_BUT  0
#define HM_ATT_JOY_PREF_AXIS 1
#define HM_ATT_JOY_REQ_BUT   2
#define HM_ATT_JOY_REQ_AXIS  3
#define HM_ATT_JOY_PREF_ANY  4
#define HM_ATT_JOY_PREF_ANY1 5
#define HM_ATT_JOY_PREF_LAST 6

#define	CMD_UP_SHFT	0
#define	CMD_DN_SHFT	1
#define	CMD_ASR		2
#define	CMD_ABS		3
#define	CMD_GEAR_R	4
#define	CMD_GEAR_N	5
#define	CMD_GEAR_1	6
#define	CMD_GEAR_2	7
#define	CMD_GEAR_3	8
#define	CMD_GEAR_4	9
#define	CMD_GEAR_5	10
#define	CMD_GEAR_6	11
#define CMD_THROTTLE	12
#define CMD_BRAKE	13
#define CMD_LEFTSTEER	14
#define CMD_RIGHTSTEER	15
#define CMD_LIGHT1	16
#define CMD_CLUTCH	17
#define CMD_SPDLIM	18
#define CMD_EBRAKE	19
#define	CMD_HBOX_X	20
#define	CMD_HBOX_Y	21
#define CMD_LEFTGLANCE	22
#define CMD_RIGHTGLANCE	23
#define CMD_DASHB_NEXT 24
#define CMD_DASHB_PREV 25
#define CMD_DASHB_INC 26
#define CMD_DASHB_DEC 27

#define CMD_END_OF_LIST 27	/* Change this to same value as last item */

#endif /* _PLAYERPREF_H_ */ 



