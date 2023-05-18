//cg_multiversion.h
#ifndef __CG_STRAFEHELPER_H_INCLUDED___
#define __CG_STRAFEHELPER_H_INCLUDED___

/* Defines */
//Strafe Helper Options
#include "../ui/ui_shared.h"
#include "../game/bg_local.h"

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

/* Variables */

/* Functions */
void CG_DrawStrafeHUD(centity_t	*cent);

/* Speedometer Functions */
static void CG_CalculateSpeed(centity_t* cent);
static void CG_GraphAddSpeed(void);
static void CG_DrawSpeedGraph(rectDef_t* rect, const vec4_t foreColor, vec4_t backColor);
static void CG_DrawJumpHeight(centity_t* cent);
static void CG_DrawJumpDistance(void);
static void CG_DrawVerticalSpeed(void);
static void CG_DrawAccelMeter(void);
static void CG_DrawSpeedometer(void);

/* Strafe Helper Functions */
static void	CG_DrawLine(float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff);
static void CG_DrawStrafehelperWeze(int moveDir);
static void CG_StrafeHelperSound(float difference);
static void CG_DrawStrafeLine(vec3_t velocity, float diff, qboolean active, int moveDir);
static int	CG_GetStrafeTriangleAccel(void);
static void CG_DrawStrafeTriangles(vec3_t velocity, float diff, float wishspeed, int moveDir);
qboolean	CG_InRollAnim(centity_t* cent);
static void CG_StrafeHelper(centity_t* cent);

//Movement Keys Functions
static float CG_GetGroundDistance(void);
static void CG_DrawMovementKeys(centity_t* cent);

/* Function Pointers */
static float* CG_GetStrafeTriangleStrafeX(vec3_t velocity, float diff, float sensitivity);

#endif //__CG_STRAFEHELPER_H_INCLUDED___
