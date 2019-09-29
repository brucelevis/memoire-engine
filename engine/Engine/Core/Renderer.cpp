#include "Engine/Core/Renderer.h"
#include "glm/vec2.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "SDL.h"
#include "bgfx/bgfx.h"
#include "bx/bx.h"
#include "bx/math.h"
#include "Engine/Data/SpriteData.h"
#include "Engine/Data/AseData.h"
#include "Engine/Core/Context.h"
#include "Engine/Utilities/Timer.h"
#include "FileUtility.h"

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif


glm::vec2* Renderer::_cameraPos = DBG_NEW glm::vec2();
Renderer* Renderer ::_instance = nullptr;
Timer Renderer::_fpsTimer = Timer();
Timer Renderer::_capTimer = Timer();
Uint64 Renderer::_frameCount = 0;

bgfx::DynamicVertexBufferHandle Renderer::lineVerts;
bgfx::IndexBufferHandle Renderer::lineIndicies;
bgfx::ProgramHandle Renderer::lineProgram;
bgfx::VertexLayout LineVertex::layout;
bgfx::VertexLayoutHandle LineVertex::handle;
std::vector<glm::vec2> Renderer::_linePoints;

const unsigned short Renderer::planeIndexList[] = {
	0,1,2,
	0,2,3
};

void LineVertex::init()
{
	layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.end();
	handle = bgfx::createVertexLayout(layout);
}

void Renderer::init()
{
	if (_instance == nullptr) {
		_instance = DBG_NEW Renderer();
		_fpsTimer.start();
		LineVertex::init();
		lineVerts = bgfx::createDynamicVertexBuffer(4, LineVertex::layout, BGFX_BUFFER_ALLOW_RESIZE);
		lineIndicies = bgfx::createIndexBuffer(bgfx::makeRef(&planeIndexList, sizeof(planeIndexList)));
		lineProgram = FileUtility::loadProgram("assets/shaders/vs_line.bin", "assets/shaders/fs_line.bin");
	}
}

void Renderer::quit()
{
	if (_instance != nullptr) {
		bgfx::destroy(lineVerts);
		bgfx::destroy(lineIndicies);
		_fpsTimer.stop();
		delete(_instance);
		delete(_cameraPos);
	}
}

void Renderer::tick()
{
	_capTimer.start();
	_linePoints.clear();
}
void Renderer::renderLine(glm::vec2 a, glm::vec2 b, SDL_Color& color) {

}

void Renderer::renderLine(glm::vec2 a, glm::vec2 b, float width)
{
	//SDL_SetRenderDrawColor(Context::getRenderer(), color.r, color.g, color.b, color.a);
	//SDL_RenderDrawLine(Context::getRenderer(), a.x - _cameraPos->x, a.y - _cameraPos->y, b.x - _cameraPos->x, b.y - _cameraPos->y);

	glm::vec2 dir = glm::normalize(b - a) * width;
	glm::vec2 right = glm::rotate(dir, glm::radians(-90.0f));
	glm::vec2 left = glm::rotate(dir, glm::radians(90.0f));
	//Get Verts
	_linePoints.push_back(a + right);
	_linePoints.push_back(b + right);
	_linePoints.push_back(b + left);
	_linePoints.push_back(a + left);

	//verts[0] = {ar.x,ar.y, 0.f, 0xff0000ff };
	//verts[1] = { br.x, br.y, 0.f, 0xff0000ff };
	//verts[2] = { bl.x, bl.y, 0.f, 0xff0000ff };
	//verts[3] = { al.x, al.y, 0.f, 0xff0000ff };

	//SDL_Log("Help, %f", verts[0].x);
}

void Renderer::renderLines(SDL_Point* points, int pointsCount, SDL_Color& color)
{
	for (size_t i = 0; i < pointsCount; i++)
	{
		points[i].x -= _cameraPos->x;
		points[i].y -= _cameraPos->y;
	}
	//SDL_SetRenderDrawColor(Context::getRenderer(), color.r, color.g, color.b, color.a);
	//SDL_RenderDrawLines(Context::getRenderer(), points, pointsCount);
}

void Renderer::renderSquare(SDL_Rect& rect, SDL_Color& color)
{
	rect.x -= _cameraPos->x;
	rect.y -= _cameraPos->y;
	//SDL_SetRenderDrawColor(Context::getRenderer(), color.r, color.g, color.b, color.a);
	//SDL_RenderFillRect(Context::getRenderer(), &rect);
}

void Renderer::renderAseFrame(int x, int y, AseFrame* frame)
{
	for (size_t i = 0; i < frame->sprites.size(); i++)
	{
		renderAseSprite(x, y, &frame->sprites[i]);
	}
}

void Renderer::renderAseSprite(int x, int y, AseSprite* ase)
{
	SDL_Rect renderQuad = {(x + ase->xPos)  - _cameraPos->x, (y + ase->yPos) - _cameraPos->y, ase->width, ase->height };
	//SDL_SetRenderDrawColor(Context::getRenderer(), 0xFF, 0xFF, 0xFF, 0xFF);
	//SDL_RenderCopy(Context::getRenderer(), ase->texture, nullptr, &renderQuad);
}

void Renderer::renderSprite(glm::vec2& pos, SpriteData* spriteData)
{
	renderSprite(pos.x, pos.y, spriteData);
}

void Renderer::renderSprite(int x, int y, SpriteData* spriteData)
{
	SDL_Rect renderQuad = { x - _cameraPos->x, y - _cameraPos->y, spriteData->width, spriteData->height };
	//SDL_SetRenderDrawColor(Context::getRenderer(), 0xFF, 0xFF, 0xFF, 0xFF);
	//SDL_RenderCopy(Context::getRenderer(), spriteData->texture, nullptr, &renderQuad);
}

void Renderer::clearRenderer(SDL_Color& color)
{
	clearRenderer(color.r, color.g, color.b, color.a);
}

void Renderer::clearRenderer(int r, int g, int b, int a)
{
	//SDL_SetRenderDrawColor(Context::getRenderer(), r, g, b, a);
	//SDL_RenderClear(Context::getRenderer());
}

void Renderer::render()
{
	//bgfx::update(lineIndicies, 0, bgfx::makeRef(&LineVertex::layout, sizeof(LineVertex::layout)));
	std::vector<LineVertex> verts;

	for (size_t i = 0; i < _linePoints.size(); i++)
	{
		verts.push_back({ _linePoints[i].x, _linePoints[i].y, 0, 0xff0000ff });
	}

	bgfx::update(lineVerts, 0, bgfx::makeRef( verts.data(), verts.size() * sizeof(LineVertex)));
	bgfx::setVertexBuffer(0, lineVerts);
	bgfx::setIndexBuffer(lineIndicies);
	bgfx::submit(0, lineProgram);


	bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
	bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };
	float view[16];
	bx::mtxLookAt(view, eye, at);
	float proj[16];
	bx::mtxProj(proj, 60.0f, float(Context::getWindowWidth()) / float(Context::getWindowHeight()), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
	bgfx::setViewTransform(0, view, proj);

	//SDL_Log("FPS: %f", _frameCount / ( _fpsTimer.getTicks() / 1000.f));

	char title[80] = {};

	strcpy(title, Context::getWindowTitle());
	strcat(title, "FPS: ");
	//strcat(title, (_frameCount / (_fpsTimer.getTicks() / 1000.f)));
		//+  + (_frameCount / (_fpsTimer.getTicks() / 1000.f));

	Context::setWindowTitle(title);

	const bgfx::Stats* stats = bgfx::getStats();
	bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
		, stats->width
		, stats->height
		, stats->textWidth
		, stats->textHeight
	);

	bgfx::frame();

	//SDL_RenderPresent(Context::getRenderer());
	_frameCount++;

	//if frame finished early
	Uint32 frameTicks = _capTimer.getTicks();
	if (frameTicks < (1000 / Context::getMaxFps())) {
		SDL_Delay((1000 / Context::getMaxFps()) - frameTicks);
	}
}

void Renderer::setCameraPos(int x, int y)
{
	_cameraPos->x = x; //- (Context::getWindowWidth()/2);
	_cameraPos->y = y; //- (Context::getWindowHeight()/2);
}

glm::vec2 Renderer::getCameraPos()
{
	return *_cameraPos;
}
