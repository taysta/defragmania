#include "cg_local.h"
#include "cg_strafehelper.h"

static float speedometerXPos, jumpsXPos;
static float firstSpeed;
static float speedSamples[SPEEDOMETER_NUM_SAMPLES];
static int oldestSpeedSample = 0;
static int maxSpeedSample = 0;
static dfstate state;

//#todo make these work from bg_pmove so not repeated
/* Physics Constants Getters */
//get the styles accelerate - acceleration on ground constant
static float DF_GetAccelerate() {
    float accelerate;
    switch (state.moveStyle) {
        case MOVEMENT_PJK:
        case MOVEMENT_CPMA:
            accelerate = pm_cpm_accelerate;
            break;
        case MOVEMENT_WSW:
            accelerate = pm_wsw_accelerate;
            break;
        case MOVEMENT_SLICK:
            accelerate = pm_slick_accelerate;
            break;
        case MOVEMENT_SP:
            accelerate = pm_sp_accelerate;
            break;
        default:
            accelerate = pm_accelerate;
    }
    return accelerate;
}
//get the styles airaccelerate - acceleration in air constant
static float DF_GetAirAccelerate() {
    float airAccelerate;
    switch (state.moveStyle) {
        case MOVEMENT_PJK:
        case MOVEMENT_CPMA:
        case MOVEMENT_WSW:
        case MOVEMENT_SLICK:
            airAccelerate = pm_cpm_airaccelerate;
            break;
        case MOVEMENT_SP:
            airAccelerate = pm_sp_airaccelerate;
            break;
        default:
            airAccelerate = pm_airaccelerate;
    }
    return airAccelerate;
}
//get the styles airstrafeaccelerate - acceleration in air with air control constant
static float DF_GetAirStrafeAccelerate() {
    float airStrafeAccelerate;
    switch (state.moveStyle) {
        case MOVEMENT_PJK:
        case MOVEMENT_CPMA:
        case MOVEMENT_WSW:
            airStrafeAccelerate = pm_cpm_airstrafeaccelerate;
            break;
        case MOVEMENT_SLICK:
            airStrafeAccelerate = pm_slick_airstrafeaccelerate;
            break;
        case MOVEMENT_SP:
            airStrafeAccelerate = pm_sp_airaccelerate;
            break;
        default:
            airStrafeAccelerate = pm_airaccelerate;
    }
    return airStrafeAccelerate;
}
//get the styles airstopaccelerate - constant of air accel stop rate with air control
static float DF_GetAirStopAccelerate() {
    float airStopAccelerate;
    switch (state.moveStyle) {
        case MOVEMENT_PJK:
        case MOVEMENT_CPMA:
        case MOVEMENT_WSW:
        case MOVEMENT_SLICK:
            airStopAccelerate = pm_cpm_airstopaccelerate;
            break;
        default:
            airStopAccelerate =  0.0f;
    }
    return airStopAccelerate;
}
//get the styles airstrafewishspeed - constant of wishspeed while in air with air control
static float DF_GetAirStrafeWishspeed() {
    float airStrafeWishSpeed;
    switch (state.moveStyle) {
        case MOVEMENT_PJK:
        case MOVEMENT_CPMA:
        case MOVEMENT_WSW:
        case MOVEMENT_SLICK:
            airStrafeWishSpeed = pm_cpm_airstrafewishspeed;
            break;
        default:
            airStrafeWishSpeed = 0;
    }
    return airStrafeWishSpeed;
}
//get the styles friction - constant of friction
static float DF_GetFriction() {
    float friction;
    switch (state.moveStyle) {
        case MOVEMENT_CPMA:
        case MOVEMENT_WSW:
        case MOVEMENT_SLICK:
            friction = pm_cpm_friction;
            break;
        default:
            friction = pm_friction;
    }
    return friction;
}
//get the styles duckscale - how much speed is scaled when ducking
static float DF_GetDuckScale() {
    float duckScale;
    switch (state.moveStyle) {
        case MOVEMENT_VQ3:
            duckScale = pm_vq3_duckScale;
            break;
        case MOVEMENT_WSW:
            duckScale = pm_wsw_duckScale;
            break;
        default:
            duckScale = pm_duckScale;
    }
    return duckScale;
}

/* Physics Constants Checkers */
//does the style have air control - using A/D turning to turn and accelerate fast in air
static qboolean DF_HasAirControl() {
    qboolean hasAirControl;
    switch (state.moveStyle) {
        case MOVEMENT_PJK:
        case MOVEMENT_CPMA:
        case MOVEMENT_WSW:
        case MOVEMENT_SLICK:
            hasAirControl = qtrue;
            break;
        default:
            hasAirControl = qfalse;
    }
    return hasAirControl;
}
//does the style have forcejumps - holding jump to keep jumping higher (base mechanic)
static qboolean DF_HasForceJumps() {
    qboolean hasForceJumps;
    switch (state.moveStyle) {
        case MOVEMENT_JK2:
        case MOVEMENT_PJK:
        case MOVEMENT_QW:
        case MOVEMENT_SPEED:
        case MOVEMENT_SP:
            hasForceJumps = qtrue;
            break;
        default:
            hasForceJumps = qfalse;
    }
    return hasForceJumps;
}
//does the style have autojumps - hold to keep jumping on ground
static qboolean DF_HasAutoJump() {
    qboolean hasAutoJump;
    switch (state.moveStyle) {
        case MOVEMENT_JK2:
        case MOVEMENT_SPEED:
        case MOVEMENT_SP:
            hasAutoJump = qfalse;
            break;
        default:
            hasAutoJump = qtrue;
    }
    return hasAutoJump;
}

/* Draw HUD */
void DF_DrawStrafeHUD(centity_t	*cent)
{
    if (cg_strafeHelper.integer) {
        DF_StrafeHelper();
    }

    //japro movement keys
    if (cg_movementKeys.integer) {
        DF_DrawMovementKeys(cent);
    }

    if ((cg_speedometer.integer & SPEEDOMETER_ENABLE)) {
        speedometerXPos = cg_speedometerX.value;
        jumpsXPos = cg_speedometerJumpsX.value;
        DF_DrawSpeedometer();

        if ((cg_speedometer.integer & SPEEDOMETER_ACCELMETER))
            DF_DrawAccelMeter();
        if (cg_speedometer.integer & SPEEDOMETER_JUMPHEIGHT)
            DF_DrawJumpHeight(cent);
        if (cg_speedometer.integer & SPEEDOMETER_JUMPDISTANCE)
            DF_DrawJumpDistance();
        if (cg_speedometer.integer & SPEEDOMETER_VERTICALSPEED)
            DF_DrawVerticalSpeed();

        if ((cg_speedometer.integer & SPEEDOMETER_SPEEDGRAPH)) {
            rectDef_c speedgraphRect;
            vec4_t foreColor = { 0.0f,0.8f,1.0f,0.8f };
            vec4_t backColor = { 0.0f,0.8f,1.0f,0.0f };
            speedgraphRect.x = (cgs.screenWidth * 0.5f - (150.0f / 2.0f));
            speedgraphRect.y = cgs.screenHeight - 22 - 2;
            speedgraphRect.w = 150.0f;
            speedgraphRect.h = 22.0f;
            DF_GraphAddSpeed();
            DF_DrawSpeedGraph(&speedgraphRect, foreColor, backColor);
        }
    }

    //jaPRO strafehelper line crosshair

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

        DF_DrawLine((0.5f * cgs.screenWidth) - (0.5f * lineWidth), (0.5f * cgs.screenHeight) - 5.0f, (0.5f * cgs.screenWidth) - (0.5f * lineWidth), (0.5f * cgs.screenHeight) + 5.0f, lineWidth, hcolor, hcolor[3], 0); //640x480, 320x240
    }
}

/* Strafehelper */
//main strafehelper function, sets states and then calls drawstrafeline function for each keypress
static void DF_StrafeHelper() {
    dfsline line;
    int i;
    DF_SetPlayerState();
    DF_SetStrafeHelper();

    for(i = 0; i <= KEY_CENTER; i++){
        line = DF_GetLine(i);
        if(line.onScreen){
            DF_DrawStrafeLine(line);
        }
        if(line.rearOnScreen){
            line.onScreen = line.rearOnScreen;
            line.x = line.rearX;
            VectorCopy(line.rearAngs, line.angs);
            DF_DrawStrafeLine(line);
        }
    }
    if(state.onGround){
        state.cgaz.wasOnGround = qtrue;
    } else {
        state.cgaz.wasOnGround = qfalse;
    }
}

/* Strafehelper Setters */
//#todo make sure correct variables were chosen here for server and client, especially vieworg, and implement other predicted events with pmovesingle
//sets the dfstate function used for strafehelper calculations
static void DF_SetPlayerState()
{
    if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) // are we a real client
    {
        DF_SetClientReal();
    }
    else if (cg.snap) //or are we a spectator/demo
    {
        DF_SetClient();
    }
    DF_SetPhysics();
    VectorCopy(cg.refdef.vieworg, state.viewOrg);
    DF_SetCGAZ();
}
//sets parts of the dfstate struct for a non-predicted client (spectator/demo playback)
static void DF_SetClient(){
    state.moveStyle = pm->pmove_movement;
    state.moveDir = cg.snap->ps.movementDir;
    switch (state.moveDir) {
        case KEY_W: // W
            state.cmd.forwardmove = 127; break;
        case KEY_WA: // WA
            state.cmd.forwardmove = 127; state.cmd.rightmove = -127; break;
        case KEY_A: // A
            state.cmd.rightmove = -127;	break;
        case KEY_AS: // AS
            state.cmd.rightmove = -127;	state.cmd.forwardmove = -127; break;
        case KEY_S: // S
            state.cmd.forwardmove = -127; break;
        case KEY_SD: // SD
            state.cmd.forwardmove = -127; state.cmd.rightmove = 127; break;
        case KEY_D: // D
            state.cmd.rightmove = 127; break;
        case KEY_DW: // DW
            state.cmd.rightmove = 127; state.cmd.forwardmove = 127;	break;
        default:
            break;
    }
    if (cg.snap->ps.pm_flags & PMF_JUMP_HELD) {
        state.cmd.upmove = 127;
    }
    state.onGround = (qboolean)(cg.snap->ps.groundEntityNum == ENTITYNUM_WORLD); //on ground this frame
    state.velocity = cg.snap->ps.velocity;
    VectorCopy(cg.refdefViewAngles, state.viewAngles);

}
//sets parts of the dfstate struct for a predicted client
static void DF_SetClientReal(){
    state.moveStyle = cgs.movement;
    state.moveDir = cg.predictedPlayerState.movementDir; //0-7 movement dir
    trap_GetUserCmd(trap_GetCurrentCmdNumber(), &state.cmd);
    state.onGround = (qboolean)(cg.predictedPlayerState.groundEntityNum == ENTITYNUM_WORLD); //on ground this frame
    state.velocity = cg.predictedPlayerState.velocity;
    VectorCopy(cg.refdefViewAngles, state.viewAngles);
}
//sets the constants relative to the current movement styles physics
static void DF_SetPhysics() {
    state.physics.stopspeed = pm_stopspeed;
    state.physics.duckscale = DF_GetDuckScale();
    state.physics.swimscale = pm_swimScale;
    state.physics.wadescale = pm_wadeScale;
    state.physics.accelerate = DF_GetAccelerate();
    state.physics.airaccelerate = DF_GetAirAccelerate();
    state.physics.wateraccelerate = pm_wateraccelerate;
    state.physics.flightfriction = pm_flightfriction;
    state.physics.friction = DF_GetFriction();
    state.physics.airaccelerate = DF_GetAirAccelerate();
    state.physics.airstopaccelerate = DF_GetAirStopAccelerate();
    state.physics.airstrafewishspeed = DF_GetAirStrafeWishspeed();
    state.physics.airstrafeaccelerate = DF_GetAirStrafeAccelerate();
    state.physics.airdecelrate = pm_sp_airDecelRate;
    state.physics.aircontrol = pm_cpm_aircontrol;
    state.physics.hasAirControl = DF_HasAirControl();
    state.physics.hasForceJumps = DF_HasForceJumps();
    state.physics.hasAutoJump = DF_HasAutoJump();
}
//calls functions that sets values to the cgaz struct
static void DF_SetCGAZ(){
    DF_SetFrameTime();
    DF_SetCurrentSpeed();
    DF_SetVelocityAngles();
    state.cgaz.wishspeed = DF_GetWishspeed(state.cmd);
}

/* CGAZ Setters */
//sets the frametime for the cgaz struct
static void DF_SetFrameTime(){
    float frameTime;
    //get the frametime
    if (cg_strafeHelper_FPS.value < 1) {
        frameTime = ((float) cg.frametime * 0.001f);
    }
    else if (cg_strafeHelper_FPS.value > 1000) {
        frameTime = 1;
    } else {
        frameTime = 1 / cg_strafeHelper_FPS.value;
    }
    //set the frametime
    state.cgaz.frametime = frameTime;
}
//sets the current speed for the cgaz struct
static void DF_SetCurrentSpeed() {
    //const vec_t* const velocity = (cent->currentState.clientNum == cg.clientNum ? cg.predictedPlayerState.velocity : cent->currentState.pos.trDelta);
    float speed;
    //get the current speed
    speed = (float)sqrt(state.velocity[0] * state.velocity[0] + state.velocity[1] * state.velocity[1]);
    //set the current speed
    state.cgaz.currentSpeed = speed;
}
//sets the velocity angle for the cgaz struct
static void DF_SetVelocityAngles()
{
    vec_t* xyVel = { 0 };
    static vec3_t velAngles = { 0 };
    //set the velocity
    VectorCopy(state.velocity, xyVel);
    xyVel[2] = 0;
    //get the velocity angle
    vectoangles(xyVel, velAngles);
    //set the velocity angle
    VectorCopy(velAngles, state.cgaz.velocityAngles);
}

/* Strafehelper/Line Setters/Getters */
//set the strafehelper user settings to the struct
static void DF_SetStrafeHelper(){
    float lineWidth;
    int sensitivity = cg_strafeHelperPrecision.integer;
    const int LINE_HEIGHT = (cgs.screenHeight * 0.5f - 10.0f); //240 is midpoint, so it should be a little higher so crosshair is always on it.
    const vec4_t twoKeyColor = { 1, 1, 1, 0.75f };
    const vec4_t oneKeyColor = { 0.5f, 1, 1, 0.75f };
    const vec4_t oneKeyColorAlt = { 1, 0.75f, 0.0f, 0.75f };
    const vec4_t rearColor = { 0.75f, 0,1, 0.75f };
    const vec4_t activeColor = {0, 1, 0, 0.75f};

    //set the default colours
    Vector4Copy(twoKeyColor, state.strafeHelper.twoKeyColor);
    Vector4Copy(oneKeyColor, state.strafeHelper.oneKeyColor);
    Vector4Copy(oneKeyColorAlt, state.strafeHelper.oneKeyColorAlt);
    Vector4Copy(rearColor, state.strafeHelper.rearColor);
    Vector4Copy(activeColor, state.strafeHelper.activeColor);

    //set the line height
    state.strafeHelper.LINE_HEIGHT = LINE_HEIGHT;

    //set the precision/sensitivity
    if (cg_strafeHelperPrecision.integer < 100)
        sensitivity = 100;
    else if (cg_strafeHelperPrecision.integer > 10000)
        sensitivity = 10000;
    state.strafeHelper.sensitivity = sensitivity;

    //set the line width
    lineWidth = cg_strafeHelperLineWidth.value;
    if (lineWidth < 0.25f)
        lineWidth = 0.25f;
    else if (lineWidth > 5)
        lineWidth = 5;
    state.strafeHelper.lineWidth = lineWidth;

    state.strafeHelper.offset = cg_strafeHelperOffset.value * 0.01f;

    state.strafeHelper.center = qfalse;
    if(cg_strafeHelper.integer & SHELPER_CENTER){
        state.strafeHelper.center = qtrue;
    }

    state.strafeHelper.rear = qfalse;
    if(cg_strafeHelper.integer & SHELPER_REAR){
        state.strafeHelper.rear = qtrue;
    }
}

//get the line struct
static dfsline DF_GetLine(int moveDir) {

    dfsline lineOut = { 0 };

    qboolean active = qfalse;
    static qboolean draw = qfalse;

    float angleOpt = 0;
    float rearOpt = 0;

    float wishspeed, airAccelerate;
    float opt;

    usercmd_t fakeCmd = { 0 };
    lineOut.rear = qfalse;

    switch (moveDir) {
        case KEY_W:
            if (cg_strafeHelper.integer & SHELPER_W) {
                if (state.cmd.rightmove == 0 && state.cmd.forwardmove > 0) {
                    active = qtrue;
                }
                if(state.physics.hasAirControl)
                {
                    if(!state.strafeHelper.center){
                        draw = qtrue;
                    } else {
                        draw = qfalse;
                    }
                }
                else
                {
                    draw = qtrue;
                }
                fakeCmd.rightmove = 0;
                fakeCmd.forwardmove = 127;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, state.physics.airaccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = 45.0f + opt;
                rearOpt = -45.0f - opt;
                lineOut.rear = qtrue;
            }
            break;
        case KEY_WA:
            if (cg_strafeHelper.integer & SHELPER_WA && state.moveStyle != MOVEMENT_QW) {
                if (state.cmd.rightmove < 0 && state.cmd.forwardmove > 0) {
                    active = qtrue;
                }
                draw = qtrue;
                fakeCmd.rightmove = -127;
                fakeCmd.forwardmove = 127;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, state.physics.airaccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = opt;
                if (state.strafeHelper.rear) {
                    rearOpt = -90.0f - opt;
                    lineOut.rear = qtrue;
                }
            }
            break;
        case KEY_A:
            if (cg_strafeHelper.integer & SHELPER_A) {
                if (state.cmd.rightmove < 0 && state.cmd.forwardmove == 0) {
                    active = qtrue;
                }
                if(state.physics.hasAirControl)
                {
                    if(!state.strafeHelper.center){
                        draw = qtrue;
                    } else {
                        draw = qfalse;
                    }
                }
                else
                {
                    draw = qtrue;
                }
                fakeCmd.rightmove = -127;
                fakeCmd.forwardmove = 0;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                if(state.physics.hasAirControl && wishspeed > state.physics.airstrafewishspeed) {
                    airAccelerate = state.physics.airstrafeaccelerate;
                } else {
                    airAccelerate = state.physics.airaccelerate;
                }
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, airAccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = -45.0f + opt;
                if (state.strafeHelper.rear) {
                    rearOpt = 225.0f - opt;
                    lineOut.rear = qtrue;
                }
            }
            break;
        case KEY_AS:
            if (cg_strafeHelper.integer & SHELPER_SA && state.moveStyle != MOVEMENT_QW) {
                if (state.cmd.rightmove < 0 && state.cmd.forwardmove < 0) {
                    active = qtrue;
                }
                draw = qtrue;
                fakeCmd.rightmove = -127;
                fakeCmd.forwardmove = -127;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, state.physics.airaccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = -90 + opt;
                if (state.strafeHelper.rear) {
                    rearOpt = 180.0f - opt;
                    lineOut.rear = qtrue;
                }
            }
            break;
        case KEY_S:
            if (state.strafeHelper.rear && cg_strafeHelper.integer & SHELPER_S) {
                if (state.cmd.rightmove == 0 && state.cmd.forwardmove < 0) {
                    active = qtrue;
                }
                if(state.physics.hasAirControl)
                {
                    if(!state.strafeHelper.center){
                        draw = qtrue;
                    } else {
                        draw = qfalse;
                    }
                }
                else
                {
                    draw = qtrue;
                }
                fakeCmd.rightmove = 0;
                fakeCmd.forwardmove = -127;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, state.physics.airaccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = 225.0f + opt;
                rearOpt = -225.0f - opt;
                lineOut.rear = qtrue;
            }
            break;
        case KEY_SD:
            if (cg_strafeHelper.integer & SHELPER_SD && state.moveStyle != MOVEMENT_QW) {
                if (state.cmd.rightmove > 0 && state.cmd.forwardmove < 0) {
                    active = qtrue;
                }
                draw = qtrue;
                fakeCmd.rightmove = 127;
                fakeCmd.forwardmove = -127;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, state.physics.airaccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = 90.0f - opt;
                if (state.strafeHelper.rear) {
                    rearOpt = 180.0f + opt;
                    lineOut.rear = qtrue;
                }
                lineOut.active = active;
                lineOut.angleOpt = angleOpt;
            }
            break;
        case KEY_D:
            if (cg_strafeHelper.integer & SHELPER_D) {
                if (state.cmd.rightmove > 0 && state.cmd.forwardmove == 0) {
                    active = qtrue;
                }
                if(state.physics.hasAirControl)
                {
                    if(!state.strafeHelper.center){
                        draw = qtrue;
                    } else {
                        draw = qfalse;
                    }
                }
                else
                {
                    draw = qtrue;
                }
                fakeCmd.rightmove = 127;
                fakeCmd.forwardmove = 0;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                if(state.physics.hasAirControl && wishspeed > state.physics.airstrafewishspeed) {
                    airAccelerate = state.physics.airstrafeaccelerate;
                } else {
                    airAccelerate = state.physics.airaccelerate;
                }
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, airAccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = 45.0f - opt;
                if (state.strafeHelper.rear) {
                    rearOpt = 135.0f + opt;
                    lineOut.rear = qtrue;
                }
            }
            break;
        case KEY_DW:
            if (cg_strafeHelper.integer & SHELPER_WD && state.moveStyle != MOVEMENT_QW) {
                if (state.cmd.rightmove > 0 && state.cmd.forwardmove > 0) {
                    active = qtrue;
                }
                draw = qtrue;
                fakeCmd.rightmove = 127;
                fakeCmd.forwardmove = 127;
                fakeCmd.upmove = state.cmd.upmove;
                wishspeed = DF_GetWishspeed(fakeCmd);
            } else {
                draw = qfalse;
            }
            if (draw) {
                opt = CGAZ_Opt(state.cgaz.wasOnGround && state.onGround, state.physics.accelerate, state.cgaz.currentSpeed, wishspeed, state.cgaz.frametime, state.physics.friction, state.physics.airaccelerate) + state.strafeHelper.offset; //#todo calc cgaz here to get correct wishspeed
                angleOpt = -opt;
                if (state.strafeHelper.rear) {
                    rearOpt = 90.0f + opt;
                    lineOut.rear = qtrue;
                }
            }
            break;
        case KEY_CENTER:
            if (state.physics.hasAirControl && state.strafeHelper.center) {
                if (state.cmd.forwardmove == 0 && state.cmd.rightmove != 0
                    || state.cmd.forwardmove == 0 && state.cmd.rightmove == 0) {
                    active = qtrue;
                }
                draw = qtrue;
            } else {
                draw = qfalse;
            }
            if (draw) {
                angleOpt = 0.0f;
                if (state.strafeHelper.rear) {
                    rearOpt = 180.0f;
                    lineOut.rear = qtrue;
                }
            }
            break;
        default:
            break;
    }

    if(draw){
        lineOut.active = active;
        lineOut.angleOpt = angleOpt;
        lineOut.rearOpt = rearOpt;
        DF_SetAngleToX(&lineOut);

        if(lineOut.onScreen || lineOut.rearOnScreen){
            DF_SetLineColour(&lineOut, moveDir);
        }
    }

    return lineOut;
} //#todo remove code repetition/switch case of keys

//get line x values to pass to drawstrafeine
static void DF_SetAngleToX(dfsline *inLine) {
    float y = 0;
    vec3_t start;
    vec3_t angs, forward, delta, line;
    vec3_t rearAngs, rearForward, rearDelta, rearLine;
    float x = 0;
    float rearX = 0;

    //get the view angles
    VectorCopy(state.viewOrg, start);

    //forward x
    //get the velocity angles
    VectorCopy(state.cgaz.velocityAngles, angs);
    //set distance from velocity angles to optimum
    angs[YAW] += inLine->angleOpt;
    //get forward angle
    AngleVectors(angs, forward, NULL, NULL);
    VectorCopy(angs, inLine->angs);
    // set the line length
    VectorScale(forward, state.strafeHelper.sensitivity, delta);

    //set the line coords
    line[0] = delta[0] + start[0];
    line[1] = delta[1] + start[1];
    line[2] = start[2];

    // is it on the screen?
    if (!CG_WorldCoordToScreenCoord(line, &x, &y)) {
        inLine->onScreen = qfalse;
    } else {
        inLine->onScreen = qtrue;
        x -= (0.5f * state.strafeHelper.lineWidth);
        inLine->x = x;
    }

    if(inLine->rear == qtrue) {
        //rear x
        //get the velocity angles
        VectorCopy(state.cgaz.velocityAngles, rearAngs);
        //set distance from velocity angles to rear optimum
        rearAngs[YAW] += inLine->rearOpt;
        //get forward angle
        AngleVectors(rearAngs, rearForward, NULL, NULL);
        VectorCopy(rearAngs, inLine->rearAngs);
        // set the line length
        VectorScale(rearForward, state.strafeHelper.sensitivity, rearDelta);

        //set the line coords
        rearLine[0] = rearDelta[0] + start[0];
        rearLine[1] = rearDelta[1] + start[1];
        rearLine[2] = start[2];

        // is it on the screen?
        if (!CG_WorldCoordToScreenCoord(rearLine, &rearX, &y)) {
            inLine->rearOnScreen = qfalse;
        } else {
            inLine->rearOnScreen = qtrue;
            rearX -= (0.5f * state.strafeHelper.lineWidth);
            inLine->rearX = rearX;
        }
    }
} //#todo remove code repetition between front/rear lines
//set the colour of the line
static void DF_SetLineColour(dfsline* inLine, int moveDir){
    vec4_t color = { 1, 1, 1, 0.75f };
    //get the default line colour
    Vector4Copy(color,  inLine->color);
    //set the colors
    if (inLine->active) {
        if (cg_strafeHelperActiveColor.value) { //does the user have a custom active colour set
            color[0] = cg.strafeHelperActiveColor[0];
            color[1] = cg.strafeHelperActiveColor[1];
            color[2] = cg.strafeHelperActiveColor[2];
            color[3] = cg.strafeHelperActiveColor[3];
        } else { //make it green
            color[0] = state.strafeHelper.activeColor[0];
            color[1] = state.strafeHelper.activeColor[1];
            color[2] = state.strafeHelper.activeColor[2];
            color[3] = state.strafeHelper.activeColor[3];
        }
        memcpy(inLine->color, color, sizeof(vec4_t));
    } else { //set the other colours
        if (moveDir == KEY_WA || moveDir == KEY_DW) {
            memcpy(inLine->color, state.strafeHelper.twoKeyColor, sizeof(vec4_t));
        } else if (moveDir == KEY_A || moveDir == KEY_D) {
            memcpy(inLine->color, state.strafeHelper.oneKeyColor, sizeof(vec4_t));
        } else if (moveDir == KEY_W || moveDir == KEY_S || moveDir == KEY_CENTER) {
            memcpy(inLine->color, state.strafeHelper.oneKeyColorAlt, sizeof(vec4_t));
        } else if (moveDir == KEY_AS || moveDir == KEY_SD) {
            memcpy(inLine->color, state.strafeHelper.rearColor, sizeof(vec4_t));
        }
    }
    inLine->color[3] = cg_strafeHelperInactiveAlpha.value / 255.0f;
}

/* Strafehelper Value Calculators */
//calculates the optimum cgaz angle //#todo implement other cgaz values (min,max)
static float CGAZ_Opt(qboolean onGround, float accelerate, float currentSpeed, float wishSpeed, float frametime, float friction, float airaccelerate){
    float optimumDelta;
    if (onGround) {
        optimumDelta = (float) acos(
                (double) ((wishSpeed - (accelerate * wishSpeed * frametime)) /
                          (currentSpeed * (1 - friction * frametime)))) * (180.0f / M_PI) -
                       45.0f;
    } else {
        optimumDelta = (float) acos(
                (double) ((wishSpeed - (airaccelerate * wishSpeed * frametime)) /
                          currentSpeed)) * (180.0f / M_PI) - 45.0f;
    }
    if (optimumDelta < 0 || optimumDelta > 360) {
        optimumDelta = 0;
    }
    return optimumDelta;
}
//takes a user commmand and returns the emulated wishspeed as a float

static float DF_GetWishspeed(usercmd_t inCmd){
    int         i;
    vec3_t		wishvel;
    float		fmove, smove;
    vec3_t		forward, right, up;
    float		wishspeed;
    float		scale;

    fmove = inCmd.forwardmove;
    smove = inCmd.rightmove;

    scale = DF_GetCmdScale( inCmd );

    AngleVectors(state.viewAngles, forward, right, up);
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
    wishspeed = VectorNormalize(wishvel);

    if(state.moveStyle != MOVEMENT_SP) {
        wishspeed = cg.predictedPlayerState.speed;
        if(state.physics.hasAirControl && (wishspeed > state.physics.airstrafewishspeed) && (fmove == 0 && smove != 0)){
            wishspeed = state.physics.airstrafewishspeed;
        }
    }

    if(state.moveStyle == MOVEMENT_SP){
        if(state.onGround){
            if(state.cgaz.wasOnGround){ //spent more than 1 frame on the ground = friction
                wishspeed *= scale; //scale is only factored on the ground for SP
            }
        }
        if(DotProduct (state.velocity, wishvel) < 0.0f) {
            wishspeed *= state.physics.airdecelrate;
        }
    }

    return wishspeed;
} //#todo emulate pmovesingle instead of this
//takes a user command and returns the emulated command scale as a float
static float DF_GetCmdScale( usercmd_t cmd) {
    int		max;
    float	total;
    float	scale;
    signed char		umove = 0; //cmd->upmove;
    //don't factor upmove into scaling speed

    if(cgs.upCmdScale || state.moveStyle == MOVEMENT_SP){ //upmove velocity scaling
        umove = state.cmd.upmove;
    }
    max = abs( cmd.forwardmove );
    if ( abs( cmd.rightmove ) > max ) {
        max = abs( cmd.rightmove );
    }
    if ( abs( umove ) > max ) {
        max = abs( umove );
    }
    if ( !max ) {
        scale = 0;
    }

    total = (float)sqrt( cmd.forwardmove * cmd.forwardmove
                         + cmd.rightmove * cmd.rightmove + umove * umove );
    scale = state.cgaz.currentSpeed * (float)max / ( 127.0 * total );

    return scale;
}

/* Strafehelper Style Distributor */
//takes a strafe line and draws it according to the strafehelper style set
static void DF_DrawStrafeLine(dfsline line) {

    if (cg_strafeHelper.integer & SHELPER_UPDATED) { //draw the updated style here
        int cutoff = (int)cgs.screenHeight - cg_strafeHelperCutoff.integer;
        int heightIn = state.strafeHelper.LINE_HEIGHT;

        if (cg_strafeHelper.integer & SHELPER_TINY) {
            cutoff = state.strafeHelper.LINE_HEIGHT + 15;
            heightIn = state.strafeHelper.LINE_HEIGHT + 5;
        } else if (cutoff < state.strafeHelper.LINE_HEIGHT + 20) {
            cutoff = state.strafeHelper.LINE_HEIGHT + 20;
        } else if ((float)cutoff > cgs.screenHeight) {
            cutoff = (int)cgs.screenHeight;
        }
        DF_DrawLine((0.5f * cgs.screenWidth) - (0.5f * state.strafeHelper.lineWidth), cgs.screenHeight, line.x, (float)heightIn, state.strafeHelper.lineWidth, line.color, line.color[3], (float)cutoff);
    }

    if (cg_strafeHelper.integer & SHELPER_CGAZ) { //draw the cgaz style strafehelper
        if (cg_strafeHelperCutoff.integer > 256) {
            DF_DrawLine(line.x, (0.5f * cgs.screenHeight) + 4, line.x, (0.5f * cgs.screenHeight) - 4, state.strafeHelper.lineWidth, line.color, 0.75f, 0); //maximum cutoff
        } else {
            DF_DrawLine(line.x, (0.5f * cgs.screenHeight) + 20 - (float)cg_strafeHelperCutoff.integer / 16.0f, line.x, (0.5f * cgs.screenHeight) - 20 + (float)cg_strafeHelperCutoff.integer / 16.0f, state.strafeHelper.lineWidth, line.color, 0.75f, 0); //default/custom cutoff
        }
    }

    if (cg_strafeHelper.integer & SHELPER_WSW && line.active && state.moveDir != 0) { //draw the wsw style strafehelper, not sure how to deal with multiple lines for W only so we don't draw any, the proper way is to tell which line we are closest to aiming at and display the strafehelper for that
        float width = (float)(-4.444 * AngleSubtract(state.viewAngles[YAW], line.angs[YAW]));
        CG_FillRect((0.5f * cgs.screenWidth), (0.5f * cgs.screenHeight), width, 12, colorTable[CT_RED]);
    }

    if (cg_strafeHelper.integer & SHELPER_WEZE) { //call the weze style strafehelper function
        DF_DrawStrafehelperWeze(state.moveDir, line);
    }

    if (cg_strafeHelper.integer & SHELPER_SOUND && line.active && state.moveDir != 8) { //strafehelper sounds - don't do them for the center line, since it's not really a strafe
        DF_StrafeHelperSound(100 * AngleSubtract(state.viewAngles[YAW], line.angs[YAW]));
    }
} //#todo make each strafehelper style its own function

/* Drawing Functions */
//draws a line on the screen
static void DF_DrawLine(float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff) {
    float stepx, stepy, length = (float)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    int i;

    if (length < 1)
        length = 1;
    else if (length > 2000)
        length = 2000;
    if (!ycutoff)
        ycutoff = cgs.screenHeight;

    stepx = (x2 - x1) / (length / size);
    stepy = (y2 - y1) / (length / size);

    trap_R_SetColor(color);

    for (i = 0; i <= (length / size); i++) {
        if (x1 < cgs.screenWidth && y1 < cgs.screenHeight && y1 < ycutoff)
            CG_DrawPic(x1, y1, size, size, cgs.media.whiteShader);
        x1 += stepx;
        y1 += stepy;
    }
    trap_R_SetColor(NULL);

}
//draws the weze strafehelper
/*
===================
CG_DrawStrafehelperWeze
japro - draw the weze style strafehelper(previously known as Dzikie_CG_DrawSpeed)
===================
*/
static void DF_DrawStrafehelperWeze(int moveDir, dfsline inLine) {
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

    midx = 0.5f * cgs.screenWidth;
    midy = 0.5f * cgs.screenHeight;
    VectorCopy(state.velocity, velocity_copy);
    velocity_copy[2] = 0;
    VectorCopy(state.viewAngles, viewangle_copy);
    viewangle_copy[PITCH] = 0;
    length = VectorNormalize(velocity_copy);
    g_speed = DF_GetWishspeed(state.cmd);
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

    DF_DrawLine(midx, midy, midx + length * sin(diff), midy - length * cos(diff), 1, colorRed, 0.75f, 0);
    DF_DrawLine(midx, midy, midx + state.cmd.rightmove, midy - state.cmd.forwardmove, 1, colorCyan, 0.75f, 0);
    DF_DrawLine(midx, midy, midx + length / 2 * sin(diff + optiangle), midy - length / 2 * cos(diff + optiangle), 1, colorRed, 0.75f, 0);
    DF_DrawLine(midx, midy, midx + length / 2 * sin(diff - optiangle), midy - length / 2 * cos(diff - optiangle), 1, colorRed, 0.75f, 0);
} //#todo fix this
//plays the strafehelper sounds
static void DF_StrafeHelperSound(float difference) {
    if (difference > -40.0f && difference < 10.0f) //Under aiming by a bit, but still good
        trap_S_StartLocalSound(cgs.media.hitSound4, CHAN_LOCAL_SOUND);
}
//draws the strafehelper triangles //#todo fix this
static int DF_GetStrafeTriangleAccel(void) {
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
static float* DF_GetStrafeTrianglesX(vec3_t velocity, float diff, float sensitivity) {
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
static void DF_DrawStrafeTriangles(vec3_t velocity, float diff, float wishspeed, int moveDir) {
    float lx, lx2, rx, rx2, lineWidth, sensitivity, accel, optimalAccel, potentialSpeed;
    static float lWidth, rWidth, width;
    vec4_t color1 = { 1.0f, 1.0f, 1.0f, 0.8f };

    accel = (float)DF_GetStrafeTriangleAccel();
    optimalAccel = wishspeed * ((float)cg.frametime / 1000.0f);
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

    if (DF_GetStrafeTrianglesX(velocity, diff, sensitivity) != NULL && DF_GetStrafeTrianglesX(velocity, 90 - diff, sensitivity) != NULL) { //A
        lx = *DF_GetStrafeTrianglesX(velocity, diff, sensitivity);
        lx2 = *DF_GetStrafeTrianglesX(velocity, 90 - diff, sensitivity);
        lWidth = (lx - lx2) / 2.0f;
    }

    if (DF_GetStrafeTrianglesX(velocity, -diff, sensitivity) != NULL && DF_GetStrafeTrianglesX(velocity, diff - 90, sensitivity) != NULL) { //D
        rx = *DF_GetStrafeTrianglesX(velocity, -diff, sensitivity);
        rx2 = *DF_GetStrafeTrianglesX(velocity, diff - 90, sensitivity);
        rWidth = (rx2 - rx) / 2.0f;
    }

    if (lWidth && lWidth > 0 && rWidth > lWidth) { //use smaller width for both to avoid perspective stretching
        width = lWidth;
    }
    else if (rWidth && rWidth > 0 && lWidth > rWidth) {
        width = rWidth;
    }

    trap_R_SetColor(color1); //set the color

    if (DF_GetStrafeTrianglesX(velocity, diff, sensitivity) != NULL) { //draw the triangles if they exist
        CG_DrawPic(*DF_GetStrafeTrianglesX(velocity, diff, sensitivity) - width - (0.5f * lineWidth), (0.5f * cgs.screenHeight) - 4.0f, width, 8.0f, cgs.media.leftTriangle);
    }
    if (DF_GetStrafeTrianglesX(velocity, -diff, sensitivity) != NULL) {
        CG_DrawPic(*DF_GetStrafeTrianglesX(velocity, -diff, sensitivity) + (0.5f * lineWidth), (0.5f * cgs.screenHeight) - 4.0f, width, 8.0f, cgs.media.rightTriangle);
    }
}

/*  Speedometer */ //#todo implement new structs, remove repetition
/*
===================
CG_GraphAddSpeed
tremulous - append a speed to the sample history for the speed graph
===================
*/
static void DF_GraphAddSpeed(void) {
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
static void DF_DrawSpeedGraph(rectDef_c* rect, const vec4_t foreColor, vec4_t backColor) {
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
static void DF_DrawJumpHeight(centity_t* cent) {
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
static void DF_DrawJumpDistance(void) {
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
static void DF_DrawVerticalSpeed(void) {
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
static void DF_DrawAccelMeter(void) {
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
static void DF_DrawSpeedometer(void) {
    const char* accelStr, * accelStr2, * accelStr3;
    char speedStr[32] = { 0 }, speedStr2[32] = { 0 }, speedStr3[32] = { 0 };
    vec4_t colorSpeed = { 1, 1, 1, 1 };
    const float currentSpeed = state.cgaz.currentSpeed;
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
        Com_sprintf(speedStr, sizeof(speedStr), "   %.0f", (float)floor((double)currentSpeed + 0.5f));
        CG_Text_Paint(speedometerXPos, cg_speedometerY.value, cg_speedometerSize.value, colorWhite, accelStr, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
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

/*  Movement Keys */ //#todo implement new structs, remove repetition
/*
===================
CG_GetGroundDistance
japro - Ground Distance function for use in jump detection for movement keys
===================
*/
static float DF_GetGroundDistance(void) {
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
static void DF_DrawMovementKeys(centity_t* cent) {
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

        if ((DF_GetGroundDistance() > 1 && zspeed > 8 && zspeed > lastZSpeed && !cg.snap->ps.fd.forceGripCripple) || (cg.snap->ps.pm_flags & PMF_JUMP_HELD))
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
