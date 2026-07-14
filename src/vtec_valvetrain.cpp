#include "../include/vtec_valvetrain.h"

#include "../include/engine.h"
#include "../include/camshaft.h"

VtecValvetrain::VtecValvetrain() {
    m_intakeCamshaft = nullptr;
    m_exhaustCamshaft = nullptr;

    m_vtecIntakeCamshaft = nullptr;
    m_vtecExhaustCamshaft = nullptr;

    m_engine = nullptr;

    m_minRpm = 0.0;
    m_minSpeed = 0.0;
    m_minThrottlePosition = 0.0;
    m_manifoldVacuum = 0.0;
}

VtecValvetrain::~VtecValvetrain() {
    // We own all four camshafts (new'd by the VTEC valvetrain script node and handed to us).
    // destroy() releases each lobe-angle buffer before we free the object. m_engine is a
    // back-reference we do not own.
    Camshaft *cams[] = {
        m_intakeCamshaft, m_exhaustCamshaft, m_vtecIntakeCamshaft, m_vtecExhaustCamshaft };
    for (Camshaft *cam : cams) {
        if (cam != nullptr) { cam->destroy(); delete cam; }
    }
    m_intakeCamshaft = nullptr;
    m_exhaustCamshaft = nullptr;
    m_vtecIntakeCamshaft = nullptr;
    m_vtecExhaustCamshaft = nullptr;
}

void VtecValvetrain::initialize(const Parameters &parameters) {
    m_intakeCamshaft = parameters.IntakeCamshaft;
    m_exhaustCamshaft = parameters.ExhaustCamshaft;
    m_vtecIntakeCamshaft = parameters.VtecIntakeCamshaft;
    m_vtecExhaustCamshaft = parameters.VtexExhaustCamshaft;

    m_minRpm = parameters.MinRpm;
    m_minSpeed = parameters.MinSpeed;
    m_minThrottlePosition = parameters.MinThrottlePosition;
    m_manifoldVacuum = parameters.ManifoldVacuum;
    m_engine = parameters.Engine;
}

double VtecValvetrain::intakeValveLift(int cylinder) {
    return isVtecEnabled()
        ? m_vtecIntakeCamshaft->valveLift(cylinder)
        : m_intakeCamshaft->valveLift(cylinder);
}

double VtecValvetrain::exhaustValveLift(int cylinder) {
    return isVtecEnabled()
        ? m_vtecExhaustCamshaft->valveLift(cylinder)
        : m_exhaustCamshaft->valveLift(cylinder);
}

Camshaft *VtecValvetrain::getActiveIntakeCamshaft() {
    return isVtecEnabled()
        ? m_vtecIntakeCamshaft
        : m_intakeCamshaft;
}

Camshaft *VtecValvetrain::getActiveExhaustCamshaft() {
    return isVtecEnabled()
        ? m_vtecExhaustCamshaft
        : m_exhaustCamshaft;
}

bool VtecValvetrain::isVtecEnabled() const {
    return
        m_engine->getManifoldPressure() > m_manifoldVacuum
        && m_engine->getSpeed() > m_minRpm
        && (1 - m_engine->getThrottle()) > m_minThrottlePosition;
}
