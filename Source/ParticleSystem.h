#pragma once
#include "Math.h"
//#include "Entity.h"

typedef uint32 ParticleGenID;

struct ParticleParams {
    Vector spawnLocation = {};
    Vector initialVel = {};

    Vector coneAccelDirection = {};
    Vector coneDegreeRange = {}; // { positive, negative } rotation
    Color colorRangeLo = {};
    Color colorRangeHi = {};

    float particleSize = 0;
    float lifeTime = 0;
    float particlesPerSecond = 0;
    float fadeInTime = 0;
    float fadeOutTime = 0;

    Vector terminalVelocity = { 1000, 1000 };
};

struct Generator {
    Vector location = {};
    Vector velocity = {};

    Vector directionLo = {};
    Vector directionHi = {};
    Color colorRangeLo = {};
    Color colorRangeHi = {};

    float lifeTime = 0;
    float particlesPerSecond = 0;
    float particleSize = 1.0f;
    float fadeInTime = 0;

    float fadeOutTime = 0;
    Vector terminalVelocity = {};
    float timeLeftToSpawn = 0;

    ParticleGenID ID = 0;
    bool inUse = false;

    void UpdateGeneratorLocation(const Vector& loc, const Vector& vel)
    {
        location = loc;
        velocity = vel;
    };
};

void ParticleInit();
ParticleGenID CreateParticleGenerator(ParticleParams p);
Generator* GetParticleGenerator(ParticleGenID ID);
void UpdateParticles(float dt, float gravity);
void Play(const ParticleGenID ID);
//void Pause(const ParticleGenID ID);
void Stop(const ParticleGenID ID);
//void Mute(const ParticleGenID ID);
