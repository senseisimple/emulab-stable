
#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define GROBOT_SIM

#include "grobot.h"

grobot::grobot()
{
}

grobot::~grobot()
{
}

void grobot::estop()
{
}

void grobot::setWheels(float Vl, float Vr)
{
}

void grobot::setvPath(float Wv, float Wr)
{
}

void grobot::pbMove(float mdisplacement)
{
    
}

void grobot::pbPivot(float pangle)
{
}

void grobot::dgoto(float Dx, float Dy, float Rf)
{
    printf("goto %f, %f, %f\n", Dx, Dy, Rf);

    this->dx_est = Dx;
    this->dy_est = Dy;
    this->gotocomplete = 1;
}

void grobot::resetPosition()
{
}

void grobot::updatePosition()
{
}

float grobot::getArclen()
{
    return 0.0f;
}

void grobot::getDisplacement(float &dxtemp, float &dytemp)
{
    dxtemp = this->dx_est;
    dytemp = this->dy_est;
}

int grobot::getGstatus()
{
    return 0;
}

int grobot::getGOTOstatus()
{
    int retval = this->gotocomplete;

    this->gotocomplete = 0;
    return retval;
}

void grobot::sleepy()
{
    struct timeval tv = { 0, 100 };

    select(0, NULL, NULL, NULL, &tv);
}

void grobot::setCBexec(int id)
{
}

void grobot::setCBstatus(int id, int status)
{
}

void grobot::createNULLbehavior()
{
}

void grobot::createPRIMbehavior()
{
}

void grobot::set_gotocomplete()
{
}
