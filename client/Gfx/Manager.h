#pragma once

#include <memory>
#include <string>

namespace Gfx
{
class CImage;
class CAnimation;

typedef std::shared_ptr<CImage> PImage;
typedef std::shared_ptr<CAnimation> PAnimation;

class CManager
{
public:
	static PImage getImage(const std::string& fname);
	static PAnimation getAnimation(const std::string& fname);
};

}
