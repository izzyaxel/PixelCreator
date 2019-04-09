#include "globals.hh"

SP<Image> canvas;

namespace Info
{
	uint32_t majorVersion = 1, minorVersion = 0, bugfixVersion = 0;
	std::string windowName = "PixelCreator | " + std::to_string(majorVersion) + "." + std::to_string(minorVersion) + "." + std::to_string(bugfixVersion);
	bool exiting = false, screenshotQueued = false;
	std::string screenshotDir = "";
}

namespace Mouse
{
	bool lmbDown = false, rmbDown = false, mmbDown = false;
	bool lmbDownDrag = false, rmbDownDrag = false, mmbDownDrag = false;
	IR::vec2<int32_t> pos{0}, prevPos{0}, lastClickPos{0}, lastReleasePos{0};
}

namespace Keyboard
{
	bool spaceDown = false;
}

namespace Camera
{
	IR::vec2<int32_t> pos{0};
}

namespace Context
{
	uint32_t width = 0, height = 0;
}

namespace State
{
	Color curColor, eraserColor, transparencyGridColor1, transparencyGridColor2;
	uint8_t gridSize = 4;
	Tools tool = Tools::BRUSH;
}

namespace Assets
{
	UP<Shader> objectShader = nullptr;
	UP<Mesh> centeredQuadMesh = nullptr;
}
