#include "../include/standard_valvetrain.h"

#include "../include/camshaft.h"

StandardValvetrain::StandardValvetrain() {
    m_intakeCamshaft = nullptr;
    m_exhaustCamshaft = nullptr;
}

StandardValvetrain::~StandardValvetrain() {
    // The camshafts are new'd by the valvetrain script node and handed to us; we own them.
    // destroy() releases their lobe-angle buffers before we free the objects themselves.
    if (m_intakeCamshaft != nullptr) { m_intakeCamshaft->destroy(); delete m_intakeCamshaft; }
    if (m_exhaustCamshaft != nullptr) { m_exhaustCamshaft->destroy(); delete m_exhaustCamshaft; }
    m_intakeCamshaft = nullptr;
    m_exhaustCamshaft = nullptr;
}

void StandardValvetrain::initialize(const Parameters &params) {
    m_intakeCamshaft = params.IntakeCamshaft;
    m_exhaustCamshaft = params.ExhaustCamshaft;
}

double StandardValvetrain::intakeValveLift(int cylinder) {
    return m_intakeCamshaft->valveLift(cylinder);
}

double StandardValvetrain::exhaustValveLift(int cylinder) {
    return m_exhaustCamshaft->valveLift(cylinder);
}

Camshaft *StandardValvetrain::getActiveIntakeCamshaft() {
    return m_intakeCamshaft;
}

Camshaft *StandardValvetrain::getActiveExhaustCamshaft() {
    return m_exhaustCamshaft;
}
