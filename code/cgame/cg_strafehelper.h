//cg_multiversion.h
#ifndef __CG_STRAFEHELPER_H_INCLUDED___
#define __CG_STRAFEHELPER_H_INCLUDED___

/* Defines */
//Strafe Helper Options
#include "../ui/ui_shared.h"
#include "../game/bg_local.h"

//Movement direction keys - Odd is 2 keys
#define KEY_W       0
#define KEY_WA      1
#define KEY_A       2
#define KEY_AS      3
#define KEY_S       4
#define KEY_SD      5
#define KEY_D       6
#define KEY_DW      7
#define KEY_CENTER  8
#define KEY_REAR    9
#define KEY_INVERT  10

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

//Speedometer Options
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

//Miscellaneous
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

/* Structs */
typedef struct {
    float x;    // horiz position
    float y;    // vert position
    float w;    // width
    float h;    // height;
} rectDef_c;

//Strafehelper states

typedef struct {
    float           stopspeed;
    float           duckscale;
    float           swimscale;
    float           wadescale;
    float           accelerate;
    float           airaccelerate;
    float           wateraccelerate;
    float           flyaccelerate;
    float           friction;
    float           waterfriction;
    float           flightfriction;
    float           airdecelrate;
    float           airstopaccelerate;
    float           airstrafewishspeed;
    float           airstrafeaccelerate;
    float           aircontrol;
    qboolean        hasAirControl;
    qboolean        hasForceJumps;
    qboolean        hasAutoJump;
}dfstyle; //movement pysics constants

typedef struct {
    float           frametime;
    float           wishspeed;
    float           currentspeed;
    vec3_t          velocityAngles;
    vec3_t          wishDir;
    float           currentSpeed;
    qboolean        wasOnGround;
}dfcgaz; //values used for cgaz and its caluclations

typedef struct {
    vec4_t color;
    qboolean active;
    float angleOpt;
    float rearOpt;
    qboolean onScreen;
    qboolean rearOnScreen;
    float x;
    float rearX;
    float y;
    vec3_t angs;
    vec3_t rearAngs;
    qboolean rear;
}dfsline;

typedef struct {
    float lineWidth;
    int sensitivity;
    int LINE_HEIGHT;
    vec4_t twoKeyColor;
    vec4_t oneKeyColor;
    vec4_t oneKeyColorAlt;
    vec4_t rearColor;
    vec4_t activeColor;
    qboolean rear;
    qboolean center;
    float offset;
}dfshelp;

typedef struct {
    qboolean        onGround;
    vec_t*          velocity;
    usercmd_t       cmd;
    int             moveStyle;
    dfstyle         physics;
    int             moveDir;
    vec3_t          viewAngles;
    vec3_t          viewOrg;
    dfshelp         strafeHelper;
    dfcgaz          cgaz;
} dfstate;


/* Functions */
static  void DF_SetStrafeHelper();
static  dfsline DF_GetLine(int moveDir);
static void DF_SetAngleToX(dfsline *inLine);
static void DF_SetLineColour(dfsline* inLine, int moveDir);
//Pmove functions
static  float DF_GetAccelerate();
static  float DF_GetAirAccelerate();
static  float DF_GetAirStrafeAccelerate();
static  float DF_GetAirStopAccelerate();
static  float DF_GetAirStrafeWishspeed();
static  float DF_GetFriction();
static  float DF_GetDuckScale();
static  qboolean DF_HasAirControl();
static  qboolean DF_HasForceJumps();
static  qboolean DF_HasAutoJump();

/* Strafehelper Init */
  void DF_DrawStrafeHUD(centity_t	*cent);
static  void DF_StrafeHelper();
static  void DF_SetPlayerState();
static  void DF_SetClientReal(); //rs Real state (not a spectator/demo)
static  void DF_SetClient(); //sn Snapshot state (not a spectator/demo)
static  void DF_SetPhysics();

/* Cgaz functions */
//static float CGAZ_Min(dfstate *cs);
static float CGAZ_Opt(qboolean onGround, float accelerate, float currentSpeed, float wishSpeed,
                      float frametime, float friction, float airaccelerate);
//static float CGAZ_Max(dfstate *cs)
static void DF_SetVelocityAngles();
static void DF_SetFrameTime();
static void DF_SetCurrentSpeed();
static float DF_GetCmdScale(usercmd_t cmd);
static float DF_GetWishspeed(usercmd_t inCmd);
static void DF_SetCGAZ();

/* Strafe Helper Functions */
static void	DF_DrawLine(float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff);
static void DF_DrawStrafehelperWeze(int moveDir, dfsline inLine);
static void DF_StrafeHelperSound(float difference);
static void DF_DrawStrafeLine(dfsline line);
static int	DF_GetStrafeTriangleAccel(void);
static void DF_DrawStrafeTriangles(vec3_t velocity, float diff, float wishspeed, int moveDir);
static float* DF_GetStrafeTrianglesX(vec3_t velocity, float diff, float sensitivity);

/* Speedometer Functions */
static void DF_GraphAddSpeed(void);
static void DF_DrawSpeedGraph(rectDef_c* rect, const vec4_t foreColor, vec4_t backColor);
static void DF_DrawJumpHeight(centity_t* cent);
static void DF_DrawJumpDistance(void);
static void DF_DrawVerticalSpeed(void);
static void DF_DrawAccelMeter(void);
static void DF_DrawSpeedometer(void);

/* Misc Functions */
extern qboolean	CG_InRollAnim(centity_t* cent);
extern void CG_DrawStringExt( int x, int y, const char *string, const float *setColor,
                       qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );//Movement Keys Functions
static float DF_GetGroundDistance(void);
static void DF_DrawMovementKeys(centity_t* cent);

/* Function Pointers */

#endif //__CG_STRAFEHELPER_H_INCLUDED___
