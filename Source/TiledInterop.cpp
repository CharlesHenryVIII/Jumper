#include "JSON/picojson.h"
#include "WinUtilities.h"
#include "Math.h"

#include <iostream>
/*
   |_______|
           ^

****possible options
-Create a function for loading hte chunks that gets ran when an object finds that the objects name is "chunks"
-Do the same thing as above just inline^
-Create a "find" function that will take a string argument for the name it needs to look for and return a picojson::value at that location
-Create a struct that holds all the data wanted, and have the recurse function fill it manaually by checking the names of objects.


*/



struct Chunk
{
	Chunk(int32 size)
	{
		blockSize = size;
	}
	int32 blockSize;
	VectorInt loc = {};
	uint32* data = new uint32[blockSize * blockSize];
};



//void nothings()
//{
//	FILE* file = fopen("file.txt", "rb");
//	//FILE* file;
//	//if (!fopen_s(&file, "file.txt", "rb"))
//	//	return;
//
//	char buffer[256];
//	size_t amount_read = fread(buffer, 1, sizeof(buffer), file);
//
//	// Where you are at in the file
//
//    //#define SEEK_CUR    1
//    //#define SEEK_END    2
//    //#define SEEK_SET    0
//	fseek(file, 0, SEEK_END);
//	long filesize = ftell(file);
//	fseek(file, 0, SEEK_SET);
//}


void LoadBlocks(Chunk chunks[])
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
std::vector<Chunk*> chunks;

void Recurse(const picojson::value& value)
{

	if (value.is<picojson::null>())
		return;
	std::string tabs;
	for (int i = 0; i < depth; i++)
		tabs += "\t";

	if (value.is<bool>())
		value.get<bool>() ? DebugPrint("true \n") : DebugPrint("false \n");
	else if (value.is<double>())
		DebugPrint("%s \n", std::to_string(value.get<double>()).c_str());
	else if (value.is<std::string>())
		DebugPrint("%s \n", value.get<std::string>().c_str());
	else if (value.is<picojson::array>())
	{
		for (int i = 0; i < value.get<picojson::array>().size(); i++)
		{
			depth++;
			Recurse(value.get<picojson::array>()[i]);
			depth--;
		}
	}
	else if (value.is<picojson::object>())
	{
		for (auto& item : value.get<picojson::object>())
		{
			if (item.first == "chunks") //chunks
			{
				for (int i = 0; i < item.second.get<picojson::array>().size(); i++) //array
				{
					int32 size = 0;
					VectorInt loc = {};
					std::vector<uint32> data;
					for (auto& info : item.second.get<picojson::array>()[i].get<picojson::object>()) //objects
					{
						if (info.first == "height")
							size = (int)info.second.get<double>();
						else if (info.first == "x")
							loc.x = info.second.get<double>();
						else if (info.first == "y")
							loc.y = info.second.get<double>();
						else if (info.first == "data")
						{
							for (int32 j = 0; j < info.second.get<picojson::array>().size(); j++)  //array
							{
									data.push_back(uint32(info.second.get<picojson::array>()[j].get<double>()));
							}
						}
					}
					Chunk* chunk = new Chunk(size);
					chunk->loc = loc;
					chunk->data = data.data();
					chunks.push_back(chunk);
				}
			}
			depth++;
			Recurse(item.second);
			depth--;
		}
	}
}


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


int JsonStruct()
{
	std::string data = ReadEntireFileAsString("Default.json");
	//DebugPrint(data.c_str());
	picojson::value v;
	std::string err = picojson::parse(v, data);
	if (!err.empty()) 
	{
		std::cerr << err << std::endl;
	}
	
	
	Recurse(v);


	return 0;
}
static int test = JsonStruct();