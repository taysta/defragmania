// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#include "../ui/ui_shared.h"
//speedometer functions
static void CG_CalculateSpeed(centity_t* cent);
static void CG_GraphAddSpeed(void);
static void CG_DrawSpeedGraph(rectDef_t* rect, const vec4_t foreColor, vec4_t backColor);
static void CG_DrawJumpHeight(centity_t* cent);
static void CG_DrawJumpDistance(void);
static void CG_DrawVerticalSpeed(void);
static void CG_DrawAccelMeter(void);
static void CG_DrawSpeedometer(void);
//strafehelper functions
static void	CG_DrawLine(float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff);
static void CG_DrawStrafehelperWeze(int moveDir);
static void CG_StrafeHelperSound(float difference);
static void CG_DrawStrafeLine(vec3_t velocity, float diff, qboolean active, int moveDir);
static int	CG_GetStrafeTriangleAccel(void);
static float* CG_GetStrafeTriangleStrafeX(vec3_t velocity, float diff, float sensitivity);
static void CG_DrawStrafeTriangles(vec3_t velocity, float diff, float baseSpeed, int moveDir);
qboolean	CG_InRollAnim(centity_t* cent);
static void CG_StrafeHelper(centity_t* cent);
//movementkeys functions
static float CG_GetGroundDistance(void);
static void CG_DrawMovementKeys(centity_t* cent);
//strafehelper options
#define SHELPER_UPDATED			(1<<0)
#define SHELPER_CGAZ			(1<<1)
#define SHELPER_WSW				(1<<2)
#define SHELPER_WEZE			(1<<3)
#define SHELPER_SOUND			(1<<4)
#define SHELPER_W				(1<<5)
#define SHELPER_WA				(1<<6)
#define SHELPER_WD				(1<<7)
#define SHELPER_A				(1<<8)
#define SHELPER_D				(1<<9)
#define SHELPER_S				(1<<10)
#define SHELPER_SA				(1<<11)
#define SHELPER_SD				(1<<12)
#define SHELPER_REAR			(1<<13)
#define SHELPER_CENTER			(1<<14)
#define SHELPER_INVERT			(1<<15)
#define SHELPER_CROSSHAIR		(1<<16)
#define SHELPER_TINY		    (1<<17)
#define SHELPER_ACCELZONES      (1<<18)
//speedometer options
#define SPEEDOMETER_ENABLE			(1<<0)
#define SPEEDOMETER_GROUNDSPEED		(1<<1)
#define SPEEDOMETER_JUMPHEIGHT		(1<<2)
#define SPEEDOMETER_JUMPDISTANCE	(1<<3)
#define SPEEDOMETER_VERTICALSPEED	(1<<4)
#define SPEEDOMETER_ACCELMETER		(1<<5)
#define SPEEDOMETER_SPEEDGRAPH		(1<<6)
#define SPEEDOMETER_COLORS          (1<<7)
#define SPEEDOMETER_JUMPS			(1<<8)
#define SPEEDOMETER_JUMPSCOLORS1    (1<<9)
#define SPEEDOMETER_JUMPSCOLORS2    (1<<10)
#define SPEEDOMETER_KPH				(1<<11)
#define SPEEDOMETER_MPH				(1<<12)
//misc strafehelper/speedometer defines
#define	SPEED_SAMPLES	256
#define SPEEDOMETER_NUM_SAMPLES 500
#define SPEEDOMETER_MIN_RANGE 900
#define SPEED_MED 1000.f
#define SPEED_FAST 1600.f
#define ACCEL_SAMPLES 16
#define PERCENT_SAMPLES 16
#define ACCEL_SAMPLE_COUNT 16
#define ACCEL_SAMPLE_MASK (ACCEL_SAMPLE_COUNT-1)
#define VectorLerp( f, s, e, r ) ((r)[0]=(s)[0]+(f)*((e)[0]-(s)[0]),\
  (r)[1]=(s)[1]+(f)*((e)[1]-(s)[1]),\
  (r)[2]=(s)[2]+(f)*((e)[2]-(s)[2]))
//misc strafehelper/speedometer variables
static float speedometerXPos, jumpsXPos;
static float speedSamples[SPEEDOMETER_NUM_SAMPLES];
static int oldestSpeedSample = 0;
static int maxSpeedSample = 0;
static float firstSpeed;

qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, float *x, float *y);
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle );

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
vec4_t	bluehudtint = {0.5, 0.5, 1.0, 1.0};
vec4_t	redhudtint = {1.0, 0.5, 0.5, 1.0};
float	*hudTintColor;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

int lastvalidlockdif;

extern float zoomFov; //this has to be global client-side

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

char *showPowersName[] = 
{
	"HEAL2",//FP_HEAL
	"JUMP2",//FP_LEVITATION
	"SPEED2",//FP_SPEED
	"PUSH2",//FP_PUSH
	"PULL2",//FP_PULL
	"MINDTRICK2",//FP_TELEPTAHY
	"GRIP2",//FP_GRIP
	"LIGHTNING2",//FP_LIGHTNING
	"DARK_RAGE2",//FP_RAGE
	"PROTECT2",//FP_PROTECT
	"ABSORB2",//FP_ABSORB
	"TEAM_HEAL2",//FP_TEAM_HEAL
	"TEAM_REPLENISH2",//FP_TEAM_FORCE
	"DRAIN2",//FP_DRAIN
	"SEEING2",//FP_SEE
	"SABER_OFFENSE2",//FP_SABERATTACK
	"SABER_DEFENSE2",//FP_SABERDEFEND
	"SABER_THROW2",//FP_SABERTHROW
	NULL
};


int MenuFontToHandle(int iMenuFont)
{
	switch (iMenuFont)
	{
		case FONT_SMALL:	return cgDC.Assets.qhSmallFont;
		case FONT_MEDIUM:	return cgDC.Assets.qhMediumFont;
		case FONT_LARGE:	return cgDC.Assets.qhBigFont;
	}

	return cgDC.Assets.qhMediumFont;
}

int CG_Text_Width(const char *text, float scale, int iMenuFont) 
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_StrLenPixels(text, iFontIndex, scale);
}

int CG_Text_Height(const char *text, float scale, int iMenuFont) 
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_HeightPixels(iFontIndex, scale);
}

#include "../qcommon/qfiles.h"	// for STYLE_BLINK etc
void CG_Text_Paint(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont)
{
	int iStyleOR = 0;
	int iFontIndex = MenuFontToHandle(iMenuFont);
	
	switch (style)
	{
	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	}

	trap_R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!limit?-1:limit,		// iCharLimit (-1 = none)
							scale	// const float scale = 1.0f
							);
}

/*
================
CG_DrawZoomMask

================
*/
static void CG_DrawZoomMask( void )
{
	vec4_t		color1;
	float		level;
	static qboolean	flip = qtrue;

//	int ammo = cg_entities[0].gent->client->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex];
	float cx, cy;
//	int val[5];
	float max, fi;

	// Check for Binocular specific zooming since we'll want to render different bits in each case
	if ( cg.predictedPlayerState.zoomMode == 2 )
	{
		int val, i;
		float off;

		// zoom level
		level = (float)(80.0f - cg.predictedPlayerState.zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		CG_WideScreenMode(qfalse);

		// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
		trap_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( 34, 48, 570, 362, cgs.media.binocularStatic );
	
		// Black out the area behind the numbers
		trap_R_SetColor( colorTable[CT_BLACK]);
		CG_DrawPic( 212, 367, 200, 40, cgs.media.whiteShader );

		// Numbers should be kind of greenish
		color1[0] = 0.2f;
		color1[1] = 0.4f;
		color1[2] = 0.2f;
		color1[3] = 0.3f;
		trap_R_SetColor( color1 );

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		val = ((int)((cg.refdefViewAngles[YAW] + 180) / 10)) * 10;
		off = (cg.refdefViewAngles[YAW] + 180) - val;

		for ( i = -10; i < 30; i += 10 )
		{
			val -= 10;

			if ( val < 0 )
			{
				val += 360;
			}

			// we only want to draw the very far left one some of the time, if it's too far to the left it will
			//	poke outside the mask.
			if (( off > 3.0f && i == -10 ) || i > -10 )
			{
				// draw the value, but add 200 just to bump the range up...arbitrary, so change it if you like
				CG_DrawNumField( 155 + i * 10 + off * 10, 374, 3, val + 200, 24, 14, NUM_FONT_CHUNKY, qtrue );
				CG_DrawPic( 245 + (i-1) * 10 + off * 10, 376, 6, 6, cgs.media.whiteShader );
			}
		}

		CG_DrawPic( 212, 367, 200, 28, cgs.media.binocularOverlay );

		color1[0] = sin( cg.time * 0.01f ) * 0.5f + 0.5f;
		color1[0] = color1[0] * color1[0];
		color1[1] = color1[0];
		color1[2] = color1[0];
		color1[3] = 1.0f;

		trap_R_SetColor( color1 );

		CG_DrawPic( 82, 94, 16, 16, cgs.media.binocularCircle );

		// Flickery color
		color1[0] = 0.7f + crandom() * 0.1f;
		color1[1] = 0.8f + crandom() * 0.1f;
		color1[2] = 0.7f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap_R_SetColor( color1 );
	
		CG_DrawPic( 0, 0, 640, 480, cgs.media.binocularMask );

		CG_DrawPic( 4, 282 - level, 16, 16, cgs.media.binocularArrow );

		// The top triangle bit randomly flips 
		if ( flip )
		{
			CG_DrawPic( 330, 60, -26, -30, cgs.media.binocularTri );
		}
		else
		{
			CG_DrawPic( 307, 40, 26, 30, cgs.media.binocularTri );
		}

		if ( random() > 0.98f && ( cg.time & 1024 ))
		{
			flip = !flip;
		}

		CG_WideScreenMode(qtrue);
	}
	else if ( cg.predictedPlayerState.zoomMode)
	{
		float xOffset = 0.5f * (cgs.screenWidth - SCREEN_WIDTH);
		float yOffset = 0.5f * (cgs.screenHeight - SCREEN_HEIGHT);

		// Fill the left and right
		trap_R_SetColor(colorTable[CT_BLACK]);
		trap_R_DrawStretchPic(0, 0, xOffset, SCREEN_HEIGHT, 0, 0, 0, 0, cgs.media.whiteShader);
		trap_R_DrawStretchPic(xOffset + SCREEN_WIDTH, 0, xOffset, SCREEN_HEIGHT, 0, 0, 0, 0, cgs.media.whiteShader);

		// Fill the top and bottom
		trap_R_DrawStretchPic(0, 0, SCREEN_WIDTH, yOffset, 0, 0, 0, 0, cgs.media.whiteShader);
		trap_R_DrawStretchPic(0, yOffset + SCREEN_HEIGHT, SCREEN_WIDTH, yOffset, 0, 0, 0, 0, cgs.media.whiteShader);

		// Draw target mask
		trap_R_SetColor(colorTable[CT_WHITE]);
		trap_R_DrawStretchPic(xOffset, yOffset, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1, 1, cgs.media.disruptorMask);
		trap_R_SetColor(NULL);

		// disruptor zoom mode
		level = (float)(50.0f - zoomFov) / 50.0f;//(float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork. 
		level *= 103.0f;

		// apparently 99.0f is the full zoom level
		if ( level >= 99 )
		{
			// Fully zoomed, so make the rotating insert pulse
			color1[0] = 1.0f; 
			color1[1] = 1.0f;
			color1[2] = 1.0f;
			color1[3] = 0.7f + sin( cg.time * 0.01f ) * 0.3f;

			trap_R_SetColor( color1 );
		}

		// Draw rotating insert
		CG_DrawRotatePic2(0.5f * cgs.screenWidth, 0.5f * cgs.screenHeight, SCREEN_WIDTH, SCREEN_HEIGHT, -level, cgs.media.disruptorInsert);

		// Increase the light levels under the center of the target
//		CG_DrawPic( 198, 118, 246, 246, cgs.media.disruptorLight );

		// weirdness.....converting ammo to a base five number scale just to be geeky.
/*		val[0] = ammo % 5;
		val[1] = (ammo / 5) % 5;
		val[2] = (ammo / 25) % 5;
		val[3] = (ammo / 125) % 5;
		val[4] = (ammo / 625) % 5;
		
		color1[0] = 0.2f;
		color1[1] = 0.55f + crandom() * 0.1f;
		color1[2] = 0.5f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap_R_SetColor( color1 );

		for ( int t = 0; t < 5; t++ )
		{
			cx = 320 + sin( (t*10+45)/57.296f ) * 192;
			cy = 240 + cos( (t*10+45)/57.296f ) * 192;

			CG_DrawRotatePic2( cx, cy, 24, 38, 45 - t * 10, trap_R_RegisterShader( va("gfx/2d/char%d",val[4-t] )));
		}
*/
		//max = ( cg_entities[0].gent->health / 100.0f );

		max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / (float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max;
		if ( max > 1.0f )
		{
			max = 1.0f;
		}

		color1[0] = (1.0f - max) * 2.0f; 
		color1[1] = max * 1.5f;
		color1[2] = 0.0f;
		color1[3] = 1.0f;

		// If we are low on health, make us flash
		if ( max < 0.15f && ( cg.time & 512 ))
		{
			VectorClear( color1 );
		}

		if ( color1[0] > 1.0f )
		{
			color1[0] = 1.0f;
		}

		if ( color1[1] > 1.0f )
		{
			color1[1] = 1.0f;
		}

		trap_R_SetColor( color1 );

		max *= 58.0f;

		for (fi = 18.5f; fi <= 18.5f + max; fi+= 3 ) // going from 15 to 45 degrees, with 5 degree increments
		{
			cx = (SCREEN_WIDTH / 2) + sin( (fi+90.0f)/57.296f ) * 190;
			cy = (SCREEN_HEIGHT / 2) + cos( (fi+90.0f)/57.296f ) * 190;

			CG_DrawRotatePic2(xOffset + cx, yOffset + cy, 12, 24, 90 - fi, cgs.media.disruptorInsertTick);
		}

		if ( cg.predictedPlayerState.weaponstate == WEAPON_CHARGING_ALT )
		{
			trap_R_SetColor( colorTable[CT_WHITE] );

			// draw the charge level
			max = ( cg.time - cg.predictedPlayerState.weaponChargeTime ) / ( 50.0f * 30.0f ); // bad hardcodedness 50 is disruptor charge unit and 30 is max charge units allowed.

			if ( max > 1.0f )
			{
				max = 1.0f;
			}

			trap_R_DrawStretchPic(xOffset + 257, yOffset + 435, 134 * max, 34, 0, 0, max, 1, cgs.media.disruptorChargeShader);
		}
//		trap_R_SetColor( colorTable[CT_WHITE] );
//		CG_DrawPic( 0, 0, 640, 480, cgs.media.disruptorMask );

	}
}


/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;
	float			xScale, yScale;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	xScale = (float) cgs.glconfig.vidWidth / cgs.screenWidth;
	yScale = (float) cgs.glconfig.vidHeight / cgs.screenHeight;
	refdef.x = x * xScale;
	refdef.y = y * yScale;
	refdef.width = w * xScale;
	refdef.height = h * yScale;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) 
{
	clientInfo_t	*ci;

	ci = &cgs.clientinfo[ clientNum ];

	CG_DrawPic( x, y, w, h, ci->modelIcon );

	// if they are deferred, draw a cross out
	if ( ci->deferred ) 
	{
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawHUDLeftFrame1
================
*/
void CG_DrawHUDLeftFrame1(float x, float y)
{
	// Inner gray wire frame
	trap_R_SetColor( hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDInnerLeft );			
}

/*
================
CG_DrawHUDLeftFrame2
================
*/
void CG_DrawHUDLeftFrame2(float x, float y)
{
	// Inner gray wire frame
	trap_R_SetColor( hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeftFrame );		// Metal frame
}

/*
================
DrawHealthArmor
================
*/
void DrawHealthArmor(float x, float y)
{
	vec4_t calcColor;
	float	armorPercent,hold,healthPercent;
	playerState_t	*ps;

	int healthAmt;
	int armorAmt;

	ps = &cg.snap->ps;

	healthAmt = ps->stats[STAT_HEALTH];
	armorAmt = ps->stats[STAT_ARMOR];

	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}

	if (armorAmt > 100)
	{
		armorAmt = 100;
	}

	trap_R_SetColor( colorTable[CT_WHITE] );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeftFrame );		// Circular black background

	//	Outer Armor circular
	memcpy(calcColor, colorTable[CT_GREEN], sizeof(vec4_t));

	hold = armorAmt-(ps->stats[STAT_MAX_HEALTH]/2);
	armorPercent = (float) hold/(ps->stats[STAT_MAX_HEALTH]/2);
	if (armorPercent <0)
	{
		armorPercent = 0;
	}
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);					
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor1 );			

	// Inner Armor circular
	if (armorPercent>0)
	{
		armorPercent = 1;
	}
	else
	{
		armorPercent = (float) armorAmt/(ps->stats[STAT_MAX_HEALTH]/2);
	}
	memcpy(calcColor, colorTable[CT_GREEN], sizeof(vec4_t));
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);					
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor2 );			//	Inner Armor circular

	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		// Make tic flash if inner armor is at 50% (25% of full armor)
		if (armorPercent<.5)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 100;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

	memcpy(calcColor, colorTable[CT_RED], sizeof(vec4_t));
	healthPercent = (float) healthAmt/ps->stats[STAT_MAX_HEALTH];
	calcColor[0] *= healthPercent;
	calcColor[1] *= healthPercent;
	calcColor[2] *= healthPercent;
	trap_R_SetColor( calcColor);					
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealth );

	// Make tic flash if health is at 20% of full
	if (healthPercent>.20)
	{
		cg.HUDHealthFlag=qtrue;
	}
	else
	{
		if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
		{
			cg.HUDTickFlashTime = cg.time + 100;

			if ((armorPercent>0) && (armorPercent<.5))		// Keep the tics in sync if flashing
			{
				cg.HUDHealthFlag=cg.HUDArmorFlag;
			}
			else
			{
				if (cg.HUDHealthFlag)
				{
					cg.HUDHealthFlag = qfalse;
				}
				else
				{
					cg.HUDHealthFlag = qtrue;
				}
			}
		}
	}

	// Draw the ticks
	if (cg.HUDHealthFlag)
	{
		trap_R_SetColor( colorTable[CT_RED] );					
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealthTic );
	}

	if (cg.HUDArmorFlag)
	{
		trap_R_SetColor( colorTable[CT_GREEN] );					
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmorTic );		//	
	}

	trap_R_SetColor(hudTintColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeftStatic );		//	

	trap_R_SetColor( colorTable[CT_RED] );	
	CG_DrawNumField (x + 16, y + 40, 3, ps->stats[STAT_HEALTH], 14, 18, 
		NUM_FONT_SMALL,qfalse);

	trap_R_SetColor( colorTable[CT_GREEN] );	
	CG_DrawNumField (x + 18 + 14, y + 40 + 14, 3, ps->stats[STAT_ARMOR], 14, 18, 
		NUM_FONT_SMALL,qfalse);

	trap_R_SetColor(hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeft );			// Metal frame
}

/*
================
CG_DrawHealth
================
*/
void CG_DrawHealth(float x, float y)
{
	vec4_t calcColor;
	float	healthPercent;
	playerState_t	*ps;
	int healthAmt;

	ps = &cg.snap->ps;

	healthAmt = ps->stats[STAT_HEALTH];

	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}

	memcpy(calcColor, colorTable[CT_HUD_RED], sizeof(vec4_t));
	healthPercent = (float) healthAmt/ps->stats[STAT_MAX_HEALTH];
	calcColor[0] *= healthPercent;
	calcColor[1] *= healthPercent;
	calcColor[2] *= healthPercent;
	trap_R_SetColor( calcColor);					
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealth );

	// Draw the ticks
	if (cg.HUDHealthFlag)
	{
		trap_R_SetColor( colorTable[CT_HUD_RED] );					
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealthTic );
	}

	trap_R_SetColor( colorTable[CT_HUD_RED] );	
	CG_DrawNumField (x + 16, y + 40, 3, ps->stats[STAT_HEALTH], 6, 12, 
		NUM_FONT_SMALL,qfalse);

}

/*
================
CG_DrawArmor
================
*/
void CG_DrawArmor(float x, float y)
{
	vec4_t			calcColor;
	float			armorPercent,hold;
	playerState_t	*ps;
	int				armor;

	ps = &cg.snap->ps;

	//	Outer Armor circular
	memcpy(calcColor, colorTable[CT_HUD_GREEN], sizeof(vec4_t));

	armor =ps->stats[STAT_ARMOR];
	
	if (armor> ps->stats[STAT_MAX_HEALTH])
	{
		armor = ps->stats[STAT_MAX_HEALTH];
	}

	hold = armor-(ps->stats[STAT_MAX_HEALTH]/2);
	armorPercent = (float) hold/(ps->stats[STAT_MAX_HEALTH]/2);
	if (armorPercent <0)
	{
		armorPercent = 0;
	}
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);					
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor1 );			

	// Inner Armor circular
	if (armorPercent>0)
	{
		armorPercent = 1;
	}
	else
	{
		armorPercent = (float) ps->stats[STAT_ARMOR]/(ps->stats[STAT_MAX_HEALTH]/2);
	}
	memcpy(calcColor, colorTable[CT_HUD_GREEN], sizeof(vec4_t));
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);					
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor2 );			//	Inner Armor circular

	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		// Make tic flash if inner armor is at 50% (25% of full armor)
		if (armorPercent<.5)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 100;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

	if (cg.HUDArmorFlag)
	{
		trap_R_SetColor( colorTable[CT_HUD_GREEN] );					
		CG_DrawPic( x, y, 80, 80, cgs.media.HUDArmorTic );		
	}

	trap_R_SetColor( colorTable[CT_HUD_GREEN] );	
	CG_DrawNumField (x + 18 + 14, y + 40 + 14, 3, ps->stats[STAT_ARMOR], 6, 12, 
		NUM_FONT_SMALL, qfalse);

}

/*
================
CG_DrawHUDRightFrame1
================
*/
void CG_DrawHUDRightFrame1(float x, float y)
{
	trap_R_SetColor( hudTintColor );
	// Inner gray wire frame
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDInnerRight );		// 
}

/*
================
CG_DrawHUDRightFrame2
================
*/
void CG_DrawHUDRightFrame2(float x, float y)
{
	trap_R_SetColor( hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDRightFrame );		// Metal frame
}

/*
================
CG_DrawAmmo
================
*/
static void CG_DrawAmmo(centity_t *cent, float x, float y)
{
	playerState_t	*ps;
	int			numColor_i;
	int			i;
	vec4_t		calcColor;
	float		value,inc,percent;

	ps = &cg.snap->ps;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon == WP_SABER )
	{
		trap_R_SetColor( colorTable[CT_WHITE] );
		// don't need to draw ammo, but we will draw the current saber style in this window
		switch ( cg.predictedPlayerState.fd.saberDrawAnimLevel )
		{
		case 1://FORCE_LEVEL_1:
			CG_DrawPic( x, y, 80, 40, cgs.media.HUDSaberStyle1 );
			break;
		case 2://FORCE_LEVEL_2:
			CG_DrawPic( x, y, 80, 40, cgs.media.HUDSaberStyle2 );
			break;
		case 3://FORCE_LEVEL_3:
			CG_DrawPic( x, y, 80, 40, cgs.media.HUDSaberStyle3 );
			break;
		}
		return;
	}
	else
	{
		value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
	}

	if (value < 0)	// No ammo
	{
		return;
	}


	//
	// ammo
	//
/*	if (cg.oldammo < value)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = value;
*/
	// Firing or reloading?
/*	if (( pm->ps->weaponstate == WEAPON_FIRING
		&& cg.predictedPlayerState.weaponTime > 100 ))
	{
		numColor_i = CT_LTGREY;
	} */
	// Overcharged?
//	else if ( cent->gent->s.powerups & ( 1 << PW_WEAPON_OVERCHARGE ) )
//	{
//		numColor_i = CT_WHITE;
//	}
//	else 
//	{
//		if ( value > 0 ) 
//		{
//			if (cg.oldAmmoTime > cg.time)
//			{
//				numColor_i = CT_YELLOW;
//			}
//			else
//			{
//				numColor_i = CT_HUD_ORANGE;
//			}
//		} 
//		else 
//		{
//			numColor_i = CT_RED;
//		}
//	}

	numColor_i = CT_HUD_ORANGE;

	trap_R_SetColor( colorTable[numColor_i] );	
	CG_DrawNumField (x + 30, y + 26, 3, value, 6, 12, NUM_FONT_SMALL,qfalse);


//cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]

	inc = (float) ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_TICS;
	value =ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	for (i=MAX_TICS-1;i>=0;i--)
	{

		if (value <= 0)	// partial tic
		{
			memcpy(calcColor, colorTable[CT_BLACK], sizeof(vec4_t));
		}
		else if (value < inc)	// partial tic
		{
			memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
			percent = value / inc;
			calcColor[0] *= percent;
			calcColor[1] *= percent;
			calcColor[2] *= percent;
		}
		else
		{
			memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
		}

		trap_R_SetColor( calcColor);
		CG_DrawPic( x + ammoTicPos[i].x, 
			y + ammoTicPos[i].y, 
			ammoTicPos[i].width, 
			ammoTicPos[i].height, 
			ammoTicPos[i].tic );

		value -= inc;
	}

}

/*
================
CG_DrawForcePower
================
*/
void CG_DrawForcePower(float x, float y)
{
	int			i;
	vec4_t		calcColor;
	float		value,inc,percent;

	inc = (float)  100 / MAX_TICS;
	value = cg.snap->ps.fd.forcePower;

	for (i=MAX_TICS-1;i>=0;i--)
	{

		if (value <= 0)	// partial tic
		{
			memcpy(calcColor, colorTable[CT_BLACK], sizeof(vec4_t));
		}
		else if (value < inc)	// partial tic
		{
			memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
			percent = value / inc;
			calcColor[0] *= percent;
			calcColor[1] *= percent;
			calcColor[2] *= percent;
		}
		else
		{
			memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
		}

		trap_R_SetColor( calcColor);
		CG_DrawPic( x + forceTicPos[i].x, 
			y + forceTicPos[i].y, 
			forceTicPos[i].width, 
			forceTicPos[i].height, 
			forceTicPos[i].tic );

		value -= inc;
	}
}

/*
================
CG_DrawHUD
================
*/
void CG_DrawHUD(centity_t	*cent)
{
	menuDef_t	*menuHUD = NULL;
	const char *scoreStr = NULL;
	int	scoreBias;
	char scoreBiasStr[16];

	//japro speedometer & strafehelper
	if (cg_strafeHelper.integer || cg_speedometer.integer & SPEEDOMETER_ENABLE) {
		CG_CalculateSpeed(cent);
	}

	//japro movement keys
	if (cg_movementKeys.integer)
		CG_DrawMovementKeys(cent);

	if ((cg_speedometer.integer & SPEEDOMETER_ENABLE)) {
		speedometerXPos = cg_speedometerX.integer;
		jumpsXPos = cg_speedometerJumpsX.integer;
		CG_DrawSpeedometer();

		if ((cg_speedometer.integer & SPEEDOMETER_ACCELMETER))
			CG_DrawAccelMeter();
		if (cg_speedometer.integer & SPEEDOMETER_JUMPHEIGHT)
			CG_DrawJumpHeight(cent);
		if (cg_speedometer.integer & SPEEDOMETER_JUMPDISTANCE)
			CG_DrawJumpDistance();
		if (cg_speedometer.integer & SPEEDOMETER_VERTICALSPEED)
			CG_DrawVerticalSpeed();

		if ((cg_speedometer.integer & SPEEDOMETER_SPEEDGRAPH)) {
			rectDef_t speedgraphRect;
			vec4_t foreColor = { 0.0f,0.8f,1.0f,0.8f };
			vec4_t backColor = { 0.0f,0.8f,1.0f,0.0f };
			speedgraphRect.x = (320.0f - (150.0f / 2.0f));
			speedgraphRect.y = cgs.screenHeight - 22 - 2;
			speedgraphRect.w = 150.0f;
			speedgraphRect.h = 22.0f;
			CG_GraphAddSpeed();
			CG_DrawSpeedGraph(&speedgraphRect, foreColor, backColor);
		}
	}

	//japro strafehelper
	if (cg_strafeHelper.integer) {
		CG_StrafeHelper(cent);
	}
	if (cg_strafeHelper.integer & SHELPER_CROSSHAIR) {
		vec4_t		hcolor;
		float		lineWidth;

		if (!cg.crosshairColor[0] && !cg.crosshairColor[1] && !cg.crosshairColor[2]) { //default to white
			hcolor[0] = 1.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 1.0f;
		}
		else {
			hcolor[0] = cg.crosshairColor[0];
			hcolor[1] = cg.crosshairColor[1];
			hcolor[2] = cg.crosshairColor[2];
			hcolor[3] = cg.crosshairColor[3];
		}

		lineWidth = cg_strafeHelperLineWidth.value;
		if (lineWidth < 0.25f)
			lineWidth = 0.25f;
		else if (lineWidth > 5)
			lineWidth = 5;

		CG_DrawLine((0.5f * cgs.screenWidth) - (0.5f * lineWidth), (0.5f * cgs.screenHeight) - 5.0f, (0.5f * cgs.screenWidth) - (0.5f * lineWidth), (0.5f * cgs.screenHeight) + 5.0f, lineWidth, hcolor, hcolor[3], 0); //640x480, 320x240
	}

	if (cg_hudFiles.integer)
	{
		int x = 0;
		int y = cgs.screenHeight-80;
		char ammoString[64];
		int weapX = x;

		UI_DrawProportionalString( x+16, y+40, va( "%i", cg.snap->ps.stats[STAT_HEALTH] ),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_RED] );

		UI_DrawProportionalString( x+18+14, y+40+14, va( "%i", cg.snap->ps.stats[STAT_ARMOR] ),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_GREEN] );

		if (cg.snap->ps.weapon == WP_SABER)
		{
			if (cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_3)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "STRONG");
				weapX += 16;
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_2)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "MEDIUM");
				weapX += 16;
			}
			else
			{
				Com_sprintf(ammoString, sizeof(ammoString), "FAST");
			}
		}
		else
		{
			Com_sprintf(ammoString, sizeof(ammoString), "%i", cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex]);
		}
		
		UI_DrawProportionalString( cgs.screenWidth-(weapX+16+32), y+40, va( "%s", ammoString ),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_ORANGE] );

		UI_DrawProportionalString( cgs.screenWidth-(x+18+14+32), y+40+14, va( "%i", cg.snap->ps.fd.forcePower),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_ICON_BLUE] );

		return;
	}

	if (cgs.gametype >= GT_TEAM)
	{	// tint the hud items based on team
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED )
			hudTintColor = redhudtint;
		else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
			hudTintColor = bluehudtint;
		else // If we're not on a team for whatever reason, leave things as they are.
			hudTintColor = colorTable[CT_WHITE];
	}
	else
	{	// tint the hud items white (dont' tint)
		hudTintColor = colorTable[CT_WHITE];
	}

	menuHUD = Menus_FindByName("lefthud");
	if (menuHUD)
	{
		CG_DrawHUDLeftFrame1(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawArmor(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawHealth(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawHUDLeftFrame2(menuHUD->window.rect.x,menuHUD->window.rect.y);
	}
	else
	{ //Apparently we failed to get proper coordinates from the menu, so resort to manually inputting them.
		CG_DrawHUDLeftFrame1(0,cgs.screenHeight-80);
		CG_DrawArmor(0,cgs.screenHeight-80);
		CG_DrawHealth(0,cgs.screenHeight-80);
		CG_DrawHUDLeftFrame2(0,cgs.screenHeight-80);
	}

	//scoreStr = va("Score: %i", cgs.clientinfo[cg.snap->ps.clientNum].score);
	if ( cgs.gametype == GT_TOURNAMENT )
	{//A duel that requires more than one kill to knock the current enemy back to the queue
		//show current kills out of how many needed
		scoreStr = va("Score: %i/%i", cg.snap->ps.persistant[PERS_SCORE], cgs.fraglimit);
	}
	else if (0 && cgs.gametype < GT_TEAM )
	{	// This is a teamless mode, draw the score bias.
		scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores1;
		if (scoreBias == 0)
		{	// We are the leader!
			if (cgs.scores2 <= 0)
			{	// Nobody to be ahead of yet.
				Q_strncpyz(scoreBiasStr, "", sizeof(scoreBiasStr));
			}
			else
			{
				scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores2;
				if (scoreBias == 0)
				{
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (Tie)");
				}
				else
				{
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (+%d)", scoreBias);
				}
			}
		}
		else // if (scoreBias < 0)
		{	// We are behind!
			Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (%d)", scoreBias);
		}
		scoreStr = va("Score: %i%s", cg.snap->ps.persistant[PERS_SCORE], scoreBiasStr);
	}
	else
	{	// Don't draw a bias.
		scoreStr = va("Score: %i", cg.snap->ps.persistant[PERS_SCORE]);
	}
	UI_DrawScaledProportionalString(cgs.screenWidth-101, cgs.screenHeight-23, scoreStr, UI_RIGHT|UI_DROPSHADOW, colorTable[CT_WHITE], 0.7);

	menuHUD = Menus_FindByName("righthud");
	if (menuHUD)
	{
		CG_DrawHUDRightFrame1(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawForcePower(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawAmmo(cent,menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawHUDRightFrame2(menuHUD->window.rect.x,menuHUD->window.rect.y);

	}
	else
	{ //Apparently we failed to get proper coordinates from the menu, so resort to manually inputting them.
		CG_DrawHUDRightFrame1(cgs.screenWidth-80,cgs.screenHeight-80);
		CG_DrawForcePower(cgs.screenWidth-80,cgs.screenHeight-80);
		CG_DrawAmmo(cent, cgs.screenWidth-80,cgs.screenHeight-80);
		CG_DrawHUDRightFrame2(cgs.screenWidth-80,cgs.screenHeight-80);
	}
}

#define MAX_SHOWPOWERS NUM_FORCE_POWERS

qboolean ForcePower_Valid(int i)
{
	if (i == FP_LEVITATION ||
		i == FP_SABERATTACK ||
		i == FP_SABERDEFEND ||
		i == FP_SABERTHROW)
	{
		return qfalse;
	}

	if (cg.snap->ps.fd.forcePowersKnown & (1 << i))
	{
		return qtrue;
	}
	
	return qfalse;
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect( void ) 
{
	int		i;
	int		count;
	float	smallIconSize,bigIconSize;
	float	holdX,x,y,pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		return;
	}

	if ((cg.forceSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		return;
	}

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0;i < NUM_FORCE_POWERS;++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

	smallIconSize = 30;
	bigIconSize = 60;
	pad = 12;

	// Max number of icons on the side
	if (cg_widescreen.integer)
		sideMax = (cgs.screenWidth - 240 - bigIconSize) / (smallIconSize + pad) / 2;
	else
		sideMax = 3;
 

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	x = 0.5 * cgs.screenWidth;
	y = cgs.screenHeight - 55;

	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS-1;
	}

	trap_R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS-1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); 
			holdX -= (smallIconSize+pad);
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{
		// Current Center Icon
		if (cgs.media.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)), bigIconSize, bigIconSize, cgs.media.forcePowerIcons[cg.forceSelect] ); //only cache the icon for display
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i>=MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); //only cache the icon for display
			holdX += (smallIconSize+pad);
		}
	}

	if ( showPowersName[cg.forceSelect] ) 
	{
		UI_DrawProportionalString(0.5f * cgs.screenWidth, y + 30, CG_GetStripEdString("INGAME", showPowersName[cg.forceSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
}

/*
===================
CG_DrawInventorySelect
===================
*/
void CG_DrawInvenSelect( void ) 
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	float			smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	float			holdX,x,y,y2,pad;
	// int				height;
	// float			addX;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		return;
	}

	if ((cg.invenSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	if (!cg.snap->ps.stats[STAT_HOLDABLE_ITEM] || !cg.snap->ps.stats[STAT_HOLDABLE_ITEMS])
	{
		return;
	}

	if (cg.itemSelect == -1)
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
	}

//const int bits = cg.snap->ps.stats[ STAT_ITEMS ];

	// count the number of items owned
	count = 0;
	for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ ) 
	{
		if (/*CG_InventorySelectable(i) && inv_icons[i]*/
			(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) ) 
		{
			count++;
		}
	}

	if (!count)
	{
		y2 = 0; //err?
		UI_DrawProportionalString(0.5f * cgs.screenWidth, y2 + 22, "EMPTY INVENTORY", UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 16;

	// Max number of icons on the side
	if (cg_widescreen.integer)
		sideMax = (cgs.screenWidth - 240 - bigIconSize) / (smallIconSize + pad) / 2;
	else
		sideMax = 3;

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.itemSelect - 1;
	if (i<0)
	{
		i = HI_NUM_HOLDABLE-1;
	}

	x = 0.5 * cgs.screenWidth;
	y = cgs.screenHeight - 70;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	// height = smallIconSize * cg.iconHUDPercent;
	// addX = (float) smallIconSize * .75;

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i<0)
		{
			i = HI_NUM_HOLDABLE-1;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.invenIcons[i])
		{
			trap_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, cgs.media.invenIcons[i] );

			trap_R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12, 
				NUM_FONT_SMALL,qfalse);
				*/

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	// height = bigIconSize * cg.iconHUDPercent;
	if (cgs.media.invenIcons[cg.itemSelect])
	{
		int itemNdex;
		trap_R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, cgs.media.invenIcons[cg.itemSelect] );
		// addX = (float) bigIconSize * .75;
		trap_R_SetColor(colorTable[CT_ICON_BLUE]);
		/*CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12, 
			NUM_FONT_SMALL,qfalse);*/

		itemNdex = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
		if (bg_itemlist[itemNdex].classname)
		{
			vec4_t	textColor = { .312f, .75f, .621f, 1.0f };
			char	text[1024];
			
			if ( trap_SP_GetStringTextString( va("INGAME_%s",bg_itemlist[itemNdex].classname), text, sizeof( text )))
			{
				UI_DrawProportionalString(0.5f * cgs.screenWidth, y+45, text, UI_CENTER | UI_SMALLFONT, textColor);
			}
			else
			{
				UI_DrawProportionalString(0.5f * cgs.screenWidth, y+45, bg_itemlist[itemNdex].classname, UI_CENTER | UI_SMALLFONT, textColor);
			}
		}
	}

	i = cg.itemSelect + 1;
	if (i> HI_NUM_HOLDABLE-1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	// height = smallIconSize * cg.iconHUDPercent;
	// addX = (float) smallIconSize * .75;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i> HI_NUM_HOLDABLE-1)
		{
			i = 0;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.invenIcons[i])
		{
			trap_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, cgs.media.invenIcons[i] );

			trap_R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12, 
				NUM_FONT_SMALL,qfalse);*/

			holdX += (smallIconSize+pad);
		}
	}
}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats( void ) 
{
	centity_t		*cent;
/*	playerState_t	*ps;
	vec3_t			angles;
//	vec3_t		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
*/
	cent = &cg_entities[cg.snap->ps.clientNum];
/*	ps = &cg.snap->ps;

	VectorClear( angles );

	// Do start
	if (!cg.interfaceStartupDone)
	{
		CG_InterfaceStartup();
	}

	cgi_UI_MenuPaintAll();*/

	CG_DrawHUD(cent);
	/*CG_DrawArmor(cent);
	CG_DrawHealth(cent);
	CG_DrawAmmo(cent);

	CG_DrawTalk(cent);*/
}


/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = .2f;
		hcolor[2] = .2f;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	} else {
		return;
	}
//	trap_R_SetColor( hcolor );

	CG_FillRect ( x, y, w, h, hcolor );
//	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawMiniScoreboard
================
*/
static float CG_DrawMiniScoreboard ( float y ) 
{
	char temp[MAX_QPATH];

	if ( !cg_drawScores.integer )
	{
		return y;
	}

	if ( cgs.gametype >= GT_TEAM )
	{
		strcpy ( temp, "Red: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );
		Q_strcat ( temp, MAX_QPATH, " Blue: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( temp, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
	}
	else
	{
		/*
		strcpy ( temp, "1st: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );
		
		Q_strcat ( temp, MAX_QPATH, " 2nd: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );
		
		CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( temp, 0.7f, FONT_SMALL ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
		*/
		//rww - no longer doing this. Since the attacker now shows who is first, we print the score there.
	}		
	

	return y;
}

/*
================
CG_DrawEnemyInfo
================
*/
static float CG_DrawEnemyInfo ( float y ) 
{
	float		size;
	int			clientNum;
	const char	*title;
	clientInfo_t	*ci;

	if ( !cg_drawEnemyInfo.integer ) 
	{
		return y;
	}

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) 
	{
		return y;
	}
	
	if ( cgs.gametype == GT_JEDIMASTER )
	{
		//title = "Jedi Master";
		title = CG_GetStripEdString("INGAMETEXT", "MASTERY7");
		clientNum = cgs.jediMaster;

		if ( clientNum < 0 )
		{
			//return y;
//			title = "Get Saber!";
			title = CG_GetStripEdString("INGAMETEXT", "GET_SABER");


			size = ICON_SIZE * 1.25;
			y += 5;

			CG_DrawPic( cgs.screenWidth - size - 12, y, size, size, cgs.media.weaponIcons[WP_SABER] );

			y += size;

			/*
			CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
			y += 15;
			*/

			CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );

			return y + BIGCHAR_HEIGHT + 2;
		}
	}
	else if ( cg.snap->ps.duelInProgress )
	{
//		title = "Dueling";
		title = CG_GetStripEdString("INGAMETEXT", "DUELING");
		clientNum = cg.snap->ps.duelIndex;
	}
	else if ( cgs.gametype == GT_TOURNAMENT && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
//		title = "Dueling";
		title = CG_GetStripEdString("INGAMETEXT", "DUELING");
		if (cg.snap->ps.clientNum == cgs.duelist1)
		{
			clientNum = cgs.duelist2;
		}
		else if (cg.snap->ps.clientNum == cgs.duelist2)
		{
			clientNum = cgs.duelist1;
		}
		else
		{
			return y;
		}
	}
	else
	{
		/*
		title = "Attacker";
		clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];

		if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) 
		{
			return y;
		}

		if ( cg.time - cg.attackerTime > ATTACKER_HEAD_TIME ) 
		{
			cg.attackerTime = 0;
			return y;
		}
		*/
		//As of current, we don't want to draw the attacker. Instead, draw whoever is in first place.
		if (cgs.duelWinner < 0 || cgs.duelWinner >= MAX_CLIENTS)
		{
			return y;
		}


		title = va("%s: %i",CG_GetStripEdString("INGAMETEXT", "LEADER"), cgs.scores1);

		/*
		if (cgs.scores1 == 1)
		{
			title = va("%i kill", cgs.scores1);
		}
		else
		{
			title = va("%i kills", cgs.scores1);
		}
		*/
		clientNum = cgs.duelWinner;
	}

	ci = &cgs.clientinfo[ clientNum ];

	if ( !ci )
	{
		return y;
	}

	size = ICON_SIZE * 1.25;
	y += 5;

	if ( ci->modelIcon )
	{
		CG_DrawPic( cgs.screenWidth - size - 5, y, size, size, ci->modelIcon );
	}

	y += size;

	CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );

	y += 15;
	CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );

	if ( cgs.gametype == GT_TOURNAMENT && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{//also print their score
		char text[1024];
		y += 15;
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[clientNum].score, cgs.fraglimit );
		CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( text, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, text, 0, 0, 0, FONT_MEDIUM );
	}

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString(cgs.screenWidth - 5 - w, y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString(cgs.screenWidth - 5 - w, y + 2, s, 1.0f);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString(cgs.screenWidth - 5 - w, y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	float x, w, xx;
	int h;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == (team_t)cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = cgs.screenWidth - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == (team_t)cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j < PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


static void CG_DrawPowerupIcons(int y)
{
	int j;
	int ico_size = 64;
	float xAlign = cgs.screenWidth - ico_size * 1.1f;
	//int y = ico_size/2;
	gitem_t	*item;

	if (!cg.snap)
	{
		return;
	}

	y += 16;

	for (j = 0; j < PW_NUM_POWERUPS; j++)
	{
		if (cg.snap->ps.powerups[j] > cg.time)
		{
			int secondsleft = (cg.snap->ps.powerups[j] - cg.time)/1000;

			item = BG_FindItemForPowerup( j );

			if (item)
			{
				int icoShader = 0;
				if (cgs.gametype == GT_CTY && (j == PW_REDFLAG || j == PW_BLUEFLAG))
				{
					if (j == PW_REDFLAG)
					{
						icoShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
					}
					else
					{
						icoShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
					}
				}
				else
				{
					icoShader = trap_R_RegisterShader( item->icon );
				}

				CG_DrawPic( xAlign, y, ico_size, ico_size, icoShader );
	
				y += ico_size;

				if (j != PW_REDFLAG && j != PW_BLUEFLAG && secondsleft < 999)
				{
					UI_DrawProportionalString(xAlign + (ico_size / 2), y - 8, va("%i", secondsleft), UI_CENTER | UI_BIGFONT | UI_DROPSHADOW, colorTable[CT_WHITE]);
				}

				y += (ico_size/3);
			}
		}
	}
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 0;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	
	y = CG_DrawEnemyInfo ( y );

	y = CG_DrawMiniScoreboard ( y );

	CG_DrawPowerupIcons(y);
}

/*
===================
CG_DrawReward
===================
*/
#ifdef JK2AWARDS
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 0.5f * (cgs.screenWidth - ICON_SIZE);
		CG_DrawPic( x + 2, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( cgs.screenWidth - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) * 0.5f;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 0.5f * (cgs.screenWidth - count * ICON_SIZE);
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic(x + 2, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}
#endif


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	if (cg.mMapChange)
	{			
		s = CG_GetStripEdString("INGAMETEXT", "SERVER_CHANGING_MAPS");	// s = "Server Changing Maps";			
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(0.5f * (cgs.screenWidth - w), 100, s, 1.0f);

		s = CG_GetStripEdString("INGAMETEXT", "PLEASE_WAIT");	// s = "Please wait...";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(0.5f * (cgs.screenWidth - w), 200, s, 1.0f);
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_GetStripEdString("INGAMETEXT", "CONNECTION_INTERRUPTED"); // s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(0.5f * (cgs.screenWidth - w), 100, s, 1.0f);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = cgs.screenWidth - 48;
	y = cgs.screenHeight - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = cgs.screenWidth - 48;
	y = cgs.screenHeight - 144;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );
	x -= 1.0f; //lines the actual graph up with the background

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		y, w, h;
	float	x;
	float	*color;
	const float scale = 1.0; //0.5

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width(linebuffer, scale, FONT_MEDIUM);
		h = CG_Text_Height(linebuffer, scale, FONT_MEDIUM);
		x = 0.5f * (cgs.screenWidth - w);
		CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		y += h + 6;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( vec3_t worldPoint, int chEntValid ) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;

	if (cg_strafeHelper.integer & SHELPER_CROSSHAIR) //japro - don't draw a crosshair when we are using the strafehelper line crosshair
	{
		return;
	}

	if ( !cg_drawCrosshair.integer ) 
	{
		return;
	}

	if (cg.snap->ps.fallingToDeath)
	{
		return;
	}

	if ( cg.predictedPlayerState.zoomMode != 0 )
	{//not while scoped
		return;
	}

	if ( cg_crosshairHealth.integer )
	{
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	}
	else
	{
		//set color based on what kind of ent is under crosshair
		if ( cg.crosshairClientNum >= ENTITYNUM_WORLD )
		{
			trap_R_SetColor( NULL );
		}
		else if (chEntValid && (cg_entities[cg.crosshairClientNum].currentState.number < MAX_CLIENTS || cg_entities[cg.crosshairClientNum].currentState.shouldtarget))
		{
			vec4_t	ecolor = {0,0,0,0};
			centity_t *crossEnt = &cg_entities[cg.crosshairClientNum];

			if ( crossEnt->currentState.number < MAX_CLIENTS )
			{
				if (cgs.gametype >= GT_TEAM &&
					cgs.clientinfo[crossEnt->currentState.number].team == cgs.clientinfo[cg.snap->ps.clientNum].team )
				{
					//Allies are green
					ecolor[0] = 0.0;//R
					ecolor[1] = 1.0;//G
					ecolor[2] = 0.0;//B
				}
				else
				{
					//Enemies are red
					ecolor[0] = 1.0;//R
					ecolor[1] = 0.0;//G
					ecolor[2] = 0.0;//B
				}

				if (cg.snap->ps.duelInProgress)
				{
					if (crossEnt->currentState.number != cg.snap->ps.duelIndex)
					{ //grey out crosshair for everyone but your foe if you're in a duel
						ecolor[0] = 0.4;
						ecolor[1] = 0.4;
						ecolor[2] = 0.4;
					}
				}
				else if (crossEnt->currentState.bolt1)
				{ //this fellow is in a duel. We just checked if we were in a duel above, so
				  //this means we aren't and he is. Which of course means our crosshair greys out over him.
					ecolor[0] = 0.4;
					ecolor[1] = 0.4;
					ecolor[2] = 0.4;
				}
			}
			else if (crossEnt->currentState.shouldtarget)
			{
				//VectorCopy( crossEnt->startRGBA, ecolor );
				if ( !ecolor[0] && !ecolor[1] && !ecolor[2] )
				{
					// We really don't want black, so set it to yellow
					ecolor[0] = 1.0F;//R
					ecolor[1] = 0.8F;//G
					ecolor[2] = 0.3F;//B
				}

				if (crossEnt->currentState.owner == cg.snap->ps.clientNum ||
					(cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner == (int)cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor[0] = 0.0;//R
					ecolor[1] = 1.0;//G
					ecolor[2] = 0.0;//B
				}
				else if (crossEnt->currentState.teamowner == 16 ||
					(cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner && crossEnt->currentState.teamowner != (int)cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor[0] = 1.0;//R
					ecolor[1] = 0.0;//G
					ecolor[2] = 0.0;//B
				}
				else if (crossEnt->currentState.eType == ET_GRAPPLE)
				{
					ecolor[0] = 1.0;//R
					ecolor[1] = 0.0;//G
					ecolor[2] = 0.0;//B
				}
			}

			ecolor[3] = 1.0;

			trap_R_SetColor( ecolor );
		}
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	if ( worldPoint && VectorLength( worldPoint ) )
	{
		if ( !CG_WorldCoordToScreenCoord( worldPoint, &x, &y ) )
		{//off screen, don't draw it
			return;
		}
	}
	else
	{
		x = 0.5f * cgs.screenWidth + cg_crosshairX.value;
		y = 0.5f * cgs.screenHeight + cg_crosshairY.value;
	}

	hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	
	CG_DrawPic(x + cg.refdef.x - 0.5f * w, y + cg.refdef.y - 0.5f * w, w, h, hShader);

	trap_R_SetColor( NULL );

}

qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, float *x, float *y)
{
	float	xcenter, ycenter;
	vec3_t	local, transformed;
	vec3_t	vfwd;
	vec3_t	vright;
	vec3_t	vup;
	float xzi;
	float yzi;

//	xcenter = cg.refdef.width / 2;//gives screen coords adjusted for resolution
//	ycenter = cg.refdef.height / 2;//gives screen coords adjusted for resolution
	
	//NOTE: did it this way because most draw functions expect virtual 640x480 coords
	//	and adjust them for current resolution
	xcenter = 0.5f * cgs.screenWidth;
	ycenter = 0.5f * cgs.screenHeight;

	AngleVectors (cg.refdefViewAngles, vfwd, vright, vup);

	VectorSubtract (worldCoord, cg.refdef.vieworg, local);

	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vfwd);		

	// Make sure Z is not negative.
	if(transformed[2] < 0.01)
	{
		return qfalse;
	}

	xzi = xcenter / transformed[2] * (90.0/cg.refdef.fov_x);
	yzi = ycenter / transformed[2] * (90.0/cg.refdef.fov_y);

	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];

	return qtrue;
}

/*
====================
CG_SaberClashFlare
====================
*/
int g_saberFlashTime = 0;
vec3_t g_saberFlashPos = {0, 0, 0};
void CG_SaberClashFlare( void ) 
{
	int				t, maxTime = 150;
	vec3_t dif;
	float x,y;
	float v, len;
	trace_t tr;

	t = cg.time - g_saberFlashTime;

	if ( t <= 0 || t >= maxTime ) 
	{
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract( g_saberFlashPos, cg.refdef.vieworg, dif );

	if ( DotProduct( dif, cg.refdef.viewaxis[0] ) < 0.2 )
	{
		return;
	}

	CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, g_saberFlashPos, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f )
	{
		return;
	}

	len = VectorNormalize( dif );

	// clamp to a known range
	if ( len > 800 )
	{
		len = 800;
	}

	v = ( 1.0f - ((float)t / maxTime )) * ((1.0f - ( len / 800.0f )) * 2.0f + 0.35f);

	if ( CG_WorldCoordToScreenCoord( g_saberFlashPos, &x, &y ) )
	{
		vec4_t color = { 0.8f, 0.8f, 0.8f, 1.0f };

		trap_R_SetColor( color );

		CG_DrawPic( x - ( v * 300 ), y - ( v * 300 ),
			v * 600, v * 600,
			trap_R_RegisterShader( "gfx/effects/saberFlare" ));
	}
}

//--------------------------------------------------------------
static void CG_DrawHolocronIcons(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = 10;
	int starty = 10;//SCREEN_HEIGHT - icon_size*3;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.zoomMode)
	{ //don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if (cg.snap->ps.holocronBits & (1 << forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			starty += (icon_size+2); //+2 for spacing
			if ((starty+icon_size) >= cgs.screenHeight-80)
			{
				starty = 10;//SCREEN_HEIGHT - icon_size*3;
				startx += (icon_size+2);
			}
		}

		i++;
	}
}

static qboolean CG_IsDurationPower(int power)
{
	if (power == FP_HEAL ||
		power == FP_SPEED ||
		power == FP_TELEPATHY ||
		power == FP_RAGE ||
		power == FP_PROTECT ||
		power == FP_ABSORB ||
		power == FP_SEE)
	{
		return qtrue;
	}

	return qfalse;
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = icon_size*2+16;
	int starty = cgs.screenHeight - icon_size*2;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.zoomMode)
	{ //don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if ((cg.snap->ps.fd.forcePowersActive & (1 << forcePowerSorted[i])) &&
			CG_IsDurationPower(forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			startx += (icon_size+2); //+2 for spacing
			if ((startx+icon_size) >= cgs.screenWidth-80)
			{
				startx = icon_size*2+16;
				starty += (icon_size+2);
			}
		}

		i++;
	}

	//additionally, draw an icon force force rage recovery
	if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawPic( startx, starty, endx, endy, cgs.media.rageRecShader);
	}
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking( int lockEntNum, int lockTime )
//--------------------------------------------------------------
{
	float	cx, cy;
	vec3_t	org;
	static	int oldDif = 0;
	centity_t *cent = &cg_entities[lockEntNum];
	vec4_t color={0.0f,0.0f,0.0f,0.0f};
	int dif = ( cg.time - cg.snap->ps.rocketLockTime ) / ( 1200.0f / /*8.0f*/16.0f );
	int i;

	if (!cg.snap->ps.rocketLockTime)
	{
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	//We can't check to see in pmove if players are on the same team, so we resort
	//to just not drawing the lock if a teammate is the locked on ent
	if (cg.snap->ps.rocketLockIndex >= 0 &&
		cg.snap->ps.rocketLockIndex < MAX_CLIENTS)
	{
		if (cgs.clientinfo[cg.snap->ps.rocketLockIndex].team == cgs.clientinfo[cg.snap->ps.clientNum].team)
		{
			if (cgs.gametype >= GT_TEAM)
			{
				return;
			}
		}
	}

	if (cg.snap->ps.rocketLockTime != -1)
	{
		lastvalidlockdif = dif;
	}
	else
	{
		dif = lastvalidlockdif;
	}

	if ( !cent )
	{
		return;
	}

	VectorCopy( cent->lerpOrigin, org );

	if ( CG_WorldCoordToScreenCoord( org, &cx, &cy ))
	{
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance( cent->lerpOrigin, cg.refdef.vieworg ) / 1024.0f; 
		
		if ( sz > 1.0f )
		{
			sz = 1.0f;
		}
		else if ( sz < 0.0f )
		{
			sz = 0.0f;
		}

		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;

		cy += sz * 0.5f;
		
		if ( dif < 0 )
		{
			oldDif = 0;
			return;
		}
		else if ( dif > 8 )
		{
			dif = 8;
		}

		// do sounds
		if ( oldDif != dif )
		{
			if ( dif == 8 )
			{
				trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" ));
			}
			else
			{
				trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" ));
			}
		}

		oldDif = dif;

		for ( i = 0; i < dif; i++ )
		{
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.1f * i + 0.2f;

			trap_R_SetColor( color );

			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic( cx - sz, cy - sz, sz, sz, i * 45.0f, trap_R_RegisterShaderNoMip( "gfx/2d/wedge" ));
		}

		// we are locked and loaded baby
		if ( dif == 8 )
		{
			color[0] = color[1] = color[2] = sin( cg.time * 0.05f ) * 0.5f + 0.5f;
			color[3] = 1.0f; // this art is additive, so the alpha value does nothing

			trap_R_SetColor( color );

			CG_DrawPic( cx - sz, cy - sz * 2, sz * 2, sz * 2, trap_R_RegisterShaderNoMip( "gfx/2d/lock" ));
		}
	}
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	if ( cg_dynamicCrosshair.integer )
	{
		vec3_t d_f, d_rt, d_up;
		/*
		if ( cg.snap->ps.weapon == WP_NONE || 
			cg.snap->ps.weapon == WP_SABER || 
			cg.snap->ps.weapon == WP_STUN_BATON)
		{
			VectorCopy( cg.refdef.vieworg, start );
			AngleVectors( cg.refdefViewAngles, d_f, d_rt, d_up );
		}
		else
		*/
		//For now we still want to draw the crosshair in relation to the player's world coordinates
		//even if we have a melee weapon/no weapon.
		{
			if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
			{
				vec3_t pitchConstraint;

				VectorCopy(cg.refdefViewAngles, pitchConstraint);

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdefViewAngles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			else
			{
				vec3_t pitchConstraint;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdefViewAngles, pitchConstraint);
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			CG_CalcMuzzlePoint(cg.snap->ps.clientNum, start);
		}

		//FIXME: increase this?  Increase when zoom in?
		VectorMA( start, 4096, d_f, end );//was 8192
	}
	else
	{
		VectorCopy( cg.refdef.vieworg, start );
		VectorMA( start, 131072, cg.refdef.viewaxis[0], end );
	}

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );

	if (trace.entityNum < MAX_CLIENTS)
	{
		if (CG_IsMindTricked(cg_entities[trace.entityNum].currentState.trickedentindex,
			cg_entities[trace.entityNum].currentState.trickedentindex2,
			cg_entities[trace.entityNum].currentState.trickedentindex3,
			cg_entities[trace.entityNum].currentState.trickedentindex4,
			cg.snap->ps.clientNum))
		{
			if (cg.crosshairClientNum == trace.entityNum)
			{
				cg.crosshairClientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
			}

			CG_DrawCrosshair(trace.endpos, 0);

			return; //this entity is mind-tricking the current client, so don't render it
		}
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		if (trace.entityNum < /*MAX_CLIENTS*/ENTITYNUM_WORLD)
		{
			CG_DrawCrosshair(trace.endpos, 1);
		}
		else
		{
			CG_DrawCrosshair(trace.endpos, 0);
		}
	}

//	if ( trace.entityNum >= MAX_CLIENTS ) {
//		return;
//	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	if ( trace.entityNum >= MAX_CLIENTS ) {
		cg.crosshairClientNum = trace.entityNum;
		cg.crosshairClientTime = cg.time;
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	vec4_t		tcolor;
	char		*name;
	int			baseColor;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	//rww - still do the trace, our dynamic crosshair depends on it

	if (cg.crosshairClientNum >= MAX_CLIENTS)
	{
		return;
	}

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;

	if (cgs.gametype >= GT_TEAM)
	{
		if (cgs.clientinfo[cg.crosshairClientNum].team == TEAM_RED)
		{
			baseColor = CT_RED;
		}
		else
		{
			baseColor = CT_BLUE;
		}

		/*
		//For now instead of team-based we'll make it oriented based on which team we're on
		if (cgs.clientinfo[cg.crosshairClientNum].team == cgs.clientinfo[cg.snap->ps.clientNum].team)
		{
			baseColor = CT_GREEN;
		}
		else
		{
			baseColor = CT_RED;
		}
		*/
	}
	else
	{
		//baseColor = CT_WHITE;
		baseColor = CT_RED; //just make it red in nonteam modes since everyone is hostile and crosshair will be red on them too
	}

	if (cg.snap->ps.duelInProgress)
	{
		if (cg.crosshairClientNum != cg.snap->ps.duelIndex)
		{ //grey out crosshair for everyone but your foe if you're in a duel
			baseColor = CT_BLACK;
		}
	}
	else if (cg_entities[cg.crosshairClientNum].currentState.bolt1)
	{ //this fellow is in a duel. We just checked if we were in a duel above, so
	  //this means we aren't and he is. Which of course means our crosshair greys out over him.
		baseColor = CT_BLACK;
	}

	tcolor[0] = colorTable[baseColor][0];
	tcolor[1] = colorTable[baseColor][1];
	tcolor[2] = colorTable[baseColor][2];
	tcolor[3] = color[3]*0.5f;

	UI_DrawProportionalString(0.5f * cgs.screenWidth, 170, name, UI_CENTER, tcolor);

	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) 
{	
	const char* s;
	s = CG_GetStripEdString("INGAMETEXT", "SPECTATOR");
	if (cgs.gametype == GT_TOURNAMENT &&
		cgs.duelist1 != -1 &&
		cgs.duelist2 != -1)
	{
		char text[1024];
		int size = 64;

		Com_sprintf(text, sizeof(text), "%s" S_COLOR_WHITE " %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStripEdString("INGAMETEXT", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name);
		CG_Text_Paint ( 0.5f * cgs.screenWidth - CG_Text_Width ( text, 1.0f, 3 ) / 2, cgs.screenHeight-60, 1.0f, colorWhite, text, 0, 0, 0, 3 );


		trap_R_SetColor( colorTable[CT_WHITE] );
		if ( cgs.clientinfo[cgs.duelist1].modelIcon )
		{
			CG_DrawPic( 10, cgs.screenHeight-(size*1.5), size, size, cgs.clientinfo[cgs.duelist1].modelIcon );
		}
		if ( cgs.clientinfo[cgs.duelist2].modelIcon )
		{
			CG_DrawPic( cgs.screenWidth-size-10, cgs.screenHeight-(size*1.5), size, size, cgs.clientinfo[cgs.duelist2].modelIcon );
		}
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist1].score, cgs.fraglimit );
		CG_Text_Paint( 42 - CG_Text_Width( text, 1.0f, 2 ) / 2, cgs.screenHeight-(size*1.5) + 64, 1.0f, colorWhite, text, 0, 0, 0, 2 );

		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist2].score, cgs.fraglimit );
		CG_Text_Paint( cgs.screenWidth-size+22 - CG_Text_Width( text, 1.0f, 2 ) / 2, cgs.screenHeight-(size*1.5) + 64, 1.0f, colorWhite, text, 0, 0, 0, 2 );
	}
	else
	{
		CG_Text_Paint ( 0.5f * cgs.screenWidth - CG_Text_Width ( s, 1.0f, 3 ) / 2, cgs.screenHeight-60, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}

	if ( cgs.gametype == GT_TOURNAMENT ) 
	{
		s = CG_GetStripEdString("INGAMETEXT", "WAITING_TO_PLAY");	// "waiting to play";
		CG_Text_Paint ( 0.5f * cgs.screenWidth - CG_Text_Width ( s, 1.0f, 3 ) / 2, cgs.screenHeight-40, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}
	else //if ( cgs.gametype >= GT_TEAM ) 
	{
		//s = "press ESC and use the JOIN menu to play";
		s = CG_GetStripEdString("INGAMETEXT", "SPEC_CHOOSEJOIN");
	}
		CG_Text_Paint ( 0.5f * cgs.screenWidth - CG_Text_Width ( s, 1.0f, 3 ) / 2, cgs.screenHeight-40, 1.0f, colorWhite, s, 0, 0, 0, 3 );
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	const char	*s;
	int		sec;
	char sYes[20];
	char sNo[20];

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
//		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	trap_SP_GetStringTextString("MENUS0_YES", sYes, sizeof(sYes) );
	trap_SP_GetStringTextString("MENUS0_NO",  sNo,  sizeof(sNo) );

	s = va("VOTE(%i):%s" S_COLOR_WHITE " %s:%i %s:%i", sec, cgs.voteString, sYes, cgs.voteYes, sNo, cgs.voteNo);
	CG_DrawSmallString( 4, 58, s, 1.0F );
	s = CG_GetStripEdString("INGAMETEXT", "OR_PRESS_ESC_THEN_CLICK_VOTE");	//	s = "or press ESC then click Vote";
	CG_DrawSmallString( 4, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;
	team_t	team = cgs.clientinfo[cg.snap->ps.clientNum].team;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
//		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	if (strstr(cgs.teamVoteString[cs_offset], "leader"))
	{
		int i = 0;

		while (cgs.teamVoteString[cs_offset][i] && cgs.teamVoteString[cs_offset][i] != ' ')
		{
			i++;
		}

		if (cgs.teamVoteString[cs_offset][i] == ' ')
		{
			int voteIndex = 0;
			char voteIndexStr[256];

			i++;

			while (cgs.teamVoteString[cs_offset][i])
			{
				voteIndexStr[voteIndex] = cgs.teamVoteString[cs_offset][i];
				voteIndex++;
				i++;
			}
			voteIndexStr[voteIndex] = 0;

			voteIndex = atoi(voteIndexStr);

			s = va("TEAMVOTE(%i):(Make %s" S_COLOR_WHITE " the new team leader) yes:%i no:%i", sec, cgs.clientinfo[voteIndex].name,
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
		else
		{
			s = va("TEAMVOTE(%i):%s" S_COLOR_WHITE " yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
	}
	else
	{
		s = va("TEAMVOTE(%i):%s" S_COLOR_WHITE " yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
								cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	}
	CG_DrawSmallString( 4, 90, s, 1.0F );
}

static qboolean CG_DrawScoreboard() {
	return CG_DrawOldScoreboard();
#if 0
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}																					  


	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#endif
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) 
{
	const char	*s;
	float		x;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) 
	{
		return qfalse;
	}

//	s = "following";
	s = CG_GetStripEdString("INGAMETEXT", "FOLLOWING");
	x = 0.5f * (cgs.screenWidth - CG_Text_Width(s, 1.0f, FONT_MEDIUM));
	CG_Text_Paint(x , 60, 1.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM);

	s = cgs.clientinfo[ cg.snap->ps.clientNum ].name;
	x = 0.5f * (cgs.screenWidth - CG_Text_Width(s, 2.0f, FONT_MEDIUM));
	CG_Text_Paint(x, 80, 2.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM);

	return qtrue;
}

#if 0
static void CG_DrawTemporaryStats()
{ //placeholder for testing (draws ammo and force power)
	char s[512];

	if (!cg.snap)
	{
		return;
	}

	sprintf(s, "Force: %i", cg.snap->ps.fd.forcePower);

	CG_DrawBigString(cgs.screenWidth-164, cgs.screenHeight-128, s, 1.0f);

	sprintf(s, "Ammo: %i", cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]);

	CG_DrawBigString(cgs.screenWidth-164, cgs.screenHeight-112, s, 1.0f);

	sprintf(s, "Health: %i", cg.snap->ps.stats[STAT_HEALTH]);

	CG_DrawBigString(8, cgs.screenHeight-128, s, 1.0f);

	sprintf(s, "Armor: %i", cg.snap->ps.stats[STAT_ARMOR]);

	CG_DrawBigString(8, cgs.screenHeight-112, s, 1.0f);
}
#endif

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
#if 0
	const char	*s;
	int			w;

	if (!cg_drawStatus.integer)
	{
		return;
	}

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(0.5f * (cgs.screenWidth - w), 64, s, 1.0f);
#endif
}



/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	float		w;
	int			sec;
	int			i;
	float scale;
	clientInfo_t	*ci1, *ci2;
	// int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
//		s = "Waiting for players";		
		s = CG_GetStripEdString("INGAMETEXT", "WAITING_FOR_PLAYERS");
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(0.5f * (cgs.screenWidth - w), 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s" S_COLOR_WHITE " vs %s", ci1->name, ci2->name );
			w = CG_Text_Width(s, 0.6f, FONT_MEDIUM);
			CG_Text_Paint(0.5f * (cgs.screenWidth - w), 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_HOLOCRON ) {
			s = "Holocron FFA";
		} else if ( cgs.gametype == GT_JEDIMASTER ) {
			s = "Jedi Master";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team FFA";
		} else if ( cgs.gametype == GT_SAGA ) {
			s = "N/A";
		} else if ( cgs.gametype == GT_CTF ) {
			s = "Capture the Flag";
		} else if ( cgs.gametype == GT_CTY ) {
			s = "Capture the Ysalamiri";
		} else {
			s = "";
		}
		w = CG_Text_Width(s, 1.5f, FONT_MEDIUM);
		CG_Text_Paint(0.5f * (cgs.screenWidth - w), 90, 1.5f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
//	s = va( "Starts in: %i", sec + 1 );
	s = va( "%s: %i",CG_GetStripEdString("INGAMETEXT", "STARTS_IN"), sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		switch ( sec ) {
		case 0:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		default:
			break;
		}
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		// cw = 28;
		scale = 1.25f;
		break;
	case 1:
		// cw = 24;
		scale = 1.15f;
		break;
	case 2:
		// cw = 20;
		scale = 1.05f;
		break;
	default:
		// cw = 16;
		scale = 0.9f;
		break;
	}

	w = CG_Text_Width(s, scale, FONT_MEDIUM);
	CG_Text_Paint(0.5f * (cgs.screenWidth - w), 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
}

//==================================================================================
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

void CG_DrawFlagStatus()
{
	int myFlagTakenShader = 0;
	int theirFlagShader = 0;
	int team = 0;
	int startDrawPos = 2;
	int ico_size = 32;

	if (!cg.snap)
	{
		return;
	}

	if (cgs.gametype != GT_CTF && cgs.gametype != GT_CTY)
	{
		return;
	}

	team = cg.snap->ps.persistant[PERS_TEAM];

	if (cgs.gametype == GT_CTY)
	{
		if (team == TEAM_RED)
		{
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
		}
		else
		{
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
		}
	}
	else
	{
		if (team == TEAM_RED)
		{
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag" );
		}
		else
		{
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag" );
		}
	}

	if (CG_YourTeamHasFlag())
	{
		CG_DrawPic( startDrawPos, cgs.screenHeight-115, ico_size, ico_size, theirFlagShader );
		startDrawPos += ico_size+2;
	}

	if (CG_OtherTeamHasFlag())
	{
		CG_DrawPic( startDrawPos, cgs.screenHeight-115, ico_size, ico_size, myFlagTakenShader );
	}
}

int cgRageTime = 0;
int cgRageFadeTime = 0;
float cgRageFadeVal = 0;

int cgRageRecTime = 0;
int cgRageRecFadeTime = 0;
float cgRageRecFadeVal = 0;

int cgAbsorbTime = 0;
int cgAbsorbFadeTime = 0;
float cgAbsorbFadeVal = 0;

int cgProtectTime = 0;
int cgProtectFadeTime = 0;
float cgProtectFadeVal = 0;

int cgYsalTime = 0;
int cgYsalFadeTime = 0;
float cgYsalFadeVal = 0;

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			bestTime;
	int				drawSelect = 0;
	float			fallTime, rageTime, rageRecTime, absorbTime, protectTime, ysalTime;
	vec4_t			hcolor;

	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		cgRageTime = 0;
		cgRageFadeTime = 0;
		cgRageFadeVal = 0;

		cgRageRecTime = 0;
		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;

		cgAbsorbTime = 0;
		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;

		cgProtectTime = 0;
		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;

		cgYsalTime = 0;
		cgYsalFadeTime = 0;
		cgYsalFadeVal = 0;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_RAGE))
		{
			if (!cgRageTime)
			{
				cgRageTime = cg.time;
			}
			
			rageTime = (float)(cg.time - cgRageTime);
			
			rageTime /= 9000;
			
			if (rageTime < 0)
			{
				rageTime = 0;
			}
			if (rageTime > 0.15)
			{
				rageTime = 0.15;
			}
			
			hcolor[3] = rageTime;
			hcolor[0] = 0.7;
			hcolor[1] = 0;
			hcolor[2] = 0;
			
			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			
			cgRageFadeTime = 0;
			cgRageFadeVal = 0;
		}
		else if (cgRageTime)
		{
			if (!cgRageFadeTime)
			{
				cgRageFadeTime = cg.time;
				cgRageFadeVal = 0.15;
			}
			
			rageTime = cgRageFadeVal;
			
			cgRageFadeVal -= (cg.time - cgRageFadeTime)*0.000005;
			
			if (rageTime < 0)
			{
				rageTime = 0;
			}
			if (rageTime > 0.15)
			{
				rageTime = 0.15;
			}
			
			if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
			{
				float checkRageRecTime = rageTime;
				
				if (checkRageRecTime < 0.15)
				{
					checkRageRecTime = 0.15;
				}
				
				hcolor[3] = checkRageRecTime;
				hcolor[0] = rageTime*4;
				if (hcolor[0] < 0.2)
				{
					hcolor[0] = 0.2;
				}
				hcolor[1] = 0.2;
				hcolor[2] = 0.2;
			}
			else
			{
				hcolor[3] = rageTime;
				hcolor[0] = 0.7;
				hcolor[1] = 0;
				hcolor[2] = 0;
			}
			
			if (!cg.renderingThirdPerson && rageTime)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			else
			{
				if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
				{
					hcolor[3] = 0.15;
					hcolor[0] = 0.2;
					hcolor[1] = 0.2;
					hcolor[2] = 0.2;
					CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
				}
				cgRageTime = 0;
			}
		}
		else if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
		{
			if (!cgRageRecTime)
			{
				cgRageRecTime = cg.time;
			}
			
			rageRecTime = (float)(cg.time - cgRageRecTime);
			
			rageRecTime /= 9000;
			
			if (rageRecTime < 0.15)//0)
			{
				rageRecTime = 0.15;//0;
			}
			if (rageRecTime > 0.15)
			{
				rageRecTime = 0.15;
			}
			
			hcolor[3] = rageRecTime;
			hcolor[0] = 0.2;
			hcolor[1] = 0.2;
			hcolor[2] = 0.2;
			
			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			
			cgRageRecFadeTime = 0;
			cgRageRecFadeVal = 0;
		}
		else if (cgRageRecTime)
		{
			if (!cgRageRecFadeTime)
			{
				cgRageRecFadeTime = cg.time;
				cgRageRecFadeVal = 0.15;
			}
			
			rageRecTime = cgRageRecFadeVal;
			
			cgRageRecFadeVal -= (cg.time - cgRageRecFadeTime)*0.000005;
			
			if (rageRecTime < 0)
			{
				rageRecTime = 0;
			}
			if (rageRecTime > 0.15)
			{
				rageRecTime = 0.15;
			}
			
			hcolor[3] = rageRecTime;
			hcolor[0] = 0.2;
			hcolor[1] = 0.2;
			hcolor[2] = 0.2;
			
			if (!cg.renderingThirdPerson && rageRecTime)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			else
			{
				cgRageRecTime = 0;
			}
		}
		
		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_ABSORB))
		{
			if (!cgAbsorbTime)
			{
				cgAbsorbTime = cg.time;
			}
			
			absorbTime = (float)(cg.time - cgAbsorbTime);
			
			absorbTime /= 9000;
			
			if (absorbTime < 0)
			{
				absorbTime = 0;
			}
			if (absorbTime > 0.15)
			{
				absorbTime = 0.15;
			}
			
			hcolor[3] = absorbTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7;
			
			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			
			cgAbsorbFadeTime = 0;
			cgAbsorbFadeVal = 0;
		}
		else if (cgAbsorbTime)
		{
			if (!cgAbsorbFadeTime)
			{
				cgAbsorbFadeTime = cg.time;
				cgAbsorbFadeVal = 0.15;
			}
			
			absorbTime = cgAbsorbFadeVal;
			
			cgAbsorbFadeVal -= (cg.time - cgAbsorbFadeTime)*0.000005;
			
			if (absorbTime < 0)
			{
				absorbTime = 0;
			}
			if (absorbTime > 0.15)
			{
				absorbTime = 0.15;
			}
			
			hcolor[3] = absorbTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7;
			
			if (!cg.renderingThirdPerson && absorbTime)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			else
			{
				cgAbsorbTime = 0;
			}
		}
		
		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_PROTECT))
		{
			if (!cgProtectTime)
			{
				cgProtectTime = cg.time;
			}
			
			protectTime = (float)(cg.time - cgProtectTime);
			
			protectTime /= 9000;
			
			if (protectTime < 0)
			{
				protectTime = 0;
			}
			if (protectTime > 0.15)
			{
				protectTime = 0.15;
			}
			
			hcolor[3] = protectTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0.7;
			hcolor[2] = 0;
			
			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			
			cgProtectFadeTime = 0;
			cgProtectFadeVal = 0;
		}
		else if (cgProtectTime)
		{
			if (!cgProtectFadeTime)
			{
				cgProtectFadeTime = cg.time;
				cgProtectFadeVal = 0.15;
			}
			
			protectTime = cgProtectFadeVal;
			
			cgProtectFadeVal -= (cg.time - cgProtectFadeTime)*0.000005;
			
			if (protectTime < 0)
			{
				protectTime = 0;
			}
			if (protectTime > 0.15)
			{
				protectTime = 0.15;
			}
			
			hcolor[3] = protectTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0.7;
			hcolor[2] = 0;
			
			if (!cg.renderingThirdPerson && protectTime)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			else
			{
				cgProtectTime = 0;
			}
		}
		
		if (cg.snap->ps.rocketLockIndex != MAX_CLIENTS && (cg.time - cg.snap->ps.rocketLockTime) > 0)
		{
			CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
		}
		
		if (BG_HasYsalamiri(cgs.gametype, &cg.snap->ps))
		{
			if (!cgYsalTime)
			{
				cgYsalTime = cg.time;
			}
			
			ysalTime = (float)(cg.time - cgYsalTime);
			
			ysalTime /= 9000;
			
			if (ysalTime < 0)
			{
				ysalTime = 0;
			}
			if (ysalTime > 0.15)
			{
				ysalTime = 0.15;
			}
			
			hcolor[3] = ysalTime/2;
			hcolor[0] = 0.7;
			hcolor[1] = 0.7;
			hcolor[2] = 0;
			
			if (!cg.renderingThirdPerson)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			
			cgYsalFadeTime = 0;
			cgYsalFadeVal = 0;
		}
		else if (cgYsalTime)
		{
			if (!cgYsalFadeTime)
			{
				cgYsalFadeTime = cg.time;
				cgYsalFadeVal = 0.15;
			}
			
			ysalTime = cgYsalFadeVal;
			
			cgYsalFadeVal -= (cg.time - cgYsalFadeTime)*0.000005;
			
			if (ysalTime < 0)
			{
				ysalTime = 0;
			}
			if (ysalTime > 0.15)
			{
				ysalTime = 0.15;
			}
			
			hcolor[3] = ysalTime/2;
			hcolor[0] = 0.7;
			hcolor[1] = 0.7;
			hcolor[2] = 0;
			
			if (!cg.renderingThirdPerson && ysalTime)
			{
				CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
			}
			else
			{
				cgYsalTime = 0;
			}
		}
	}

	if (cg.snap->ps.rocketLockIndex != MAX_CLIENTS && (cg.time - cg.snap->ps.rocketLockTime) > 0)
	{
		CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
	}

	if (cg.snap->ps.holocronBits)
	{
		CG_DrawHolocronIcons();
	}
	if (cg.snap->ps.fd.forcePowersActive || cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawActivePowers();
	}

	// Draw this before the text so that any text won't get clipped off
	CG_DrawZoomMask();

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshair(NULL, 0);
		CG_DrawCrosshairNames();
		CG_SaberClashFlare();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			if ( /*cg_drawStatus.integer*/0 ) {
				//Reenable if stats are drawn with menu system again
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
      
			//CG_DrawTemporaryStats();

			CG_DrawAmmoWarning();

			CG_DrawCrosshairNames();

			if (cg_drawStatus.integer)
			{
				CG_DrawIconBackground();
			}

			if (inTime > wpTime)
			{
				drawSelect = 1;
				bestTime = cg.invenSelectTime;
			}
			else //only draw the most recent since they're drawn in the same place
			{
				drawSelect = 2;
				bestTime = cg.weaponSelectTime;
			}

			if (cg.forceSelectTime > bestTime)
			{
				drawSelect = 3;
			}

			switch(drawSelect)
			{
			case 1:
				CG_DrawInvenSelect();
				break;
			case 2:
				CG_DrawWeaponSelect();
				break;
			case 3:
				CG_DrawForceSelect();
				break;
			default:
				break;
			}

			if (cg_drawStatus.integer)
			{
				//Powerups now done with upperright stuff
				//CG_DrawPowerupIcons();

				CG_DrawFlagStatus();
			}

			CG_SaberClashFlare();

			if (cg_drawStatus.integer)
			{
				CG_DrawStats();
			}

			//Do we want to use this system again at some point?
			//CG_DrawReward();
		}
    
	}

	if (cg.snap->ps.fallingToDeath)
	{
		fallTime = (float)(cg.time - cg.snap->ps.fallingToDeath);

		fallTime /= (FALL_FADE_TIME/2);

		if (fallTime < 0)
		{
			fallTime = 0;
		}
		if (fallTime > 1)
		{
			fallTime = 1;
		}

		hcolor[3] = fallTime;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_FillRect(0, 0, cgs.screenWidth, cgs.screenHeight, hcolor);
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

	if (!cg_paused.integer) {
		CG_DrawUpperRight();
	}

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}
}


static void CG_DrawTourneyScoreboard() {
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// draw status bar and other floating elements
 	CG_Draw2D();
}

/*
===================
CG_CalculateSpeed
japro - calculate the current speed
===================
*/
static void CG_CalculateSpeed(centity_t* cent) {
	const vec_t* const velocity = (cent->currentState.clientNum == cg.clientNum ? cg.predictedPlayerState.velocity : cent->currentState.pos.trDelta);
	cg.currentSpeed = (float)sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);
}

/*
===================
CG_GraphAddSpeed
tremulous - append a speed to the sample history for the speed graph
===================
*/
static void CG_GraphAddSpeed(void) {
	float speed;
	vec3_t vel;

	VectorCopy(cg.snap->ps.velocity, vel);

	speed = VectorLength(vel);

	if (speed > speedSamples[maxSpeedSample])
	{
		maxSpeedSample = oldestSpeedSample;
		speedSamples[oldestSpeedSample++] = speed;
		oldestSpeedSample %= SPEEDOMETER_NUM_SAMPLES;
		return;
	}

	speedSamples[oldestSpeedSample] = speed;
	if (maxSpeedSample == oldestSpeedSample++) // if old max was overwritten find a new one
	{
		int i;
		for (maxSpeedSample = 0, i = 1; i < SPEEDOMETER_NUM_SAMPLES; i++)
		{
			if (speedSamples[i] > speedSamples[maxSpeedSample])
				maxSpeedSample = i;
		}
	}

	oldestSpeedSample %= SPEEDOMETER_NUM_SAMPLES;
}

/*
===================
CG_DrawSpeedGraph
tremulous - speedgraph initially ported by TomArrow
===================
*/
static void CG_DrawSpeedGraph(rectDef_t* rect, const vec4_t foreColor, vec4_t backColor) {
	int i;
	float val, max, top;
	// colour of graph is interpolated between these values
	const vec3_t slow = { 0.0f, 0.0f, 1.0f };
	const vec3_t medium = { 0.0f, 1.0f, 0.0f };
	const vec3_t fast = { 1.0f, 0.0f, 0.0f };
	vec4_t color;

	max = speedSamples[maxSpeedSample];
	if (max < SPEEDOMETER_MIN_RANGE)
		max = SPEEDOMETER_MIN_RANGE;

	trap_R_SetColor(backColor);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.whiteShader);

	Vector4Copy(foreColor, color);

	for (i = 1; i < SPEEDOMETER_NUM_SAMPLES; i++)
	{
		val = speedSamples[(oldestSpeedSample + i) % SPEEDOMETER_NUM_SAMPLES];
		if (val < SPEED_MED)
			VectorLerp(val / SPEED_MED, slow, medium, color);
		else if (val < SPEED_FAST)
			VectorLerp((val - SPEED_MED) / (SPEED_FAST - SPEED_MED),
				medium, fast, color);
		else
			VectorCopy(fast, color);
		trap_R_SetColor(color);
		top = rect->y + (1 - val / max) * rect->h;
		CG_DrawPic(rect->x + ((float)i / (float)SPEEDOMETER_NUM_SAMPLES) * rect->w, top,
			rect->w / (float)SPEEDOMETER_NUM_SAMPLES, val * rect->h / max,
			cgs.media.whiteShader);
	}
	trap_R_SetColor(NULL);
}

/*
===================
CG_DrawJumpHeight
japro - Draw speedometer jump height
===================
*/
static void CG_DrawJumpHeight(centity_t* cent) {
	const vec_t* const velocity = (cent->currentState.clientNum == cg.clientNum ? cg.predictedPlayerState.velocity : cent->currentState.pos.trDelta);
	char jumpHeightStr[32] = { 0 };

	if (!pm || !pm->ps)
		return;

	if (pm->ps->fd.forceJumpZStart == -65536) //Coming back from a tele
		return;

	if (pm->ps->fd.forceJumpZStart && (cg.lastZSpeed > 0) && (velocity[2] <= 0)) {//If we were going up, and we are now going down, print our height.
		cg.lastJumpHeight = pm->ps->origin[2] - pm->ps->fd.forceJumpZStart;
		cg.lastJumpHeightTime = cg.time;
	}

	if ((cg.lastJumpHeightTime > cg.time - 1500) && (cg.lastJumpHeight > 0.0f)) {
		Com_sprintf(jumpHeightStr, sizeof(jumpHeightStr), "%.1f", cg.lastJumpHeight);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.integer, cg_speedometerSize.value, colorTable[CT_WHITE], jumpHeightStr, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
	}

	speedometerXPos += 42;

	cg.lastZSpeed = velocity[2];
}

/*
===================
CG_DrawJumpDistance
japro - Draw speedometer jump distance
===================
*/
static void CG_DrawJumpDistance(void) {
	char jumpDistanceStr[64] = { 0 };

	if (!cg.snap)
		return;

	if (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_WORLD) {

		if (!cg.wasOnGround) {//We were just in the air, but now we arnt
			vec3_t distance;

			VectorSubtract(cg.predictedPlayerState.origin, cg.lastGroundPosition, distance);
			cg.lastJumpDistance = (float)sqrt(distance[0] * distance[0] + distance[1] * distance[1]);
			cg.lastJumpDistanceTime = cg.time;
		}

		VectorCopy(cg.predictedPlayerState.origin, cg.lastGroundPosition);
		cg.wasOnGround = qtrue;
	}
	else {
		cg.wasOnGround = qfalse;
	}

	if ((cg.lastJumpDistanceTime > cg.time - 1500) && (cg.lastJumpDistance > 0.0f)) {
		Com_sprintf(jumpDistanceStr, sizeof(jumpDistanceStr), "%.1f", cg.lastJumpDistance);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.integer, cg_speedometerSize.value, colorTable[CT_WHITE], jumpDistanceStr, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
	}

	speedometerXPos += 62;
}

/*
===================
CG_DrawVerticalSpeed
japro - Draw speedometer vertical speed
===================
*/
static void CG_DrawVerticalSpeed(void) {
	char speedStr5[64] = { 0 };
	float vertspeed = cg.predictedPlayerState.velocity[2];

	if (vertspeed < 0)
		vertspeed = -vertspeed;

	if (vertspeed) {
		Com_sprintf(speedStr5, sizeof(speedStr5), "%.0f", vertspeed);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.integer, cg_speedometerSize.value, colorWhite, speedStr5, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
	}

	speedometerXPos += 42;
}

/*
===================
CG_DrawAccelMeter
japro - Draw acceleration meter
===================
*/
static void CG_DrawAccelMeter(void) {
	const float optimalAccel = cg.predictedPlayerState.speed * ((float)cg.frametime / 1000.0f);
	const float potentialSpeed = (float)sqrt(cg.previousSpeed * cg.previousSpeed - optimalAccel * optimalAccel + 2 * (250 * optimalAccel));
	float actualAccel, total, percentAccel, x;
	const float accel = cg.currentSpeed - cg.previousSpeed;
	static int t, i, previous, lastupdate;
	unsigned int frameTime;
	static float previousTimes[PERCENT_SAMPLES];
	static unsigned int index;

	x = speedometerXPos;

	if (cg_speedometer.integer & SPEEDOMETER_GROUNDSPEED)
		x -= 88;
	else
		x -= 52;

	CG_DrawRect(x - 0.75,
		cg_speedometerY.value - 10.75,
		37.75,
		13.75,
		0.5f,
		colorTable[CT_BLACK]);

	actualAccel = accel;
	if (actualAccel < 0)
		actualAccel = 0.001f;
	else if (actualAccel > (potentialSpeed - cg.currentSpeed)) //idk how
		actualAccel = (potentialSpeed - cg.currentSpeed) * 0.99f;

	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;
	{
		lastupdate = t;
		previousTimes[index % PERCENT_SAMPLES] = actualAccel / (potentialSpeed - cg.currentSpeed);
		index++;
	}

	total = 0;
	for (i = 0; i < PERCENT_SAMPLES; i++) {
		total += previousTimes[i];
	}
	if (!total) {
		total = 1;
	}
	percentAccel = total / (float)PERCENT_SAMPLES;

	if (percentAccel && cg.currentSpeed) {
		CG_FillRect(x,
			cg_speedometerY.value - 9.9f,
			36 * percentAccel,
			12,
			colorTable[CT_RED]);
	}

	cg.previousSpeed = cg.currentSpeed;
}

/*
===================
CG_DrawSpeedometer
japro - Draw the speedometer
===================
*/
static void CG_DrawSpeedometer(void) {
	const char* accelStr, * accelStr2, * accelStr3;
	char speedStr[32] = { 0 }, speedStr2[32] = { 0 }, speedStr3[32] = { 0 };
	vec4_t colorSpeed = { 1, 1, 1, 1 };
	const float currentSpeed = cg.currentSpeed;
	static float lastSpeed = 0, previousAccels[ACCEL_SAMPLES];
	const float accel = currentSpeed - lastSpeed;
	float total, avgAccel, groundSpeedColor, groundSpeedsColor, currentSpeedColor;
	int t, i;
	static unsigned int index;
	static int lastupdate, jumpsCounter = 0;
	static qboolean clearOnNextJump = qfalse;

	lastSpeed = currentSpeed;

	if (currentSpeed > 250 && !(cg_speedometer.integer & SPEEDOMETER_COLORS))
	{
		currentSpeedColor = 1 / ((currentSpeed / 250) * (currentSpeed / 250));
		colorSpeed[1] = currentSpeedColor;
		colorSpeed[2] = currentSpeedColor;
	}

	t = trap_Milliseconds();

	if (t - lastupdate > 5)	//don't sample faster than this
	{
		lastupdate = t;
		previousAccels[index % ACCEL_SAMPLES] = accel;
		index++;
	}

	total = 0;
	for (i = 0; i < ACCEL_SAMPLES; i++) {
		total += previousAccels[i];
	}
	if (!total) {
		total = 1;
	}
	avgAccel = total / (float)ACCEL_SAMPLES - 0.0625f;//fucking why does it offset by this number

	if (avgAccel > 0.0f)
	{
		accelStr = S_COLOR_GREEN "\xb5:";
		accelStr2 = S_COLOR_GREEN "k:";
		accelStr3 = S_COLOR_GREEN "m: ";
	}
	else if (avgAccel < 0.0f)
	{
		accelStr = S_COLOR_RED "\xb5:";
		accelStr2 = S_COLOR_RED "k:";
		accelStr3 = S_COLOR_RED "m: ";
	}
	else
	{
		accelStr = S_COLOR_WHITE "\xb5:";
		accelStr2 = S_COLOR_WHITE "k:";
		accelStr3 = S_COLOR_WHITE "m: ";
	}

	if (!(cg_speedometer.integer & SPEEDOMETER_KPH) && !(cg_speedometer.integer & SPEEDOMETER_MPH))
	{
		Com_sprintf(speedStr, sizeof(speedStr), "   %.0f", (float)floor(currentSpeed + 0.5f));
		CG_Text_Paint(speedometerXPos, cg_speedometerY.integer, cg_speedometerSize.value, colorWhite, accelStr, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorSpeed, speedStr, 0.0f, 0, ITEM_ALIGN_LEFT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
	}
	else if (cg_speedometer.integer & SPEEDOMETER_KPH)
	{
		Com_sprintf(speedStr2, sizeof(speedStr2), "   %.1f", currentSpeed * 0.05);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorWhite, accelStr2, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorSpeed, speedStr2, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
	}
	else if (cg_speedometer.integer & SPEEDOMETER_MPH)
	{
		Com_sprintf(speedStr3, sizeof(speedStr3), "   %.1f", currentSpeed * 0.03106855);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorWhite, accelStr3, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
		CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorSpeed, speedStr3, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
	}
	speedometerXPos += 52;

	if (cg_speedometer.integer & SPEEDOMETER_GROUNDSPEED || (cg_speedometer.integer && (cg_speedometer.integer & SPEEDOMETER_JUMPS))) {
		char speedStr4[32] = { 0 };
		char speedsStr4[32] = { 0 };

		vec4_t colorGroundSpeed = { 1, 1, 1, 1 };
		vec4_t colorGroundSpeeds = { 1, 1, 1, 1 };

		if (cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE || cg.predictedPlayerState.velocity[2] < 0) { //On ground or Moving down
			cg.firstTimeInAir = qfalse;
		}
		else if (!cg.firstTimeInAir) { //Moving up for first time
			cg.firstTimeInAir = qtrue;
			cg.lastGroundSpeed = currentSpeed;
			cg.lastGroundTime = cg.time;
			if (cg_speedometer.integer & SPEEDOMETER_JUMPS) {

				if (clearOnNextJump == qtrue) {
					memset(&cg.lastGroundSpeeds, 0, sizeof(cg.lastGroundSpeeds));
					jumpsCounter = 0;
					clearOnNextJump = qfalse;
				}
				cg.lastGroundSpeeds[++jumpsCounter] = cg.lastGroundSpeed; //add last ground speed to the array
			}
		}
		if (cg_speedometer.integer & SPEEDOMETER_JUMPS) {
			if ((cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE &&
				cg.predictedPlayerState.pm_time <= 0 && cg.currentSpeed < 250) || cg.currentSpeed == 0) {
				clearOnNextJump = qtrue;
			}
			if (cg_speedometerJumps.value &&
				(jumpsCounter < cg_speedometerJumps.integer)) { //if we are in the first n jumps
				for (i = 0; i <= cg_speedometerJumps.integer; i++) { //print the jumps
					groundSpeedsColor = 1 / ((cg.lastGroundSpeeds[i] / 250) * (cg.lastGroundSpeeds[i] / 250));
					Com_sprintf(speedsStr4, sizeof(speedsStr4), "%.0f", cg.lastGroundSpeeds[i]); //create the string
					if (cg_speedometer.integer & SPEEDOMETER_JUMPSCOLORS1) { //color the string
						colorGroundSpeeds[1] = groundSpeedsColor;
						colorGroundSpeeds[2] = groundSpeedsColor;
					}
					else if (cg_speedometer.integer & SPEEDOMETER_JUMPSCOLORS2) {
						if ((jumpsCounter > 0 && (cg.lastGroundSpeeds[i] > cg.lastGroundSpeeds[i - 1])) ||
							(i == 0 && (cg.lastGroundSpeeds[0] > firstSpeed))) {
							colorGroundSpeeds[0] = groundSpeedsColor;
							colorGroundSpeeds[1] = 1;
							colorGroundSpeeds[2] = groundSpeedsColor;
						}
						else {
							colorGroundSpeeds[0] = 1;
							colorGroundSpeeds[1] = groundSpeedsColor;
							colorGroundSpeeds[2] = groundSpeedsColor;
						}
					}
					if (strcmp(speedsStr4, "0") != 0) {
						CG_Text_Paint((jumpsXPos), cg_speedometerJumpsY.value,
							cg_speedometerSize.value, colorGroundSpeeds, speedsStr4, 0.0f, 0,
							ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE); //print the jump
						jumpsXPos += 52; //shift x
					}
				}
			}
			else if (cg_speedometerJumps.value &&
				jumpsCounter == cg_speedometerJumps.integer) { //we out of the first n jumps
				firstSpeed = cg.lastGroundSpeeds[0];
				for (i = 0; i <= cg_speedometerJumps.integer; i++) { //shuffle jumps array down
					cg.lastGroundSpeeds[i] = cg.lastGroundSpeeds[i + 1];
				}
				jumpsCounter--;  //reduce jump counter
			}
		}

		groundSpeedColor = 1 / ((cg.lastGroundSpeed / 250) * (cg.lastGroundSpeed / 250));
		if (cg_jumpGoal.value && (cg_jumpGoal.value <= cg.lastGroundSpeed) && jumpsCounter == 1) {
			colorGroundSpeed[0] = groundSpeedColor;
			colorGroundSpeed[1] = 1;
			colorGroundSpeed[2] = groundSpeedColor;
		}
		else if (cg.lastGroundSpeed > 250 && !(cg_speedometer.integer & SPEEDOMETER_COLORS)) {
			colorGroundSpeed[0] = 1;
			colorGroundSpeed[1] = groundSpeedColor;
			colorGroundSpeed[2] = groundSpeedColor;
		}

		if ((cg.lastGroundTime > cg.time - 1500) && (cg_speedometer.integer & SPEEDOMETER_GROUNDSPEED)) {
			if (cg.lastGroundSpeed) {
				Com_sprintf(speedStr4, sizeof(speedStr4), "%.0f", cg.lastGroundSpeed);
				CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorGroundSpeed, speedStr4, 0.0f, 0, ITEM_ALIGN_LEFT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
			}
		}
		speedometerXPos += 52;
	}
}

/*
===================
CG_DrawLine
japro - draw a line (previously known as Dzikie_CG_DrawLine)
===================
*/
static void CG_DrawLine(float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff) {
	float stepx, stepy, length = (float)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	int i;

	if (length < 1)
		length = 1;
	else if (length > 2000)
		length = 2000;
	if (!ycutoff)
		ycutoff = 480;

	stepx = (x2 - x1) / (length / size);
	stepy = (y2 - y1) / (length / size);

	trap_R_SetColor(color);

	for (i = 0; i <= (length / size); i++) {
		if (x1 < 640 && y1 < 480 && y1 < ycutoff)
			CG_DrawPic(x1, y1, size, size, cgs.media.whiteShader);
		x1 += stepx;
		y1 += stepy;
	}
}

/*
===================
CG_DrawStrafehelperWeze
japro - draw the weze style strafehelper(previously known as Dzikie_CG_DrawSpeed)
===================
*/
static void CG_DrawStrafehelperWeze(int moveDir) {
	float length;
	float diff;
	float midx;
	float midy;
	vec3_t velocity_copy;
	vec3_t viewangle_copy;
	vec3_t velocity_angle;
	float g_speed;
	float accel;
	float optiangle;
	usercmd_t cmd = { 0 };

	if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) {
		trap_GetUserCmd(trap_GetCurrentCmdNumber(), &cmd);
	}
	else if (cg.snap) {
		moveDir = cg.snap->ps.movementDir;
		switch (moveDir) {
		case 0: // W
			cmd.forwardmove = 127; break;
		case 1: // WA
			cmd.forwardmove = 127; cmd.rightmove = -127; break;
		case 2: // A
			cmd.rightmove = -127;	break;
		case 3: // AS
			cmd.rightmove = -127;	cmd.forwardmove = -127; break;
		case 4: // S
			cmd.forwardmove = -127; break;
		case 5: // SD
			cmd.forwardmove = -127; cmd.rightmove = 127; break;
		case 6: // D
			cmd.rightmove = 127; break;
		case 7: // DW
			cmd.rightmove = 127; cmd.forwardmove = 127;	break;
		default:
			break;
		}
	}
	else {
		return; //No cg.snap causes this to return.
	}

	midx = 0.5f * cgs.screenWidth;
	midy = 0.5f * cgs.screenHeight;
	VectorCopy(cg.predictedPlayerState.velocity, velocity_copy);
	velocity_copy[2] = 0;
	VectorCopy(cg.refdefViewAngles, viewangle_copy);
	viewangle_copy[PITCH] = 0;
	length = VectorNormalize(velocity_copy);
	g_speed = cg.predictedPlayerState.speed;
	accel = g_speed;
	accel *= 8.0f;
	accel /= 1000;
	optiangle = (g_speed - accel) / length;
	if ((optiangle <= 1) && (optiangle >= -1))
		optiangle = acos(optiangle);
	else
		optiangle = 0;
	length /= 5;
	if (length > (0.5f * cgs.screenHeight))
		length = (float)(0.5f * cgs.screenHeight);
	vectoangles(velocity_copy, velocity_angle);
	diff = AngleSubtract(viewangle_copy[YAW], velocity_angle[YAW]);
	diff = diff / 180 * M_PI;

	CG_DrawLine(midx, midy, midx + length * sin(diff), midy - length * cos(diff), 1, colorRed, 0.75f, 0);
	CG_DrawLine(midx, midy, midx + cmd.rightmove, midy - cmd.forwardmove, 1, colorCyan, 0.75f, 0);
	CG_DrawLine(midx, midy, midx + length / 2 * sin(diff + optiangle), midy - length / 2 * cos(diff + optiangle), 1, colorRed, 0.75f, 0);
	CG_DrawLine(midx, midy, midx + length / 2 * sin(diff - optiangle), midy - length / 2 * cos(diff - optiangle), 1, colorRed, 0.75f, 0);
}

/*
===================
CG_StrafeHelperSound
japro - play strafehelper sounds
===================
*/
static void CG_StrafeHelperSound(float difference) {
	if (difference > -40.0f && difference < 10.0f) //Under aiming by a bit, but still good
		trap_S_StartLocalSound(cgs.media.hitSound4, CHAN_LOCAL_SOUND);
}

/*
===================
CG_DrawStrafeLine
japro - this function is essentially a wrapper between CG_DrawLine and CG_Strafehelper (Previously DrawStrafeLine)
===================
*/
static void CG_DrawStrafeLine(vec3_t velocity, float diff, qboolean active, int moveDir) {
	vec3_t start, angs, forward, delta, line;
	float x, y, lineWidth;
	int sensitivity = cg_strafeHelperPrecision.integer;
	static const int LINE_HEIGHT = 230; //240 is midpoint, so it should be a little higher so crosshair is always on it.
	static const vec4_t normalColor = { 1, 1, 1, 0.75 }, invertColor = { 0.5f, 1, 1, 0.75 }, wColor = { 1, 0.5, 0.5, 0.75 }, rearColor = { 0.5, 1,1, 0.75 }, centerColor = { 0.5, 1, 1, 0.75 }; //activeColor = {0, 1, 0, 0.75},
	vec4_t color = { 1, 1, 1, 0.75 };

	if (cg_strafeHelperPrecision.integer < 100) //set the precision/sensitivity
		sensitivity = 100;
	else if (cg_strafeHelperPrecision.integer > 10000)
		sensitivity = 10000;

	lineWidth = cg_strafeHelperLineWidth.value; //get the line width
	if (lineWidth < 0.25f)
		lineWidth = 0.25f;
	else if (lineWidth > 5)
		lineWidth = 5;

	if (active) { //set the active colour
		color[0] = cg.strafeHelperActiveColor[0];
		color[1] = cg.strafeHelperActiveColor[1];
		color[2] = cg.strafeHelperActiveColor[2];
		color[3] = cg.strafeHelperActiveColor[3];
	}

	else { //set the other colours
		if (moveDir == 1 || moveDir == 7)
			memcpy(color, normalColor, sizeof(vec4_t));
		else if (moveDir == 2 || moveDir == 6)
			memcpy(color, invertColor, sizeof(vec4_t));
		else if (moveDir == 0 || moveDir == 4 || moveDir == 3 || moveDir == 5)
			memcpy(color, wColor, sizeof(vec4_t));
		else if (moveDir == 8)
			memcpy(color, centerColor, sizeof(vec4_t));
		else if (moveDir == 9 || moveDir == 10)
			memcpy(color, rearColor, sizeof(vec4_t));
		color[3] = cg_strafeHelperInactiveAlpha.value / 255.0f;
	}

	VectorCopy(cg.refdef.vieworg, start); //get the view angles
	VectorCopy(velocity, angs);
	angs[YAW] += diff;
	AngleVectors(angs, forward, NULL, NULL);
	VectorScale(forward, sensitivity, delta); // set the line length

	line[0] = delta[0] + start[0]; //set the line coords
	line[1] = delta[1] + start[1];
	line[2] = start[2];

	if (!CG_WorldCoordToScreenCoord(line, &x, &y)) //is it on the screen?
		return;

	x -= (0.5f * lineWidth);


	if (cg_strafeHelper.integer & SHELPER_UPDATED) { //draw the updated style here
		int cutoff = cgs.screenHeight - cg_strafeHelperCutoff.integer;
		int heightIn = LINE_HEIGHT;

		if (cg_strafeHelper.integer & SHELPER_TINY) {
			cutoff = LINE_HEIGHT + 15;
			heightIn = LINE_HEIGHT + 5;
		} else if (cutoff < LINE_HEIGHT + 20) {
			cutoff = LINE_HEIGHT + 20;
		} else if (cutoff > cgs.screenHeight) {
			cutoff = cgs.screenHeight;
		}
		CG_DrawLine((0.5f * cgs.screenWidth) - (0.5f * lineWidth), cgs.screenHeight, x, heightIn, lineWidth, color, color[3], cutoff);
	}
	if (cg_strafeHelper.integer & SHELPER_CGAZ) { //draw the cgaz style strafehelper
		if (cg_strafeHelperCutoff.integer > 256) {
			CG_DrawLine(x, (0.5f * cgs.screenHeight) + 4, x, (0.5f * cgs.screenHeight) - 4, lineWidth, color, 0.75f, 0); //maximum cutoff
		} else {
			CG_DrawLine(x, (0.5f * cgs.screenHeight) + 20 - (float)cg_strafeHelperCutoff.integer / 16.0f, x, (0.5f * cgs.screenHeight) - 20 + (float)cg_strafeHelperCutoff.integer / 16.0f, lineWidth, color, 0.75f, 0); //default/custom cutoff
		}
	}
	if (cg_strafeHelper.integer & SHELPER_WSW && active && moveDir != 0) { //draw the wsw style strafehelper, not sure how to deal with multiple lines for W only so we don't draw any, the proper way is to tell which line we are closest to aiming at and display the strafehelper for that
		float width = (float)(-4.444 * AngleSubtract(cg.predictedPlayerState.viewangles[YAW], angs[YAW]));
		CG_FillRect((0.5f * cgs.screenWidth), (0.5f * cgs.screenHeight), width, 12, colorTable[CT_RED]);
	}
	if (cg_strafeHelper.integer & SHELPER_WEZE) { //call the weze style strafehelper function
		CG_DrawStrafehelperWeze(moveDir);
	}
	if (cg_strafeHelper.integer & SHELPER_SOUND && active && moveDir != 8) { //strafehelper sounds - don't do them for the center line, since it's not really a strafe
		CG_StrafeHelperSound(100 * AngleSubtract(cg.predictedPlayerState.viewangles[YAW], angs[YAW]));
	}
}

/*
===================
CG_GetStrafeTriangleAccel
japro - gets the acceleration value to determine triangles colours
===================
*/
static int CG_GetStrafeTriangleAccel(void) {
	int i;
	float t, dt;
	float accel;
	float newSpeed;
	static float oldSpeed = 0.0f;
	static float oldTime = 0.0f;
	static float accelHistory[ACCEL_SAMPLE_COUNT] = { 0.0f };
	static int sampleCount = 0;

	t = (float)cg.time * 0.001f;
	dt = t - oldTime;
	if (dt < 0.0f)
	{
		oldTime = t;
	}
	else if (dt > 0.0f) { // raw acceleration
		newSpeed = cg.currentSpeed;
		accel = (newSpeed - oldSpeed) / dt;
		accelHistory[sampleCount & ACCEL_SAMPLE_MASK] = accel;
		sampleCount++;
		oldSpeed = newSpeed;
		oldTime = t;
	}

	// average accel for n frames (TODO: emphasis on later frames)
	accel = 0.0f;
	for (i = 0; i < ACCEL_SAMPLE_COUNT; i++)
		accel += accelHistory[i];
	accel /= (float)(ACCEL_SAMPLE_COUNT);

	return (int)(1000 * accel);
}

/*
===================
CG_GetStrafeTriangleStrafeX
japro - gets the x value for the triangle and checks it is on the screen
===================
*/
static float* CG_GetStrafeTriangleStrafeX(vec3_t velocity, float diff, float sensitivity) {
	vec3_t start, angs, forward, delta, line;
	static float x, y;

	//get sensitivity
	if (cg_strafeHelperPrecision.integer < 100)
		sensitivity = 100.0f;
	else if (cg_strafeHelperPrecision.integer > 10000)
		sensitivity = 10000.0f;

	VectorCopy(cg.refdef.vieworg, start); //where we are looking

	VectorCopy(velocity, angs); //copy our velocity vector
	angs[YAW] += diff; //adjust the yaw angle according to the difference
	AngleVectors(angs, forward, NULL, NULL); //adjust the angle of the vectors
	VectorScale(forward, sensitivity, delta); //scale to where we want to be

	line[0] = delta[0] + start[0]; //draw the line
	line[1] = delta[1] + start[1];
	line[2] = start[2];

	if (!CG_WorldCoordToScreenCoord(line, &x, &y)) { //check it exists
		return NULL;
	}

	return &x;
}

/*
===================
CG_DrawStrafeTriangles
japro - draws the strafe triangles
===================
*/
static void CG_DrawStrafeTriangles(vec3_t velocity, float diff, float baseSpeed, int moveDir) {
	float lx, lx2, rx, rx2, lineWidth, sensitivity, accel, optimalAccel, potentialSpeed;
	static float lWidth, rWidth, width;
	vec4_t color1 = { 1.0f, 1.0f, 1.0f, 0.8f };

	accel = (float)CG_GetStrafeTriangleAccel();
	optimalAccel = baseSpeed * ((float)cg.frametime / 1000.0f);
	potentialSpeed = cg.previousSpeed * cg.previousSpeed - optimalAccel * optimalAccel + 2.0f * (250.0f * optimalAccel);

	if (80.0f < ((float)sqrt(accel / potentialSpeed)) * 10.0f) { //good strafe = green
		color1[0] = 0.0f;
		color1[1] = 1.0f;
		color1[2] = 0.0f;
	}
	else if (-5000.0f > accel) { //decelerating = red
		color1[0] = 1.0f;
		color1[1] = 0.0f;
		color1[2] = 0.0f;
	}

	lineWidth = cg_strafeHelperLineWidth.value; //offset the triangles by half of the line width
	if (lineWidth < 0.25f) {
		lineWidth = 0.25f;
	}
	else if (lineWidth > 5.0f) {
		lineWidth = 5.0f;
	}

	sensitivity = cg_strafeHelperPrecision.value; //set the sensitivity/precision

	if (CG_GetStrafeTriangleStrafeX(velocity, diff, sensitivity) != NULL && CG_GetStrafeTriangleStrafeX(velocity, 90 - diff, sensitivity) != NULL) { //A
		lx = *CG_GetStrafeTriangleStrafeX(velocity, diff, sensitivity);
		lx2 = *CG_GetStrafeTriangleStrafeX(velocity, 90 - diff, sensitivity);
		lWidth = (lx - lx2) / 2.0f;
	}

	if (CG_GetStrafeTriangleStrafeX(velocity, -diff, sensitivity) != NULL && CG_GetStrafeTriangleStrafeX(velocity, diff - 90, sensitivity) != NULL) { //D
		rx = *CG_GetStrafeTriangleStrafeX(velocity, -diff, sensitivity);
		rx2 = *CG_GetStrafeTriangleStrafeX(velocity, diff - 90, sensitivity);
		rWidth = (rx2 - rx) / 2.0f;
	}

	if (lWidth && lWidth > 0 && rWidth > lWidth) { //use smaller width for both to avoid perspective stretching
		width = lWidth;
	}
	else if (rWidth && rWidth > 0 && lWidth > rWidth) {
		width = rWidth;
	}

	trap_R_SetColor(color1); //set the color

	if (CG_GetStrafeTriangleStrafeX(velocity, diff, sensitivity) != NULL) { //draw the triangles if they exist
		CG_DrawPic(*CG_GetStrafeTriangleStrafeX(velocity, diff, sensitivity) - width - (0.5f * lineWidth), (0.5f * cgs.screenHeight) - 4.0f, width, 8.0f, cgs.media.leftTriangle);
	}
	if (CG_GetStrafeTriangleStrafeX(velocity, -diff, sensitivity) != NULL) {
		CG_DrawPic(*CG_GetStrafeTriangleStrafeX(velocity, -diff, sensitivity) + (0.5f * lineWidth), (0.5f * cgs.screenHeight) - 4.0f, width, 8.0f, cgs.media.rightTriangle);
	}
}

static float CG_CmdScale( usercmd_t *cmd ) {
    int		max;
    float	total;
    float	scale;
    int		umove = 0; //cmd->upmove;
    //don't factor upmove into scaling speed

    if(pm->pmove_upCmdScale || pm->pmove_movement == MOVEMENT_SP){ //upmove velocity scaling
        umove = cmd->upmove;
    }
    max = abs( cmd->forwardmove );
    if ( abs( cmd->rightmove ) > max ) {
        max = abs( cmd->rightmove );
    }
    if ( abs( umove ) > max ) {
        max = abs( umove );
    }
    if ( !max ) {
        return 0;
    }

    total = sqrt( cmd->forwardmove * cmd->forwardmove
                  + cmd->rightmove * cmd->rightmove + umove * umove );
    scale = (float)pm->ps->speed * max / ( 127.0 * total );

    return scale;
}

static void CG_GetWishspeed(){
    int         i;
    vec3_t		wishvel;
    float		fmove, smove;
    vec3_t		forward, right, up;
    float		wishspeed;
    float		scale;
    usercmd_t	cmd;

    fmove = pm->cmd.forwardmove;
    smove = pm->cmd.rightmove;

    cmd = pm->cmd;
    scale = CG_CmdScale( &cmd );

    AngleVectors(cg.refdefViewAngles, forward, right, up);
    // project moves down to flat plane
    forward[2] = 0;
    right[2] = 0;
    VectorNormalize (forward);
    VectorNormalize (right);

    for ( i = 0 ; i < 2 ; i++ )
    {
        wishvel[i] = forward[i]*fmove + right[i]*smove;
    }
    wishvel[2] = 0;
    VectorCopy (wishvel, cg.wishdir);
    wishspeed = VectorNormalize(cg.wishdir);
    if(pm->pmove_movement != MOVEMENT_SP) {
        wishspeed *= scale;
    }else if(pm->pmove_movement == MOVEMENT_SP) { //SP - Encourage deceleration away from the current velocity when in the air
        if(!((qboolean)(cg.snap->ps.groundEntityNum == ENTITYNUM_WORLD)) && (DotProduct (pm->ps->velocity, cg.wishdir) < 0.0f)) {
            wishspeed *= 1.35;
        }
        if((qboolean)(cg.snap->ps.groundEntityNum == ENTITYNUM_WORLD)) //SP no cmdscale in air - should be applying scale on ground, making lines bounce on landings?
        {
            wishspeed *= scale;
        }
    }

    cg.wishspeed = wishspeed;
}

/*
===================
CG_StrafeHelper
japro - Main strafehelper function
===================
*/
static void CG_StrafeHelper(centity_t* cent) {
	vec_t* velocity = cg.predictedPlayerState.velocity;
	static vec3_t velocityAngle;
	const float currentSpeed = cg.currentSpeed;
	float pmAccel = 10.0f, pmAirAccel = 1.0f, pmFriction = 6.0f, frametime, optimalDeltaAngle, baseSpeed = cg.predictedPlayerState.speed;
	const int moveStyle = cgs.movement;
	int moveDir = 0;
	qboolean onGround;
	usercmd_t cmd = { 0 };

	if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) {
		trap_GetUserCmd(trap_GetCurrentCmdNumber(), &cmd);
	}

	else if (cg.snap) {
		moveDir = cg.snap->ps.movementDir;
		switch (moveDir) {
		case 0: // W
			cmd.forwardmove = 1; break;
		case 1: // WA
			cmd.forwardmove = 1; cmd.rightmove = -1; break;
		case 2: // A
			cmd.rightmove = -1;	break;
		case 3: // AS
			cmd.rightmove = -1;	cmd.forwardmove = -1; break;
		case 4: // S
			cmd.forwardmove = -1; break;
		case 5: // SD
			cmd.forwardmove = -1; cmd.rightmove = 1; break;
		case 6: // D
			cmd.rightmove = 1; break;
		case 7: // DW
			cmd.rightmove = 1; cmd.forwardmove = 1;	break;
		default:
			break;
		}
		if (cg.snap->ps.pm_flags & PMF_JUMP_HELD)
			cmd.upmove = 1;
	}
	else {
		return; //No cg.snap causes this to return.
	}

	onGround = (qboolean)(cg.snap->ps.groundEntityNum == ENTITYNUM_WORLD);

	if (moveStyle == MOVEMENT_WSW) {
		pmAccel = 12.0f;
		pmFriction = 8.0f;
	}
	else if (moveStyle == MOVEMENT_CPMA) {
		pmAccel = 15.0f;
		pmFriction = 8.0f;
	}
	else if (moveStyle == MOVEMENT_SLICK) {
		pmFriction = 0.0f;//unless walking
		pmAccel = 30.0f;
	}
    else if(moveStyle == MOVEMENT_SP) {
        pmAccel = 12.0f;
        pmAirAccel = 4.0f;
    }
    if(pm->pmove_upCmdScale || moveStyle == MOVEMENT_SP){
        CG_GetWishspeed();
        baseSpeed = cg.wishspeed;
    }

	if (currentSpeed < (baseSpeed - 1))
		return;

	if (cg_strafeHelper_FPS.value < 1)
		frametime = ((float)cg.frametime * 0.001f);
	else if (cg_strafeHelper_FPS.value > 1000)
		frametime = 1;
	else frametime = 1 / cg_strafeHelper_FPS.value;

	if (onGround)
		optimalDeltaAngle = acos((double)((baseSpeed - (pmAccel * baseSpeed * frametime)) / (currentSpeed * (1 - pmFriction * (frametime))))) * (180.0f / M_PI) - 45.0f;
	else
		optimalDeltaAngle = acos((double)((baseSpeed - (pmAirAccel * baseSpeed * frametime)) / currentSpeed)) * (180.0f / M_PI) - 45.0f;

	if (optimalDeltaAngle < 0 || optimalDeltaAngle > 360)
		optimalDeltaAngle = 0;

	velocity[2] = 0;
	vectoangles(velocity, velocityAngle); //We have the offset from our Velocity angle that we should be aiming at, so now we need to get our velocity angle.

	if ((moveStyle == MOVEMENT_CPMA || moveStyle == MOVEMENT_WSW || moveStyle == MOVEMENT_PJK || moveStyle == MOVEMENT_SLICK) && cg_strafeHelper.integer & SHELPER_ACCELZONES) { //Accel Zones (air control styles)
		CG_DrawStrafeTriangles(velocityAngle, optimalDeltaAngle, baseSpeed, moveDir);
	}

	if (moveStyle == MOVEMENT_CPMA || moveStyle == MOVEMENT_PJK || moveStyle == MOVEMENT_WSW || moveStyle == MOVEMENT_QW || (moveStyle == MOVEMENT_SLICK && !onGround)) {//center line (air control styles)
		if (cg_strafeHelper.integer & SHELPER_CENTER)
			CG_DrawStrafeLine(velocityAngle, 0, (qboolean)(cmd.forwardmove == 0 && cmd.rightmove != 0), 8); //Center
		if (cg_strafeHelper.integer & SHELPER_REAR && cg_strafeHelper.integer & SHELPER_S && cg_strafeHelper.integer & SHELPER_CENTER)
			CG_DrawStrafeLine(velocityAngle, 180.0f, (qboolean)(cmd.forwardmove == 0 && cmd.rightmove == 0), 8); //Rear Center
	}

	if (moveStyle != MOVEMENT_QW) { //Every style but QW has WA/WD lines
        if((qboolean)(cmd.forwardmove > 0 && cmd.rightmove < 0)){ //SP - draw the active line on top so it is the correct colour
            if (cg_strafeHelper.integer & SHELPER_WD) //WD forwards
                CG_DrawStrafeLine(velocityAngle, -(optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f)),
                                  (qboolean) (cmd.forwardmove > 0 && cmd.rightmove > 0), 7); //WD
            if (cg_strafeHelper.integer & SHELPER_WA) //WA forwards
                CG_DrawStrafeLine(velocityAngle, (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f)),
                                  (qboolean) (cmd.forwardmove > 0 && cmd.rightmove < 0), 1); //WA
        } else if((cmd.forwardmove > 0 && cmd.rightmove > 0)) { //SP - draw the active line on top so it is the correct colour
            if (cg_strafeHelper.integer & SHELPER_WA) //WA forwards
                CG_DrawStrafeLine(velocityAngle, (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f)),
                                  (qboolean) (cmd.forwardmove > 0 && cmd.rightmove < 0), 1); //WA
            if (cg_strafeHelper.integer & SHELPER_WD) //WD forwards
                CG_DrawStrafeLine(velocityAngle, -(optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f)),
                                  (qboolean) (cmd.forwardmove > 0 && cmd.rightmove > 0), 7); //WD
        }
		if (cg_strafeHelper.integer & SHELPER_REAR) { //SA/SD backwards
			if (cg_strafeHelper.integer & SHELPER_SA)
				CG_DrawStrafeLine(velocityAngle, (180.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove < 0), 3); //SA
			if (cg_strafeHelper.integer & SHELPER_SD)
				CG_DrawStrafeLine(velocityAngle, (180.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove > 0), 5); //SD
		}
		if (cg_strafeHelper.integer & SHELPER_INVERT) //inverted (wa/wd backwards, sa/sd forwards)
		{
			if (cg_strafeHelper.integer & SHELPER_WA)
				CG_DrawStrafeLine(velocityAngle, (-90.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove < 0), 9); //WA backwards
			if (cg_strafeHelper.integer & SHELPER_WD)
				CG_DrawStrafeLine(velocityAngle, (90.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove > 0), 10); //WD backwards
			if (cg_strafeHelper.integer & SHELPER_SA)
				CG_DrawStrafeLine(velocityAngle, (-90.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove < 0), 9); //SA forwards
			if (cg_strafeHelper.integer & SHELPER_SD)
				CG_DrawStrafeLine(velocityAngle, (90.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove > 0), 10); //SD forwards
		}
	}
	if (moveStyle == MOVEMENT_JK2 || moveStyle == MOVEMENT_VQ3 || moveStyle == MOVEMENT_SLIDE || moveStyle == MOVEMENT_SPEED || moveStyle == MOVEMENT_SP || (moveStyle == MOVEMENT_SLICK && onGround)) { //A/D
		if (cg_strafeHelper.integer & SHELPER_A)
			CG_DrawStrafeLine(velocityAngle, (-45.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove < 0), 2); //A
		if (cg_strafeHelper.integer & SHELPER_D)
			CG_DrawStrafeLine(velocityAngle, (45.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove > 0), 6); //D
		if (cg_strafeHelper.integer & SHELPER_REAR) { //A/D backwards
			CG_DrawStrafeLine(velocityAngle, (225.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove < 0), 9); //A
			CG_DrawStrafeLine(velocityAngle, (135.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove > 0), 10); //D
		}
	}
	if (moveStyle == MOVEMENT_JK2 || moveStyle == MOVEMENT_VQ3 || moveStyle == MOVEMENT_SLIDE || moveStyle == MOVEMENT_SPEED || moveStyle == MOVEMENT_SP) {
		if (cg_strafeHelper.integer & SHELPER_W) { //W/S only
			CG_DrawStrafeLine(velocityAngle, (45.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove == 0), 0); //W
			CG_DrawStrafeLine(velocityAngle, (-45.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove == 0), 0); //W
		}
		if (cg_strafeHelper.integer & SHELPER_S && cg_strafeHelper.integer & SHELPER_REAR) {
			CG_DrawStrafeLine(velocityAngle, (225.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove == 0), 4); //S
			CG_DrawStrafeLine(velocityAngle, (-225.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove == 0), 4); //S
		}
	}
}

/*
===================
CG_GetGroundDistance
japro - Ground Distance function for use in jump detection for movement keys
===================
*/
static float CG_GetGroundDistance(void) {
	trace_t tr;
	vec3_t down;

	VectorCopy(cg.predictedPlayerState.origin, down);
	down[2] -= 4096;
	CG_Trace(&tr, cg.predictedPlayerState.origin, NULL, NULL, down, cg.predictedPlayerState.clientNum, MASK_SOLID);
	VectorSubtract(cg.predictedPlayerState.origin, tr.endpos, down);

	return VectorLength(down) - 24.0f;
}

/*
===================
CG_DrawMovementKeys
japro - Draw the movement keys
===================
*/
static void CG_DrawMovementKeys(centity_t* cent) {
	usercmd_t cmd = { 0 };
	playerState_t* ps = NULL;
	int moveDir;
	float w, h, x, y, xOffset, yOffset;

	if (!cg.snap)
		return;

	ps = &cg.predictedPlayerState;
	moveDir = ps->movementDir;

	if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) {
		trap_GetUserCmd(trap_GetCurrentCmdNumber(), &cmd);
	}
	else
	{
		float xyspeed = (float)sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);
		float zspeed = ps->velocity[2];
		static float lastZSpeed = 0.0f;

		if ((CG_GetGroundDistance() > 1 && zspeed > 8 && zspeed > lastZSpeed && !cg.snap->ps.fd.forceGripCripple) || (cg.snap->ps.pm_flags & PMF_JUMP_HELD))
			cmd.upmove = 1;
		else if ((ps->pm_flags & PMF_DUCKED) || CG_InRollAnim(cent))
			cmd.upmove = -1;
		if (xyspeed < 9)
			moveDir = -1;
		lastZSpeed = zspeed;

		if ((cent->currentState.eFlags & EF_FIRING) && !(cent->currentState.eFlags & EF_ALT_FIRING)) {
			cmd.buttons |= BUTTON_ATTACK;
			cmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else if (cent->currentState.eFlags & EF_ALT_FIRING) {
			cmd.buttons |= BUTTON_ALT_ATTACK;
			cmd.buttons &= ~BUTTON_ATTACK;
		}

		switch (moveDir) {
		case 0: // W
			cmd.forwardmove = 1;
			break;
		case 1: // WA
			cmd.forwardmove = 1;
			cmd.rightmove = -1;
			break;
		case 2: // A
			cmd.rightmove = -1;
			break;
		case 3: // AS
			cmd.rightmove = -1;
			cmd.forwardmove = -1;
			break;
		case 4: // S
			cmd.forwardmove = -1;
			break;
		case 5: // SD
			cmd.forwardmove = -1;
			cmd.rightmove = 1;
			break;
		case 6: // D
			cmd.rightmove = 1;
			break;
		case 7: // DW
			cmd.rightmove = 1;
			cmd.forwardmove = 1;
			break;
		default:
			break;
		}
	}

	if (cg_movementKeys.integer == 2) {
		w = (8 * cg_movementKeysSize.value);
		h = 8 * cg_movementKeysSize.value;
		x = 0.5f * cgs.screenWidth - w * 1.5f;
		y = 0.5f * cgs.screenHeight - h * 1.5f;
	}
	else {
		w = (16 * cg_movementKeysSize.value);
		h = 16 * cg_movementKeysSize.value;
		x = cgs.screenWidth - (cgs.screenWidth - cg_movementKeysX.integer);
		y = cg_movementKeysY.integer;
	}

	xOffset = yOffset = 0;
	if (cgs.newHud && cg_movementKeys.integer != 2) {
		switch (cg_hudFiles.integer) {
		case 0: xOffset += 26; /*492*/ yOffset -= 3;	break; //jk2hud
		case 1: xOffset += 51; /*516*/					break; //simplehud
		default:										break;
		}

		if (cgs.newHud) { //offset the keys if using newhud
			yOffset += 12; //445
		}

		x += xOffset;
		y += yOffset;
	}

	if (cg_movementKeys.integer > 1) { //new movement keys style
		if (cmd.upmove < 0)
			CG_DrawPic(w * 2 + x, y, w, h, cgs.media.keyCrouchOnShader2);
		if (cmd.upmove > 0)
			CG_DrawPic(x, y, w, h, cgs.media.keyJumpOnShader2);
		if (cmd.forwardmove < 0)
			CG_DrawPic(w + x, h * 2 + y, w, h, cgs.media.keyBackOnShader2);
		if (cmd.forwardmove > 0)
			CG_DrawPic(w + x, y, w, h, cgs.media.keyForwardOnShader2);
		if (cmd.rightmove < 0)
			CG_DrawPic(x, h + y, w, h, cgs.media.keyLeftOnShader2);
		if (cmd.rightmove > 0)
			CG_DrawPic(w * 2 + x, h + y, w, h, cgs.media.keyRightOnShader2);
		if (cmd.buttons & BUTTON_ATTACK)
			CG_DrawPic(x, 2 * h + y, w, h, cgs.media.keyAttackOn2);
		if (cmd.buttons & BUTTON_ALT_ATTACK)
			CG_DrawPic(w * 2 + x, 2 * h + y, w, h, cgs.media.keyAltOn2);
	} else if (cg_movementKeys.integer == 1) { //original movement keys style
		if (cmd.upmove < 0)
			CG_DrawPic(w * 2 + x, y, w, h, cgs.media.keyCrouchOnShader);
		else
			CG_DrawPic(w * 2 + x, y, w, h, cgs.media.keyCrouchOffShader);
		if (cmd.upmove > 0)
			CG_DrawPic(x, y, w, h, cgs.media.keyJumpOnShader);
		else
			CG_DrawPic(x, y, w, h, cgs.media.keyJumpOffShader);
		if (cmd.forwardmove < 0)
			CG_DrawPic(w + x, h + y, w, h, cgs.media.keyBackOnShader);
		else
			CG_DrawPic(w + x, h + y, w, h, cgs.media.keyBackOffShader);
		if (cmd.forwardmove > 0)
			CG_DrawPic(w + x, y, w, h, cgs.media.keyForwardOnShader);
		else
			CG_DrawPic(w + x, y, w, h, cgs.media.keyForwardOffShader);
		if (cmd.rightmove < 0)
			CG_DrawPic(x, h + y, w, h, cgs.media.keyLeftOnShader);
		else
			CG_DrawPic(x, h + y, w, h, cgs.media.keyLeftOffShader);
		if (cmd.rightmove > 0)
			CG_DrawPic(w * 2 + x, h + y, w, h, cgs.media.keyRightOnShader);
		else
			CG_DrawPic(w * 2 + x, h + y, w, h, cgs.media.keyRightOffShader);
		if (cmd.buttons & BUTTON_ATTACK)
			CG_DrawPic(w * 3 + x, y, w, h, cgs.media.keyAttackOn);
		else
			CG_DrawPic(w * 3 + x, y, w, h, cgs.media.keyAttackOff);
		if (cmd.buttons & BUTTON_ALT_ATTACK)
			CG_DrawPic(w * 3 + x, h + y, w, h, cgs.media.keyAltOn);
		else
			CG_DrawPic(w * 3 + x, h + y, w, h, cgs.media.keyAltOff);
	}
}
