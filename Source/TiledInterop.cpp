#include "JSON/picojson.h"
#include "WinUtilities.h"
#include "Math.h"
#include "Entity.h"

#include <cassert>
#include <iostream>


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
	//DebugPrint(data.c_str());
	picojson::value v;
	std::string err = picojson::parse(v, data);
	if (!err.empty())
	{
		std::cerr << err << std::endl;
	}
	return v;
}

const picojson::value& FindChildObject(const std::string& name, const std::string& type, const picojson::value& parent)
{
	for (int i = 0; i < parent.get<picojson::array>().size(); i++)
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

const picojson::value& GetActorPropertyValue(const picojson::value& props, const std::string& propertyName)
{
	for (int32 i = 0; i < props.get<picojson::array>().size(); i++)
	{
		if (props.get<picojson::array>()[i].get("name").get<std::string>() == propertyName)
		{
			return props.get<picojson::array>()[i].get("value");
		}
	}
	assert(false);
	DebugPrint("GetActorPropertyValue failed to return a value for %s\n", propertyName.c_str());
	static picojson::value result;
	return result;
}

template<typename T>
T GetActorProperty(const picojson::value& props, const std::string& propertyName)
{
	const picojson::value& result = GetActorPropertyValue(props, propertyName);
	if (result.is<T>())
	{
		return result.get<T>();
	}

	assert(false);
	DebugPrint("GetActorProperty failed to return a value for %s\n", propertyName.c_str());
	return {};
}

Level* LoadLevel(const std::string& name)
{
	const picojson::value& v = JsonStruct(name);

	Level* level = &levels[name];
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
			Actor* player = FindActor(CreatePlayer(*level), *level);
			player->position = loc;
			int a = 0;
		}
		else if (type == "EnemyType")
		{
			// Do Enemy Stuff
			const picojson::value& props = actorProperties.get("properties");
			Actor* enemy = FindActor(CreateEnemy(*level), *level);
			enemy->position = loc;
			enemy->damage = (float)GetActorProperty<double>(props, "Damage");
		}
		else if (type == "DummyType")
		{
			// Do Dummy Stuff
			const picojson::value& props = actorProperties.get("properties");
			ActorType mimic = (ActorType)GetActorProperty<double>(props, "ActorType");
			Actor* dummy = FindActor(CreateDummy(mimic, *level), *level);
			dummy->position = loc;
		}
		else if (type == "PortalType")
		{
			
			const picojson::value& props = actorProperties.get("properties");
			Portal* portal = CreatePortal((int32)GetActorProperty<double>(props, "PortalID"), 
												 GetActorProperty<std::string>(props, "PortalPointerLevel"), 
										  (int32)GetActorProperty<double>(props, "PortalPointerID"),
												*level);
			portal->position = loc;

		}
	}
	level->blocks.UpdateAllBlocks();
	return level;
}

Level* GetLevel(const std::string& name)
{
	if (levels.find(name) != levels.end())
	{
		return &levels[name];
	}
	return LoadLevel(name);
}
