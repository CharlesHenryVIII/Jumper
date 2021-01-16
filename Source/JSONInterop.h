#pragma once

struct FontSprite;
struct AnimationData;

void AddAllLevels();
void LoadLevel(Level* level, const std::string& name);
void LoadFonts();
AnimationData GetAnimationData(const std::string& name, std::string* states);
