#include "cg_local.h"
#include "cg_strafehelper.h"

static float speedometerXPos, jumpsXPos;
static float firstSpeed;
static float speedSamples[SPEEDOMETER_NUM_SAMPLES];
static int oldestSpeedSample = 0;
static int maxSpeedSample = 0;

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

/*  Speedometer Keys */

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
static void CG_DrawStrafeTriangles(vec3_t velocity, float diff, float wishspeed, int moveDir) {
    float lx, lx2, rx, rx2, lineWidth, sensitivity, accel, optimalAccel, potentialSpeed;
    static float lWidth, rWidth, width;
    vec4_t color1 = { 1.0f, 1.0f, 1.0f, 0.8f };

    accel = (float)CG_GetStrafeTriangleAccel();
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

/*
===================
CG_CmdScale
japro - used to emulate BG_CmdScale
===================
*/

static float CG_CmdScale( usercmd_t *cmd ) {
    int		max;
    float	total;
    float	scale;
    int		umove = 0; //cmd->upmove;
    //don't factor upmove into scaling speed

    if(cgs.upCmdScale || cgs.movement == MOVEMENT_SP){ //upmove velocity scaling
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
    scale = (float)cg.currentSpeed * max / ( 127.0 * total );

    return scale;
}

/*
===================
CG_CmdScale
DFMania - Get wishspeed
===================
*/
static float CG_GetWishspeed( usercmd_t *cmd){
    int         i;
    vec3_t		wishvel;
    float		fmove, smove;
    vec3_t		forward, right, up;
    float		wishspeed;
    float		scale;

    fmove = cmd->forwardmove;
    smove = cmd->rightmove;

    scale = CG_CmdScale( cmd );

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
    VectorCopy (wishvel, cg.wishdir); //set the wishdir here for now
    wishspeed = VectorNormalize(cg.wishdir);
    if(cgs.movement != MOVEMENT_SP) {
        wishspeed *= scale;
    }else if(cg.predictedPlayerState.groundEntityNum == ENTITYNUM_WORLD){
        if(cg.wasOnGround){ //spent more than 1 frame on the ground = friction
            wishspeed *= scale; //scale is only factored on the ground for SP
        }else if(DotProduct (cg.predictedPlayerState.velocity, cg.wishdir) < 0.0f) {
            wishspeed *= 1.35f;
        }
        cg.wasOnGround = qtrue;
    } else {
        cg.wasOnGround = qfalse;
    }

    return wishspeed;
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
    float pmAccel = 10.0f, pmAirAccel = 1.0f, pmFriction = 6.0f, frametime, optimalDeltaAngle, wishspeed = cg.predictedPlayerState.speed;
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
        if (cg.snap->ps.pm_flags & PMF_JUMP_HELD)
            cmd.upmove = 127;
    }
    else {
        return; //No cg.snap causes this to return.
    }

    onGround = (qboolean)(cg.predictedPlayerState.groundEntityNum == ENTITYNUM_WORLD);

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
    if(cgs.upCmdScale || moveStyle == MOVEMENT_SP){
        wishspeed = CG_GetWishspeed(&cmd);
    }

    if (currentSpeed < (wishspeed - 1))
        return;

    if (cg_strafeHelper_FPS.value < 1)
        frametime = ((float)cg.frametime * 0.001f);
    else if (cg_strafeHelper_FPS.value > 1000)
        frametime = 1;
    else frametime = 1 / cg_strafeHelper_FPS.value;

    if (onGround)
        optimalDeltaAngle = acos((double)((wishspeed - (pmAccel * wishspeed * frametime)) / (currentSpeed * (1 - pmFriction * (frametime))))) * (180.0f / M_PI) - 45.0f;
    else
        optimalDeltaAngle = acos((double)((wishspeed - (pmAirAccel * wishspeed * frametime)) / currentSpeed)) * (180.0f / M_PI) - 45.0f;

    if (optimalDeltaAngle < 0 || optimalDeltaAngle > 360)
        optimalDeltaAngle = 0;

    velocity[2] = 0;
    vectoangles(velocity, velocityAngle); //We have the offset from our Velocity angle that we should be aiming at, so now we need to get our velocity angle.

    if ((moveStyle == MOVEMENT_CPMA || moveStyle == MOVEMENT_WSW || moveStyle == MOVEMENT_PJK || moveStyle == MOVEMENT_SLICK) && cg_strafeHelper.integer & SHELPER_ACCELZONES) { //Accel Zones (air control styles)
        CG_DrawStrafeTriangles(velocityAngle, optimalDeltaAngle, wishspeed, moveDir);
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

/*  Movement Keys */

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


/* Draw The Hud */

void CG_DrawStrafeHUD(centity_t	*cent)
{
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

    //japro strafehelper line crosshair
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
}
