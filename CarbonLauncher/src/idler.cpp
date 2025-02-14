#include "pch.h"
#include "idler.h"

using namespace CL;

void Idler::IdleBySleeping() const {
    if ((this->idleFPSLimit <= 0.f) || !this->enableIdling)
        return;

    double waitTimeout = 1. / (double)this->idleFPSLimit;
    glfwWaitEventsTimeout(waitTimeout);
}