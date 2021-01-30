#pragma once
#include "Misc.h"

struct FontSprite;
struct AnimationData;
struct Level;
struct AudioFileMetaData;

void AddAllLevels();
void LoadLevel(Level* level, const std::string& name);
void LoadFonts();
AnimationData GetAnimationData(const std::string& name, std::string* states);
void LoadAudioFiles(std::vector<AudioFileMetaData>* result);
