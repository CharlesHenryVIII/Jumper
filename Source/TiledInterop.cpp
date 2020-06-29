#include "JSON/picojson.h"
#include "WinUtilities.h"
#include "Math.h"
#include "Entity.h"

#include <cassert>
#include <iostream>


void AddBlocksToTileMap(Level* level, picojson::value chunks)
{

}



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

int depth = 0;

//void Recurse(const picojson::value& value)
//{
//
//	if (value.is<picojson::null>())
//		return;
//	std::string tabs;
//	for (int i = 0; i < depth; i++)
//		tabs += "\t";
//
//	if (value.is<bool>())
//		value.get<bool>() ? DebugPrint("true \n") : DebugPrint("false \n");
//	else if (value.is<double>())
//		DebugPrint("%s \n", std::to_string(value.get<double>()).c_str());
//	else if (value.is<std::string>())
//		DebugPrint("%s \n", value.get<std::string>().c_str());
//	else if (value.is<picojson::array>())
//	{
//		for (int i = 0; i < value.get<picojson::array>().size(); i++)
//		{
//			depth++;
//			Recurse(value.get<picojson::array>()[i]);
//			depth--;
//		}
//	}
//	else if (value.is<picojson::object>())
//	{
//		for (auto& item : value.get<picojson::object>())
//		{
//			if (item.first == "chunks") //chunks
//			{
//				for (int i = 0; i < item.second.get<picojson::array>().size(); i++) //array
//				{
//					int32 size = 0;
//					VectorInt loc = {};
//					std::vector<uint32> data;
//					for (auto& info : item.second.get<picojson::array>()[i].get<picojson::object>()) //objects
//					{
//						if (info.first == "height")
//							size = (int)info.second.get<double>();
//						else if (info.first == "x")
//							loc.x = info.second.get<double>();
//						else if (info.first == "y")
//							loc.y = info.second.get<double>();
//						else if (info.first == "data")
//						{
//							for (int32 j = 0; j < info.second.get<picojson::array>().size(); j++)  //array
//							{
//								data.push_back(uint32(info.second.get<picojson::array>()[j].get<double>()));
//							}
//						}
//					}								
//					Chunk* chunk = new Chunk(size);
//					chunk->loc = loc;
//					assert((int32)data.size() == (size * size));
//
//					chunk->data = data.data();
//					chunks.push_back(chunk);
//				}
//			}
//			depth++;
//			Recurse(item.second);
//			depth--;
//		}
//	}
//}


//void Recurse(const picojson::value &value)
//{
//
//	if (value.is<picojson::null>())
//		return;
//	std::string tabs;
//	for (int i = 0; i < depth; i++)
//		tabs += "\t";
//
//	if (value.is<bool>())
//		value.get<bool>() ? DebugPrint("true \n") : DebugPrint("false \n");
//	else if (value.is<double>())
//		DebugPrint("%s \n", std::to_string(value.get<double>()).c_str());
//	else if (value.is<std::string>())
//		DebugPrint("%s \n",  value.get<std::string>().c_str());
//	else if (value.is<picojson::array>())
//	{
//		for (int i = 0; i < value.get<picojson::array>().size(); i++)
//		{
//			DebugPrint("\n");
//			depth++;
//			Recurse(value.get<picojson::array>()[i]);
//			depth--;
//		}
//	}
//	else if (value.is<picojson::object>())
//	{
//		for (auto& item : value.get<picojson::object>())
//		{
//			//DebugPrint()
//			DebugPrint("%s%s: ", tabs.c_str(), item.first.c_str());
//			depth++;
//			Recurse(item.second);
//			depth--;
//		}
//	}
//}

//"objects", "objectGroup", layers);

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

const picojson::value& FindChildObject(std::string name, std::string type, const picojson::value& parent)
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

double GetActorPropertyValue(const picojson::value& props, std::string propertyName)
{
	ActorType mimic = ActorType::none;
	for (int32 j = 0; j < props.get<picojson::array>().size(); j++)
	{
		if (props.get<picojson::array>()[j].get("name").get<std::string>() == propertyName)
		{
			props.get<picojson::array>()[j].get("type").get<std::string>();
			return props.get<picojson::array>()[j].get("value").get<double>();
		}
		else
			return 0;
	}
	return 0;
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


	const picojson::value& objects = FindChildObject("objects", "objectgroup", layers);
	int32 size = (int32)objects.get<picojson::array>().size();
	for (int32 i = 0; i < objects.get<picojson::array>().size(); i++)
	{
		const picojson::value& actorProperties = objects.get<picojson::array>()[i];

		Vector loc = { (float)actorProperties.get("x").get<double>() / blockSize,
					   (float)actorProperties.get("y").get<double>() / blockSize };
		loc.y -= 2 * loc.y;
		const std::string& type = actorProperties.get("type").get<std::string>();
		if (type == "PlayerType")
		{
			// Do Player Stuff
			Actor* player = FindActor(CreateActor(ActorType::player, ActorType::none, &level->actors), &level->actors);
			player->position = loc;
			int a = 0;
		}
		else if (type == "EnemyType")
		{
			// Do Enemy Stuff
			const picojson::value& props = actorProperties.get("properties");
			Actor* enemy = FindActor(CreateActor(ActorType::enemy, ActorType::none, &level->actors), &level->actors);
			enemy->position = loc;
			enemy->damage = (float)GetActorPropertyValue(props, "Damage");
		}
		else if (type == "DummyType")
		{
			// Do Dummy Stuff
			const picojson::value& props = actorProperties.get("properties");
			ActorType mimic = (ActorType)GetActorPropertyValue(props, "ActorType");
			Actor* dummy = FindActor(CreateActor(ActorType::enemy, mimic, &level->actors), &level->actors);
			dummy->position = loc;
			int32 a = 0;
		}
	}
	return level;
}

Level* GetLevel(const std::string& name)
{
	for (auto& level : levels)
	{
		if (level.first == name)
		{
			return &level.second;
		}
	}
	return LoadLevel(name);
}
