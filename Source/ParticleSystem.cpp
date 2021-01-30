#include "ParticleSystem.h"
#include "Rendering.h"

#include <vector>
#include <unordered_map>

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
    ParticleGenID GenID = 0;

    bool inUse = true;

	//void UpdateParticlePosition(float gravity, float dt)
	//{
	//	velocity.x += acceleration.x * dt;
	//	velocity.y += gravity * dt;

	//	if (terminalVelocity.x > 0.5)
	//	{
	//		velocity.x -= ((velocity.x / 2.0f) * (1.5f) * dt);
	//		velocity.x = Clamp(velocity.x, -terminalVelocity.x, terminalVelocity.x);
	//	}

	//	if (terminalVelocity.y > 0.5)
	//	{
	//		velocity.y -= ((velocity.y / 2.0f) * (1.5f) * dt);
	//		velocity.y = Clamp(velocity.y, -terminalVelocity.y, terminalVelocity.y);
	//	}

	//	location += velocity * dt;
	//}

};


std::unordered_map<ParticleGenID, Generator> s_generators;
std::vector<Particle> s_particles;


ParticleGenID s_RefParticleGenID = 0;
ParticleGenID CreateParticleGenerator(ParticleParams p)
{
    Generator g = {
        .location = p.spawnLocation,

        .colorRangeLo = p.colorRangeLo,
        .colorRangeHi = p.colorRangeHi,

        .coneDir = p.coneDir,
        .coneDeg = p.coneDeg,
        .particleSpeed = p.particleSpeed,

        .lifeTime = p.lifeTime,
        .particlesPerSecond = p.particlesPerSecond,
        .particleSize = p.particleSize,
        .fadeInTime = p.fadeInTime,

        .fadeOutTime = p.fadeOutTime,
        .terminalVelocity = p.terminalVelocity,
        .timeLeftToSpawn = 0,

        .ID = ++s_RefParticleGenID,
    };
    s_generators[g.ID] = g;

    return g.ID;
}

void Play(const ParticleGenID ID)
{
    GetParticleGenerator(ID)->inUse = true;
}

void Pause(const ParticleGenID ID)
{
	GetParticleGenerator(ID)->inUse = false;
}

void Stop(const ParticleGenID ID)
{

}

void Mute(const ParticleGenID ID)
{

}

Color CreateRandomColor(Color min, Color max)
{
    return { Random(min.r, max.r),
            Random(min.g, max.g),
            Random(min.b, max.b),
            Random(min.a, max.a) };
}

Generator* GetParticleGenerator(ParticleGenID ID)
{
    for (uint32 i = 0; i < s_generators.size(); i++)
    {
        if (s_generators[i].ID == ID)
            return &s_generators[i];
    }
    return nullptr;
}

void ParticleInit()
{
    s_particles.reserve(20000);
}

void UpdateParticles(float dt, float gravity)
{

    //Update Generators
	for (uint32 i = 0; i < s_generators.size(); i++)
	{
		Generator& g = s_generators[i];

        //Create Particles
        g.timeLeftToSpawn -= dt;
		while (g.inUse && g.timeLeftToSpawn <= 0)
		{

            Vector result = {};
            float coneDeg = DegToRad(Random<float>(-g.coneDeg, g.coneDeg));

            //gbMat4 rotateMat;
            //gb_mat4_rotate(&rotateMat, Vec2TogbVec3(Normalize(g.coneDir) * g.particleSpeed), coneDeg);
            //
            //Vector vel = gbMat4ToVec2(rotateMat);

            Vector vel = RotateVector(Normalize(g.coneDir), coneDeg);

            Particle p = {
                .location = g.location,
                .velocity = vel * g.particleSpeed,
                .acceleration = vel,

				.color = CreateRandomColor(g.colorRangeLo, g.colorRangeHi),
				.timeLeft = g.lifeTime, //seconds
				.fadeInTime = g.fadeInTime,
				.fadeOutTime = g.fadeOutTime, //seconds

				.size = g.particleSize,
				.GenID = g.ID,
			};
			s_particles.push_back(p);

			g.timeLeftToSpawn += (1.0f / g.particlesPerSecond);
		}

	}

	//Update Particles
	for (uint32 i = 0; i < s_particles.size(); i++)
	{
		Particle& p = s_particles[i];
		p.velocity.x += p.acceleration.x * dt;
		p.velocity.y += gravity * dt;

		if (p.terminalVelocity.x > 0.5)
		{
			p.velocity.x -= ((p.velocity.x / 2.0f) * (1.5f) * dt);
			p.velocity.x = Clamp(p.velocity.x, -p.terminalVelocity.x, p.terminalVelocity.x);
		}

		if (p.terminalVelocity.y > 0.5)
		{
			p.velocity.y -= ((p.velocity.y / 2.0f) * (1.5f) * dt);
			p.velocity.y = Clamp(p.velocity.y, -p.terminalVelocity.y, p.terminalVelocity.y);
		}

		p.location += p.velocity * dt;

        if (p.timeLeft <= 0.0f)
            p.inUse = false;
        p.timeLeft -= dt;
	}

    //Remove Particles
    std::erase_if(s_particles, [](Particle p) { return p.inUse == false; } );

	//std::erase_if(s_audioPlaying, [ID](AudioInstance a) { return a.ID == ID; });

	//Render Particles
	for (uint32 i = 0; i < s_particles.size(); i++)
	{
        const Particle& p = s_particles[i];
        float s = p.size / 2;
        Rectangle r = {
            .botLeft = { p.location.x - s, p.location.y - s},
            .topRight = { p.location.x + s, p.location.y + s},
        };
        AddRectToRender(r, p.color, RenderPrio::Sprites, CoordinateSpace::World);
	}
}
