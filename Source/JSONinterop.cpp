#include "JSON/picojson.h"
#include "WinUtilities.h"
#include "Math.h"
#include "Entity.h"
#include "JSONInterop.h"
#include "Console.h"
#include "Rendering.h"
#include <cassert>
#include <iostream>

std::unordered_map<std::string, Level> s_levels;

const char* ReadEntireFileAsString(const char* fileName)
{
	FILE* file;
	file = fopen(fileName, "rb");
	fseek(file, 0, SEEK_END);
	const long fileLength = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = new char[fileLength + 1];
	fread(buffer, 1, fileLength, file);
	fclose(file);
	buffer[fileLength] = 0;
	return buffer;
}

std::string ReadEntireFileAsString(const std::string &fileName)
{
	FILE* file;
	file = fopen(fileName.c_str(), "rb");
	if (file != nullptr)
	{
		fseek(file, 0, SEEK_END);
		const long fileLength = ftell(file);
		fseek(file, 0, SEEK_SET);
		std::string buffer;
		buffer.resize(fileLength, {});
		fread(buffer.data(), 1, fileLength, file);
		fclose(file);
		return buffer;
	}
	else
		return "";
}

picojson::value JsonStruct(const std::string& name)
{
	std::string data = ReadEntireFileAsString(name + ".json");
	picojson::value v;
	std::string err = picojson::parse(v, data);
	if (!err.empty())
	{
		ConsoleLog(LogLevel::LogLevel_Error, "Error parsing: %s", name.c_str());
		ConsoleLog(LogLevel::LogLevel_Error, "%s", err.c_str());
	}
	return v;
}

const picojson::value& FindChildObject(const std::string& name, const std::string& type, const picojson::value& parent)
{
	for (int32 i = 0; i < parent.get<picojson::array>().size(); i++)
	{
		const picojson::value& typeVal = parent.get(i).get("type");
		if (typeVal.is<picojson::null>())
			continue;

		if (typeVal.get<std::string>() == type)
		{
			return parent.get(i).get(name);
		}
	}
	static picojson::value result;
	return result;
}

const picojson::value& GetActorPropertyValue(const picojson::value& props, const std::string& propertyName, bool& success)
{
	for (int32 i = 0; i < props.get<picojson::array>().size(); i++)
	{
		if (props.get<picojson::array>()[i].get("name").get<std::string>() == propertyName)
		{
			success = true;
			return props.get<picojson::array>()[i].get("value");
		}
	}
	ConsoleLog("GetActorPropertyValue failed to return a value for %s\n", propertyName.c_str());
	static picojson::value result;
	success = false;
	return result;
}

template<typename T>
T GetActorProperty(const picojson::value& props, const std::string& propertyName)
{
	bool status = true;
	const picojson::value& result = GetActorPropertyValue(props, propertyName, status);
	if (status)
	{
		if (result.is<T>())
		{
			return result.get<T>();
		}
	}
	ConsoleLog("GetActorProperty failed to return a value for %s\n", propertyName.c_str());
	return {};
}

void CreateLevel(Level* level, const std::string& name)
{
	const picojson::value& v = JsonStruct("Assets/Levels/" + name);
	const picojson::value& layers = v.get("layers");

	const picojson::value& chunks = FindChildObject("chunks", "tilelayer", layers);
	for (int32 i = 0; i < chunks.get<picojson::array>().size(); i++)
	{
		const picojson::value& chunk = chunks.get<picojson::array>()[i];
		int32 chunkSize = (int32)chunk.get("height").get<double>();
		VectorInt chunkLoc = { (int32)chunk.get("x").get<double>(), (int32)chunk.get("y").get<double>() };
		chunkLoc.y -= 2 * chunkLoc.y; //invert so -16 is on bottom

		for (int32 j = 0; j < chunk.get("data").get<picojson::array>().size(); j++)
		{
			//picojson::value* test = chunk.get("data").get<picojson::array>();
			if (chunk.get("data").get<picojson::array>()[j].get<double>())
			{
				Vector blockLoc = { float((j % chunkSize) + chunkLoc.x), float(chunkLoc.y - (j / chunkSize)) };
				level->blocks.AddBlock(blockLoc, TileType::filled);
			}
		}
	}



	const picojson::value& portalObjects = FindChildObject("objects", "objectgroup", layers);
	for (int32 i = 0; i < (int32)portalObjects.get<picojson::array>().size(); i++)
	{
		const picojson::value& actorProperties = portalObjects.get<picojson::array>()[i];

		Vector loc = { (float)actorProperties.get("x").get<double>() / blockSize,
					   (float)actorProperties.get("y").get<double>() / blockSize };
		loc.y -= 2 * loc.y;
		const std::string& type = actorProperties.get("type").get<std::string>();
		if (type == "PlayerType")
		{
			// Do Player Stuff

			Player* player = level->CreateActor<Player>();
			player->position = loc;
			int32 a = 0;
		}
		else if (type == "EnemyType")
		{
			// Do Enemy Stuff

			const picojson::value& props = actorProperties.get("properties");
			Enemy* enemy = level->CreateActor<Enemy>();
			enemy->position = loc;
			enemy->damage = (float)GetActorProperty<double>(props, "Damage");
		}
		else if (type == "DummyType")
		{
			// Do Dummy Stuff
			const picojson::value& props = actorProperties.get("properties");

			DummyInfo info;
			info.mimicType = (ActorType)GetActorProperty<double>(props, "ActorType");
			Dummy* dummy = level->CreateActor<Dummy>(info);
			dummy->position = loc;
		}
		else if (type == "PortalType")
		{
			// Do Portal Stuff

			const picojson::value& props = actorProperties.get("properties");

			PortalInfo info;
			info.PortalID = (int32)GetActorProperty<double>(props, "PortalID");
			info.levelName = GetActorProperty<std::string>(props, "PortalPointerLevel");
			info.levelPortalID = (int32)GetActorProperty<double>(props, "PortalPointerID");
			Portal* portal = level->CreateActor<Portal>(info);
			portal->position = loc;

		}
		else if (type == "SpringType")
		{
			// Do Spring Stuff

			Spring* spring = level->CreateActor<Spring>();
			spring->position = loc;
		}
		else if (type == "MovingPlatformType")
		{
			// Do Spring Stuff
			MovingPlatform* MP = level->CreateActor<MovingPlatform>();
			MP->position = loc;
			MP->locations.push_back(loc);

			const picojson::value& props = actorProperties.get("properties");
			if (props.evaluate_as_boolean())
			{
				int32 locationCount = (int32)GetActorProperty<double>(props, "LocationCounts");
				if (locationCount == 0)
					locationCount = 1;

				for (int32 i = 1; i <= locationCount; i++)
				{
					std::string x = "Destination" + std::to_string(i) + "X";
					std::string y = "Destination" + std::to_string(i) + "Y";
					Vector pos = { (float)GetActorProperty<double>(props, x), (float)GetActorProperty<double>(props, y) };
					pos.y -= 2 * pos.y;
					MP->locations.push_back(pos);
				}
				MP->dest = MP->locations[MP->nextLocationIndex];
				Vector directionVector = Normalize(MP->dest - MP->position);
				MP->velocity = directionVector * MP->speed;
			}
		}
	}
	level->blocks.UpdateAllBlocks();
}

void AddAllLevels()
{
	std::string names[] = { "Default", "Level 1" };
	for (std::string name : names)
	{
		CreateLevel(&s_levels[name], name);
	}
}

void LoadLevel(Level* level, const std::string& name)
{
	if (s_levels.find(name) != s_levels.end())
	{

		*level = s_levels[name];
		level->movingPlatforms.clear();
		level->actors.clear();
		level->playerID = ActorID::Invalid;
		for (Actor* actor : s_levels[name].actors)
		{
			Actor* actorCopy = actor->Copy();
			if (actorCopy->GetActorType() == ActorType::MovingPlatform)
				level->movingPlatforms.push_back(actorCopy->id);
			else if (actorCopy->GetActorType() == ActorType::Player)
				level->playerID = actorCopy->id;
			level->AddActor(actorCopy);
		}

		return;
	}
	CreateLevel(level, name);
}

void LoadFonts()
{
    picojson::value metadata = JsonStruct("Assets/Fonts/Metadata");

	for (int32 i = 0; i < metadata.get<picojson::array>().size(); i++)
	{
		picojson::object obj = metadata.get<picojson::array>()[i].get<picojson::object>();
		const char* name = obj["name"].get<std::string>().c_str();

		g_fonts[name] = CreateFont(	name,
									static_cast<int32>(obj["charSize"].get<double>()),
									static_cast<int32>(obj["actualCharWidth"].get<double>()),
									static_cast<int32>(obj["charPerRow"].get<double>()));
		if (g_fonts[name] == nullptr)
		{
			ConsoleLog(LogLevel::LogLevel_Warning, "Failed to load font: %s", name);
			assert(false);
		}

	}
}


AnimationData GetAnimationData(const std::string& name, std::string* states)
{

    picojson::value v = JsonStruct("Assets/Actor_Art/" + name + "/Metadata");
	if (v.is<picojson::null>())
		return {};

	picojson::object obj = v.get<picojson::object>();

	//name
	{
		const char* string = "name";
		if (obj.contains(string))
		{
			assert(obj[string].get<std::string>() == name);
			AnimationData result = {
				.name = name.c_str()
			};
		}
		else
			assert(false);
	}
	AnimationData result = {
		.name = name.c_str()
	};

	//animationFPS
	{
		const char* string = "animationFPS";
		if (obj.contains(string))
		{

			picojson::object animationFPS = obj[string].get<picojson::object>();
			for (int32 i = 0; i < int32(ActorState::Count); i++)
			{
				if(animationFPS.contains(states[i]))
					result.animationFPS[i] = static_cast<float>(animationFPS[states[i]].get<double>());
			}
		}
	}

	//collisionOffset
	{
		const char* xString = "collisionOffsetX";
		const char* yString = "collisionOffsetY";
		if (obj.contains(xString))
			result.collisionOffset.x = static_cast<float>(obj[xString].get<double>());
		if (obj.contains(yString))
			result.collisionOffset.y = static_cast<float>(obj[yString].get<double>());
	}

	//collisionRectangle
	{
		const char* string = "collisionRectangle";
		if (obj.contains(string))
		{
			picojson::object colRect = obj[string].get<picojson::object>();
			if (colRect.contains("left"))
				result.collisionRectangle.botLeft.x = static_cast<float>(colRect["left"].get<double>());
			if (colRect.contains("bot"))
				result.collisionRectangle.botLeft.y = static_cast<float>(colRect["bot"].get<double>());
			if (colRect.contains("right"))
				result.collisionRectangle.topRight.x = static_cast<float>(colRect["right"].get<double>());
			if (colRect.contains("top"))
				result.collisionRectangle.topRight.y = static_cast<float>(colRect["top"].get<double>());
		}
	}

	//scaledWidth
	{
		const char* string = "scaledWidth";
		if (obj.contains(string))
		{
			float f = static_cast<float>(obj[string].get<double>());
			if (f == 0)
				result.scaledWidth = inf;
			else
				result.scaledWidth = f;
		}
	}

	return result;
}

 void LoadAudioFiles(std::vector<AudioFileMetaData>* result)
{
    picojson::value metadata = JsonStruct("Assets/Audio/Metadata");

	auto test = metadata.get<picojson::object>()["AudioData"].get<picojson::array>();
	picojson::array ar = metadata.get<picojson::object>()["AudioData"].get<picojson::array>();

	for (int32 i = 0; i < ar.size(); i++)
	{
		picojson::object obj = ar[i].get<picojson::object>();

		AudioFileMetaData afmd = {};
		afmd.name = obj["Name"].get<std::string>().c_str();
		const std::string& type = obj["VolumeType"].get<std::string>();

        afmd.volumeType = Volume::None;
        if (type == "Effect")
            afmd.volumeType = Volume::Effect;
        else if (type == "Music")
            afmd.volumeType = Volume::Music;
        else
            FAIL;

		result->push_back(afmd);
	}
}

