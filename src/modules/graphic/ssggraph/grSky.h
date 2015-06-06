/***************************************************************************

    file        : grSky.h
    copyright   : (C) 2009 by Xavier Bertaux (based on ssgasky plib code)
    web         : http://www.speed-dreams.org
    version     : $Id: grSky.h 5353 2013-03-24 10:26:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _GRSKY_H_
#define _GRSKY_H_

#include "plib/sg.h"
#include "plib/ssg.h"

typedef struct 
{
	float *view_pos, *zero_elev, *view_up;
	double lon, lat, alt, spin;
	double gst;
	double sun_ra, sun_dec, sun_dist;
	double moon_ra, moon_dec, moon_dist;
	double sun_angle;
} cGrSkyState;

typedef struct 
{
	float *sky_color, *fog_color, *cloud_color;
	double sun_angle, moon_angle;
	int nplanets, nstars;
	sgdVec3 *planet_data, *star_data;
} cGrSkyColor;

class cGrCloudLayer;
class cGrCloudLayerList;
class cGrSun;
class cGrMoon;
class cGrStars;
class cGrSkyDome;
class cGrSky;


class cGrCloudLayer
{
private:

  ssgRoot *layer_root;
  ssgTransform *layer_transform;
  ssgLeaf *layer[4];

  ssgColourArray *cl[4]; 
  ssgVertexArray *vl[4];
  ssgTexCoordArray *tl[4];

  bool enabled;
  float layer_span;
  float layer_asl;
  float layer_thickness;
  float layer_transition;
  float scale;
  float speed;
  float direction;

  double last_lon, last_lat;
  double last_x, last_y;

public:

  cGrCloudLayer( void );
  ~cGrCloudLayer( void );

  void build( const char *cloud_tex_path, float span, float elevation, float thickness, float transition );
  void build( ssgSimpleState *cloud_state, float span, float elevation, float thickness, float transition );

  bool repositionFlat( sgVec3 p, double dt );
  bool reposition( sgVec3 p, sgVec3 up, double lon, double lat, double alt, double dt );

  bool repaint( sgVec3 fog_color );

  void draw();

  void enable() { enabled = true; }
  void disable() { enabled = false; }
  bool isEnabled() { return enabled; }

  float getElevation () { return layer_asl; }
  void  setElevation ( float elevation ) { layer_asl = elevation; }

  float getThickness () { return layer_thickness; }
  void  setThickness ( float thickness ) { layer_thickness = thickness; }

  float getTransition () { return layer_transition; }
  void  setTransition ( float transition ) { layer_transition = transition; }

  float getSpeed () { return speed; }
  void  setSpeed ( float val ) { speed = val; }

  float getDirection () { return direction; }
  void  setDirection ( float val ) { direction = val; }
};


class cGrCloudLayerList : private ssgSimpleList
{
public:

  cGrCloudLayerList ( int init = 3 )
	  : ssgSimpleList ( sizeof(cGrCloudLayer*), init ) { }

  ~cGrCloudLayerList () { removeAll(); }

  int getNum (void) { return total ; }

  cGrCloudLayer* get ( unsigned int n )
  {
    assert(n<total);
    return *( (cGrCloudLayer**) raw_get ( n ) ) ;
  }

  void add ( cGrCloudLayer* item ) { raw_add ( (char *) &item ) ;}

  void removeAll ()
  {
    for ( int i = 0; i < getNum (); i++ )
      delete get (i) ;
    ssgSimpleList::removeAll () ;
  }
};


class cGrMoon 
{
private:
    ssgTransform *moon_transform;
    ssgSimpleState *moon_state;

    ssgColourArray *moon_cl;

    double prev_moon_angle;
	double moon_angle;
    double moon_rotation;
	double moon_size;
	double moon_dist;
	double moonAscension;
	double moondeclination;


public:
    // Constructor
    cGrMoon( void );

    // Destructor
    ~cGrMoon( void );

    // build the moon object
    ssgBranch *build( double moon_size );

    // repaint the moon colors based on current value of moon_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = moon rise/set
    // 180 degrees = darkest midnight
	bool repaint() { return repaint ( moon_angle ); }
    bool repaint(double angle);

    bool reposition(sgVec3 p, double moon_angle) 
    {
       return reposition (p, moon_angle, moonAscension, moondeclination, moon_dist); 
    }

    bool reposition(sgVec3 p, double moon_angle, double moonAscension, double moondeclination, double moon_dist);


	void getMoonPosition (sgCoord* p)
    {
		sgMat4 Xform;
		moon_transform->getTransform(Xform);
		sgSetCoord(p, Xform);
    }

    void setMoonAngle (double angle) { moon_angle = angle; }
    double getMoonAngle () { return moon_angle; }

    void setMoonRotation (double rotation) { moon_rotation = rotation; }
    double getMoonRotation () { return moon_rotation; }

	void setMoonRightAscension ( double ra ) { moonAscension = ra; }
	double getMoonRightAscension () { return moonAscension; }

	void setMoonDeclination ( double decl ) { moondeclination = decl; }
	double getMoonDeclination () { return moondeclination; }

    void setMoonDist ( double dist ) { moon_dist = dist; }
    double getMoonDist() { return moon_dist; }
};

class cGrSun 
{
private:
    ssgTransform *sun_transform;
    ssgSimpleState *sun_state; 
    ssgSimpleState *ihalo_state;
    ssgSimpleState *ohalo_state;

    ssgColourArray *sun_cl;
    ssgColourArray *ihalo_cl;
    ssgColourArray *ohalo_cl;

    ssgVertexArray *ihalo_vl;
    ssgVertexArray *ohalo_vl;

    ssgTexCoordArray *ihalo_tl;
    ssgTexCoordArray *ohalo_tl;

    float visibility;
    double prev_sun_angle;
    double sun_angle;
    double sun_rotation;

    // used by reposition
    double sun_right_ascension;
    double sun_declination;
    double sun_dist;
    double path_distance;

public:

    // Constructor
    cGrSun( void );

    // Destructor
    ~cGrSun( void );

    // return the sun object
    ssgBranch *build( double sun_size );

    // repaint the sun colors based on current value of sun_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( double sun_angle, double new_visibility );

    // reposition the sun at the specified right ascension and
    // declination, offset by our current position (p) so that it
    // appears fixed at a great distance from the viewer.  Also add in
    // an optional rotation (i.e. for the current time of day.)

    /*bool reposition( sgVec3 p, double angle,
		     double rightAscension, double declination,
		     double sun_dist, double lat, double alt_asl, double sun_angle );*/

    bool reposition( sgVec3 p, double sun_angle ) 
    {
       return reposition ( p, sun_angle, sun_right_ascension, sun_declination, sun_dist ); 
    }

    bool reposition( sgVec3 p, double sun_angle, double sun_right_ascension, double sun_declination, double sun_dist );

    void getSunPosition (sgCoord* p)
    {
		sgMat4 Xform;
		sun_transform->getTransform(Xform);
		sgSetCoord(p, Xform);
    }

    void setSunAngle(double angle) { sun_angle = angle; }
    double getSunAngle() { return sun_angle; }

    void setSunRotation(double rotation) { sun_rotation = rotation; }
    double getSunRotation() { return sun_rotation; }

    void setSunRightAscension(double ra) { sun_right_ascension = ra; }
    double getSunRightAscension() { return sun_right_ascension; }

    void setSunDeclination( double decl ) { sun_declination = decl; }
    double getSunDeclination() { return sun_declination; }

    void setSunDistance( double dist ) { sun_dist = dist; }
    double getSunDistance() { return sun_dist; }

    // retrun the current color of the sun
    inline float *get_color() { return  ohalo_cl->get( 0 ); }
	double effective_visibility;
};


class cGrStars
{
private:

  ssgTransform *stars_transform;
  ssgSimpleState *state;

  ssgColourArray *cl;
  ssgVertexArray *vl;

  int old_phase;  // data for optimization

public:

  cGrStars( void );
  ~cGrStars( void );

  ssgBranch *build( int num, sgdVec3 *star_data, double star_dist );

  bool reposition( sgVec3 p, double angle );

  bool repaint( double sol_angle, int num, sgdVec3 *star_data );
};


class cGrSkyDome
{
private:

  ssgTransform *dome_transform;
  ssgSimpleState *dome_state;

  ssgVertexArray *center_disk_vl;
  ssgColourArray *center_disk_cl;

  ssgVertexArray *upper_ring_vl;
  ssgColourArray *upper_ring_cl;

  ssgVertexArray *middle_ring_vl;
  ssgColourArray *middle_ring_cl;

  ssgVertexArray *lower_ring_vl;
  ssgColourArray *lower_ring_cl;
  float asl;

public:

  cGrSkyDome( void );
  ~cGrSkyDome( void );

  ssgBranch *build( double hscale = 80000.0, double vscale = 80000.0 );

  bool repositionFlat( sgVec3 p, double spin );
  bool reposition( sgVec3 p, double lon, double lat, double spin );

  bool repaint( sgVec3 sky_color, sgVec3 fog_color, double sol_angle, double vis );
};


class cGrSky
{
private:
	// components of the sky
	cGrSkyDome *dome;
	cGrSun	*sun;
	cGrMoon	*moon;
	cGrCloudLayerList clouds;
	cGrStars *planets;
	cGrStars *stars;

	ssgRoot *pre_root, *post_root;

	ssgSelector *pre_selector, *post_selector;
	ssgTransform *pre_transform, *post_transform;
	ssgTransform *sun_transform, *moon_transform, *stars_transform;

	// visibility
	float visibility;
	float effective_visibility;

	// near cloud visibility state variables
	bool in_puff;
	double puff_length;       // in seconds
	double puff_progression;  // in seconds
	double ramp_up;           // in seconds
	double ramp_down;         // in seconds

public:

	cGrSky( void );
	~cGrSky( void );

	void build( double h_radius, double v_radius,
	  double sun_size, double sun_dist, 
	  double moon_size, double moon_dist,
	  int nplanets, sgdVec3 *planet_data,
	  int nstars, sgdVec3 *star_data );

	cGrCloudLayer* addCloud( const char *cloud_tex_path, float span, float elevation, float thickness, float transition );
	cGrCloudLayer* addCloud( ssgSimpleState *cloud_state, float span, float elevation, float thickness, float transition );
	cGrCloudLayer* getCloud(int i) { return clouds.get(i); }
	int getCloudCount() { return clouds.getNum(); }

	bool repositionFlat( sgVec3 view_pos, double spin, double dt );
	bool reposition( sgVec3 view_pos, sgVec3 zero_elev, sgVec3 view_up, double lon, double lat, double alt, double spin, double gst, double dt );

	bool repaint( sgVec4 sky_color, sgVec4 fog_color, sgVec4 cloud_color, double sol_angle, double moon_angle,
	  int nplanets, sgdVec3 *planet_data,
	  int nstars, sgdVec3 *star_data );

	// modify visibility based on cloud layers, thickness, transition range, and simulated "puffs".
	void modifyVisibility( float alt, float time_factor );

	void setMA(double angle) { moon->setMoonAngle(angle); }
	double getMA() { return moon->getMoonAngle(); }

	void setMR(double rotation) { moon->setMoonRotation( rotation); }
	double getMR() { return moon->getMoonRotation(); }
	void setMRA( double ra ) { moon->setMoonRightAscension( ra ); }
	double getMRA() { return moon->getMoonRightAscension(); }

	void setMD( double decl ) { moon->setMoonDeclination( decl ); }
	double getMD() { return moon->getMoonDeclination(); }

    void setMDist( double dist ) { moon->setMoonDist(dist); }
    double getMDist() { return moon->getMoonDist(); }

    void setSA(double angle) { sun->setSunAngle(angle); }
    double getSA() { return sun->getSunAngle(); }

    void setSR(double rotation) { sun->setSunRotation( rotation ); }
    double getSR() { return sun->getSunRotation(); }

    void setSRA(double ra) { sun->setSunRightAscension( ra ); }
    double getSRA() { return sun->getSunRightAscension(); }

    void setSD( double decl ) { sun->setSunDeclination( decl ); }
    double getSD() { return sun->getSunDeclination(); }

    void setSDistance( double dist ) { sun->setSunDistance( dist ); }
    double getSDistance() { return sun->getSunDistance(); }

	void getSunPos(sgCoord* p) 
	{
		sun->getSunPosition( p);
    }

	// draw background portions of sky (do this before you draw rest of your scene).
	void preDraw();

	// draw translucent clouds (do this after you've drawn all oapaque elements of your scene).
	void postDraw( float alt );



	// enable the sky
	inline void enable() 
	{
		pre_selector->select( 1 );
		post_selector->select( 1 );
	}

	// disable the sky
	inline void disable() 
	{
		pre_selector->select( 0 );
		post_selector->select( 0 );
	}

    inline float *get_sun_color() { return sun->get_color(); }

	// current effective visibility
	inline float getVisibility() const { return effective_visibility; }
	inline void setVisibility( float v ) 
	{
		effective_visibility = visibility = (v <= 25.0f) ? 25.0f : v;
	}
};


// return a random number between [0.0, 1.0)
inline double grRandom(void)
{
  return(rand() / (double)RAND_MAX);
}

//#if defined( macintosh )
//const float system_gamma = 1.4;

const float system_gamma = 2.5;

// simple architecture independant gamma correction function.
inline void grGammaCorrectRGB(float *color, float reff = 2.5, float system = system_gamma)
{
	if (reff == system)
       return;
    //float tmp = reff/system;
	color[0] = (float)pow(color[0], reff/system);
	color[1] = (float)pow(color[1], reff/system);
	color[2] = (float)pow(color[2], reff/system);
};

inline void grGammaCorrectC(float *color, float reff = 2.5, float system = system_gamma)
{
  *color = (float)pow(*color, reff/system);
};

inline void grGammaRestoreRGB(float *color, float reff = 2.5, float system = system_gamma)
{
  color[0] = (float)pow(color[0], system/reff);
  color[1] = (float)pow(color[1], system/reff);
  color[2] = (float)pow(color[2], system/reff);
};

inline void grGammaRestoreC(float *color, float reff = 2.5, float system = system_gamma)
{
  *color = (float)pow(*color, system/reff);
};

#endif
