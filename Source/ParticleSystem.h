#pragma once
#include "Math.h"

#include <vector>

struct ParticleParams {
    Vector spawnLocation = {};

    Color colorRangeLo = { 0.0f, 0.0f, 0.0f, 0.0f };
    Color colorRangeHi = { 1.0f, 1.0f, 1.0f, 1.0f };

    //Vector coneDir = {};
    //float coneDir = 0;
    Range coneDeg = {};

    float gravity = -20.0f;
    float particleSpeed = 2.0f;
    float particleSize = 0.1f;
    float lifeTime = 0;
    float particlesPerSecond = 0;
    float fadeInTime = 0;
    float fadeOutTime = 0;

    Vector terminalVelocity = { 1000, 1000 };
};

struct Particle {

    Vector location = {};
    Vector velocity = {};
    Vector acceleration = {};
    Vector terminalVelocity = { 10.0f, 10.0f };

    Color color = {};
    float timeLeft = 4; //seconds
    float fadeInTime = 1; //seconds
    float fadeOutTime = 1; //seconds

    float size;
    bool inUse = true;
};

struct ParticleSystem {

	std::vector<Particle> particles;

	ParticleParams pp;

    Vector location = {};
    float timeLeftToSpawn = 0;
	bool playing = true;

    ParticleSystem(const ParticleParams& info) : pp(info) {}
    void Update(float deltaTime);
    void Render();
};
