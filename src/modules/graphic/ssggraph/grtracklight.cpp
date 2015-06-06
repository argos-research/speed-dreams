#ifndef TRUE
#define TRUE 1
#endif //TRUE
#ifndef FALSE
#define FALSE 0
#endif //FALSE

#include <plib/ssg.h>
#include <plib/sg.h>
#include <raceman.h>
#include <track.h>

#include "grscene.h"

typedef struct LightInfo
{
	int index;
	ssgVtxTable *light;
	//ssgSimpleState* onState;
	//ssgSimpleState* offState;
	ssgStateSelector *states;
	struct LightInfo *next;
} tLightInfo;

typedef struct TrackLights
{
	tLightInfo *st_red;
	tLightInfo *st_green;
	tLightInfo *st_yellow;
	tLightInfo *st_green_st;
} tTrackLights;

typedef struct StateList
{
	ssgSimpleState *state;
	struct StateList *next;
} tStateList;

static tStateList *statelist;
static ssgBranch *lightBranch;
static tTrackLights trackLights;

//static void setOnOff( tLightInfo *light, char onoff );

static void calcNorm( sgVec3 topleft, sgVec3 bottomright, sgVec3 *result )
{
	(*result)[ 0 ] = bottomright[ 1 ] - topleft[ 1 ];
	(*result)[ 1 ] = topleft[ 0 ] - bottomright[ 0 ];
	(*result)[ 2 ] = 0.0f;
}

static ssgSimpleState* createState( char const *filename )
{
	tStateList *current = statelist;

	while( current && current->state )
	{
		if( strcmp( filename, current->state->getTextureFilename() ) == 0 )
		{
			return current->state;
		}
		current = current->next;
	}

	current = (tStateList*)malloc( sizeof( tStateList ) );
	current->state = new ssgSimpleState();
	if( !current->state )
	{
		free( current );
		return NULL;
	}

	current->state->disable( GL_LIGHTING );
	current->state->enable( GL_BLEND );
	current->state->enable( GL_CULL_FACE );
	current->state->enable( GL_TEXTURE_2D );
	current->state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
	current->state->setTexture( filename, TRUE, TRUE, TRUE );
	current->state->ref();
	current->next = statelist;
	statelist = current;

	return current->state;
}

static void deleteStates()
{
	tStateList *current = statelist;
	tStateList *next;

	while( current )
	{
		next = current->next;

		if( current->state )
		{
			current->state->deRef();
			delete current->state;
		}
		free( current );
		current = next;
	}
}

static void addLight( tGraphicLightInfo *info, tTrackLights *lights, ssgBranch *parent )
{
	tLightInfo *trackLight;
	int states = 2;

	ssgVertexArray *vertexArray = new ssgVertexArray( 4 );
	ssgNormalArray *normalArray = new ssgNormalArray( 4 );
	ssgColourArray *colourArray = new ssgColourArray( 4 );
	ssgTexCoordArray *texArray = new ssgTexCoordArray( 4 );

	sgVec3 vertex;
	sgVec3 normal;
	sgVec4 colour;
	sgVec2 texcoord;

	colour[ 0 ] = info->red;
	colour[ 1 ] = info->green;
	colour[ 2 ] = info->blue;
	colour[ 3 ] = 1.0f;
	colourArray->add( colour );
	colourArray->add( colour );
	colourArray->add( colour );
	colourArray->add( colour );

	vertex[ 0 ] = info->topleft.x;
	vertex[ 1 ] = info->topleft.y;
	vertex[ 2 ] = info->topleft.z;
	vertexArray->add( vertex );
	vertex[ 2 ] = info->bottomright.z;
	vertexArray->add( vertex );
	vertex[ 0 ] = info->bottomright.x;
	vertex[ 1 ] = info->bottomright.y;
	vertex[ 2 ] = info->topleft.z; //?
	vertexArray->add( vertex );
	vertex[ 2 ] = info->topleft.z;
	vertex[ 2 ] = info->bottomright.z; //?
	vertexArray->add( vertex );

	calcNorm( vertexArray->get( 0 ), vertexArray->get( 2 ), &normal );
	normalArray->add( normal );
	normalArray->add( normal );
	normalArray->add( normal );
	normalArray->add( normal );

	texcoord[ 0 ] = 0.0f;
	texcoord[ 1 ] = 0.0f;
	texArray->add( texcoord );
	texcoord[ 0 ] = 0.0f;
	texcoord[ 1 ] = 1.0f;
	texArray->add( texcoord );
	texcoord[ 0 ] = 1.0f;
	texcoord[ 1 ] = 0.0f;
	texArray->add( texcoord );
	texcoord[ 0 ] = 1.0f;
	texcoord[ 1 ] = 1.0f;
	texArray->add( texcoord );

	if( info->role == GR_TRACKLIGHT_START_YELLOW || info->role == GR_TRACKLIGHT_POST_YELLOW ||
	    info->role == GR_TRACKLIGHT_POST_GREEN || info->role == GR_TRACKLIGHT_POST_RED ||
	    info->role == GR_TRACKLIGHT_POST_BLUE || info->role == GR_TRACKLIGHT_POST_WHITE ||
	    info->role == GR_TRACKLIGHT_PIT_BLUE )
	{
		states = 3;
	}

	trackLight = (tLightInfo*)malloc( sizeof( tLightInfo ) );
	trackLight->index = info->index;
	trackLight->light = new ssgVtxTable( GL_TRIANGLE_STRIP, vertexArray, normalArray, texArray, colourArray );
	trackLight->states = new ssgStateSelector( states );
	trackLight->states->setStep( 0, createState( info->offTexture ) );
	trackLight->states->setStep( 1 + MAX( states - 2, info->index % 2 ), createState( info->onTexture ) );
	if( states == 3 )
		trackLight->states->setStep( 1 + ( info->index + 1 ) % 2, createState( info->offTexture ) );
	trackLight->states->selectStep( 0 );
	trackLight->light->setState( trackLight->states );
	//trackLight->onState = createState( info->onTexture );
	//trackLight->offState = createState( info->offTexture );

	switch( info->role )
	{
	case GR_TRACKLIGHT_START_RED:
		trackLight->next = lights->st_red;
		lights->st_red = trackLight;
		break;
	case GR_TRACKLIGHT_START_GREEN:
		trackLight->next = lights->st_green;
		lights->st_green = trackLight;
		break;
	case GR_TRACKLIGHT_START_GREENSTART:
		trackLight->next = lights->st_green_st;
		lights->st_green_st = trackLight;
		break;
	case GR_TRACKLIGHT_START_YELLOW:
		trackLight->next = lights->st_yellow;
		lights->st_yellow = trackLight;
		break;
	case GR_TRACKLIGHT_POST_YELLOW:
	case GR_TRACKLIGHT_POST_GREEN:
	case GR_TRACKLIGHT_POST_RED:
	case GR_TRACKLIGHT_POST_BLUE:
	case GR_TRACKLIGHT_POST_WHITE:
	case GR_TRACKLIGHT_PIT_RED:
	case GR_TRACKLIGHT_PIT_GREEN:
	case GR_TRACKLIGHT_PIT_BLUE:
	default:
		delete trackLight->light;
		free( trackLight );
		return;
	}

	parent->addKid( trackLight->light );
}

static void addLights( tTrackLights *lights, ssgBranch *parent )
{
	int xx;

	for( xx = 0; xx < grTrack->graphic.nb_lights; ++xx )
		addLight( &(grTrack->graphic.lights[ xx ]), lights, parent );
}

static void manageStartLights( tTrackLights *startlights, tSituation *s, char phase )
{
	static int  onoff_red_index = -1;
	static char onoff_red = FALSE;
	static char onoff_green = FALSE;
	static char onoff_green_st = FALSE;
	static char onoff_yellow = FALSE;
	static char onoff_phase = 1;
	char onoff;
	int current_index;
	char active = s->currentTime >= 0.0f && ( s->_totTime < 0.0f || s->currentTime < s->_totTime );
	tLightInfo *current;
	
	if( s->currentTime < 0.0f )
		current_index = (int)floor( s->currentTime * -10.0f );
	else
		current_index = -1;
	
	current = startlights->st_red;
	onoff = !active && s->_raceType != RM_TYPE_RACE;
	if( current_index != onoff_red_index || onoff != onoff_red )
	{
		onoff_red_index = current_index;
		onoff_red = onoff;
		while( current )
		{
			//setOnOff( current, onoff || ( current_index >= 0 && current_index < current->index ) );
			current->states->selectStep( ( onoff || ( current_index >= 0 && current_index < current->index ) ) ? 1 : 0 );
			current = current->next;
		}
	}

	current = startlights->st_green;
	onoff = active && s->_raceType != RM_TYPE_RACE;
	if( onoff_green != onoff )
	{
		onoff_green = onoff;
		while( current )
		{
			//setOnOff( current, onoff );
			current->states->selectStep( onoff ? 1 : 0 );
			current = current->next;
		}
	}

	current = startlights->st_green_st;
	onoff = active && ( s->_raceType != RM_TYPE_RACE || s->currentTime < 30.0f );
	if( onoff_green_st != onoff )
	{
		onoff_green_st = onoff;
		while( current )
		{
			//setOnOff( current, onoff );
			current->states->selectStep( onoff ? 1 : 0 );
			current = current->next;
		}
	}

	current = startlights->st_yellow;
	onoff = FALSE;
	if( onoff_yellow != onoff || ( onoff && phase != onoff_phase ) )
	{
		onoff_yellow = onoff;
		while( current )
		{
			//setOnOff( current, onoff ? ( phase + current->index ) % 2 : 0 );
			current->states->selectStep( onoff ? phase : 0 );
			current = current->next;
		}
	}

	onoff_phase = phase;
}

/*static void setOnOff( tLightInfo *light, char onoff )
{
	ssgSimpleState *sstate;
	if( !light )
		return;

	sstate = onoff ? light->onState : light->offState;
	if( light->light != NULL && light->light->getState() != sstate )
		light->light->setState( sstate );
}*/

void grTrackLightInit()
{
	statelist = NULL;
	lightBranch = new ssgBranch();
	TrackLightAnchor->addKid( lightBranch );
	memset( &trackLights, 0, sizeof( tTrackLights ) );
	addLights( &trackLights, lightBranch );
}

void grTrackLightUpdate( tSituation *s )
{
	char phase = (char)( ( (int)floor( fmod( s->currentTime + 120.0f, (double)0.3f ) / 0.3f ) % 2 ) + 1 );
	manageStartLights( &trackLights, s, phase );
}

void grTrackLightShutdown()
{
	TrackLightAnchor->removeAllKids();
	//lightBranch->removeAllKids();
	/*delete lightBranch;*/ lightBranch = NULL;
	deleteStates();
}

