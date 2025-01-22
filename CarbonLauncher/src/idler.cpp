#include "idler.h"

void Idler::IdleBySleeping() const {
    ZoneScoped;

    if ((this->idleFPSLimit <= 0.f) || !this->enableIdling)
        return;

    double waitTimeout = 1. / (double)this->idleFPSLimit;

    glfwWaitEventsTimeout(waitTimeout);
}