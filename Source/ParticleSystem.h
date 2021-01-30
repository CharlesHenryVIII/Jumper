#pragma once
#include "Math.h"
//#include "Entity.h"

typedef uint32 ParticleGenID;

struct ParticleParams {
    Vector spawnLocation = {};

    Color colorRangeLo = { 0.0f, 0.0f, 0.0f, 0.0f };
    Color colorRangeHi = { 1.0f, 1.0f, 1.0f, 1.0f };

    Vector coneDir = {};
    float coneDeg = {};

    float particleSpeed = 2.0f;
    float particleSize = 0.1f;
    float lifeTime = 0;
    float particlesPerSecond = 0;
    float fadeInTime = 0;
    float fadeOutTime = 0;

    Vector terminalVelocity = { 1000, 1000 };
};

struct Generator {
    Vector location = {};

    Color colorRangeLo = {};
    Color colorRangeHi = {};

    Vector coneDir = {};
    float coneDeg = 0.0f;
    float particleSpeed;

    float lifeTime = 0;
    float particlesPerSecond = 0;
    float particleSize;
    float fadeInTime = 0;

    float fadeOutTime = 0;
    Vector terminalVelocity = {};
    float timeLeftToSpawn = 0;

    ParticleGenID ID = 0;
    bool inUse = false;

    void UpdateGeneratorLocation(const Vector& loc, const Vector& vel)
    {
        location = loc;
    };
};

void ParticleInit();
ParticleGenID CreateParticleGenerator(ParticleParams p);
Generator* GetParticleGenerator(ParticleGenID ID);
void UpdateParticles(float dt, float gravity);
void Play(const ParticleGenID ID);
void Pause(const ParticleGenID ID);
//void Stop(const ParticleGenID ID);
//void Mute(const ParticleGenID ID);
