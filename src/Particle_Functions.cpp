#include "Particle_Functions.h"

Particle_Functions *Particle_Functions::_instance;

// [static]
Particle_Functions &Particle_Functions::instance() {
    if (!_instance) {
        _instance = new Particle_Functions();
    }
    return *_instance;
}

Particle_Functions::Particle_Functions() {
}

Particle_Functions::~Particle_Functions() {
}

void Particle_Functions::setup() {
}

void Particle_Functions::loop() {
    // Put your code to run during the application thread loop here
}

