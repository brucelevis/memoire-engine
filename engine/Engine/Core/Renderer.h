#pragma once
#include "glm/fwd.hpp"
#include "SDL_stdinc.h"
#include "glm/vec4.hpp"
#include <vector>

struct SDL_Color;
struct SDL_Rect;
struct SDL_Point;
struct AseSprite;
struct AseFrame;
class Timer;
class Camera;

namespace bgfx {
	struct DynamicVertexBufferHandle;
	struct TransientVertexBuffer;
	struct IndexBufferHandle;
	struct VertexLayout;
	struct VertexLayoutHandle;
	struct ProgramHandle;
}

class Renderer
{
public:
	static void init();
	static void quit();
	static void tick();
	//Draw Line 
	static void renderLine(glm::vec2 a, glm::vec2 b, glm::vec4& color = glm::vec4{255, 0, 0, 255}, float width = 1.0f);
	static void renderLines(glm::vec2* points, int pointsCount, glm::vec4& color);

	//Draw Square
	static void renderSquare(SDL_Rect& rect, SDL_Color& color);

	//Draw Circle
	//static void renderCircle(glm::vec2& pos, float& radius);

	static void renderAseFrame(int x, int y, AseFrame* frame);
	static void renderAseSprite(int x, int y, AseSprite* ase);

	static void setCamera(Camera* cam);
	static Camera* getCamera();

	static void render();

	//Create Camera class to store projections and to transform mouse pos to world pos
private:
	static Renderer* _instance;
	static Camera* _camera;
	static Timer _fpsTimer;
	static Timer _capTimer;
	static Uint64 _frameCount;
	static bgfx::DynamicVertexBufferHandle lineVerts;
	static bgfx::IndexBufferHandle lineIndicies;
	static bgfx::ProgramHandle lineProgram;
	static std::vector<bgfx::TransientVertexBuffer> _tvbs;
	//static LineVertex verts[];
};