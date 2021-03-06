#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_syswm.h"

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "bimg/bimg.h"
#include <bimg/decode.h>
#include "bx/bx.h"
#include <bx/file.h>
#include "bx/readerwriter.h"
#include "bx/string.h"
#include "bx/allocator.h"
#include <iostream>


#define AC_SCREEN_WIDTH 1280
#define AC_SCREEN_HEIGHT 720

bx::AllocatorI* getDefaultAllocator() {
	BX_PRAGMA_DIAGNOSTIC_PUSH();
	BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4459); // warning C4459: declaration of 's_allocator' hides global declaration
	BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
	static bx::DefaultAllocator s_allocator;
	return &s_allocator;
	BX_PRAGMA_DIAGNOSTIC_POP();
}

bx::AllocatorI* g_allocator = getDefaultAllocator();

bx::AllocatorI* getAllocator()
{
	if (NULL == g_allocator)
	{
		g_allocator = getDefaultAllocator();
	}

	return g_allocator;
}


static bx::FileReaderI* s_fileReader = NULL;

typedef bx::StringT<&g_allocator> String;
static String s_currentDir;

class FileReader : public bx::FileReader
{
	typedef bx::FileReader super;

public:
	virtual bool open(const bx::FilePath& _filePath, bx::Error* _err) override
	{
		s_currentDir.set("");
		String filePath(s_currentDir);
		filePath.append(_filePath);
		return super::open(filePath.getPtr(), _err);
	}
};

void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size)
{
	if (bx::open(_reader, _filePath))
	{
		uint32_t size = (uint32_t)bx::getSize(_reader);
		void* data = BX_ALLOC(_allocator, size);
		bx::read(_reader, data, size);
		bx::close(_reader);
		if (NULL != _size)
		{
			*_size = size;
		}
		return data;
	}
	else
	{
		//DBG("Failed to open: %s.", _filePath);
	}

	if (NULL != _size)
	{
		*_size = 0;
	}

	return NULL;
}

void unload(void* _ptr)
{
	BX_FREE(getAllocator(), _ptr);
}

static void imageReleaseCb(void* _ptr, void* _userData)
{
	BX_UNUSED(_ptr);
	bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
	bimg::imageFree(imageContainer);
}

bgfx::TextureHandle loadTexture(bx::FileReaderI* _reader, const char* _filePath, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
{
	BX_UNUSED(_skip);
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

	uint32_t size;
	void* data = load(_reader, getAllocator(), _filePath, &size);
	if (NULL != data)
	{
		bimg::ImageContainer* imageContainer = bimg::imageParse(getAllocator(), data, size);

		if (NULL != imageContainer)
		{
			if (NULL != _orientation)
			{
				*_orientation = imageContainer->m_orientation;
			}

			const bgfx::Memory* mem = bgfx::makeRef(
				imageContainer->m_data
				, imageContainer->m_size
				, imageReleaseCb
				, imageContainer
			);
			unload(data);

			if (imageContainer->m_cubeMap)
			{
				handle = bgfx::createTextureCube(
					uint16_t(imageContainer->m_width)
					, 1 < imageContainer->m_numMips
					, imageContainer->m_numLayers
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
					, _flags
					, mem
				);
			}
			else if (1 < imageContainer->m_depth)
			{
				handle = bgfx::createTexture3D(
					uint16_t(imageContainer->m_width)
					, uint16_t(imageContainer->m_height)
					, uint16_t(imageContainer->m_depth)
					, 1 < imageContainer->m_numMips
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
					, _flags
					, mem
				);
			}
			else if (bgfx::isTextureValid(0, false, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags))
			{
				handle = bgfx::createTexture2D(
					uint16_t(imageContainer->m_width)
					, uint16_t(imageContainer->m_height)
					, 1 < imageContainer->m_numMips
					, imageContainer->m_numLayers
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
					, _flags
					, mem
				);
			}

			if (bgfx::isValid(handle))
			{
				bgfx::setName(handle, _filePath);
			}

			if (NULL != _info)
			{
				bgfx::calcTextureSize(
					*_info
					, uint16_t(imageContainer->m_width)
					, uint16_t(imageContainer->m_height)
					, uint16_t(imageContainer->m_depth)
					, imageContainer->m_cubeMap
					, 1 < imageContainer->m_numMips
					, imageContainer->m_numLayers
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
				);
			}
		}
	}

	return handle;
}

struct PosColorVertex
{
	float x;
	float y;
	float z;
	uint32_t abgr;
	int16_t m_u;
	int16_t m_v;
	static bgfx::VertexLayout pcvLayout;
	static void init() {
		pcvLayout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Int16, true, true)
			.end();
	}
};

bgfx::VertexLayout PosColorVertex::pcvLayout;

static PosColorVertex planeVerts[] = {
	{-1.0f, -1.0f, 0.0f, 0xffffffff, 0, 0x7fff},
	{1.0f, -1.0f, 0.0f, 0xffffffff, 0x7fff, 0x7fff},
	{1.0f, 1.0f, 0.0f, 0xffffffff, 0x7fff, 0},
	{-1.0f, 1.0f, 0.0f, 0xffffffff, 0, 0}
};

static const uint16_t planeTriList[] = {
	0,1,2,
	0,2,3
};

bgfx::ShaderHandle loadShader(const char* FILENAME)
{
	//const char* shaderPath = "???";

	//switch (bgfx::getRendererType()) {
	//case bgfx::RendererType::Noop:
	//case bgfx::RendererType::Direct3D9:  shaderPath = "shaders/dx9/";   break;
	//case bgfx::RendererType::Direct3D11:
	//case bgfx::RendererType::Direct3D12: shaderPath = "shaders/dx11/";  break;
	//case bgfx::RendererType::Gnm:        shaderPath = "shaders/pssl/";  break;
	//case bgfx::RendererType::Metal:      shaderPath = "shaders/metal/"; break;
	//case bgfx::RendererType::OpenGL:     shaderPath = "shaders/glsl/";  break;
	//case bgfx::RendererType::OpenGLES:   shaderPath = "shaders/essl/";  break;
	//case bgfx::RendererType::Vulkan:     shaderPath = "shaders/spirv/"; break;
	//}

	//size_t shaderLen = strlen(shaderPath);
	//size_t fileLen = strlen(FILENAME);
	//char* filePath = (char*)malloc(shaderLen + fileLen);
	//memcpy(filePath, shaderPath, shaderLen);
	//memcpy(&filePath[shaderLen], FILENAME, fileLen);

	FILE* file = fopen(FILENAME, "rb");
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	const bgfx::Memory* mem = bgfx::alloc(fileSize + 1);
	fread(mem->data, 1, fileSize, file);
	mem->data[mem->size - 1] = '\0';
	fclose(file);

	return bgfx::createShader(mem);
}

bx::FileReaderI* getFileReader()
{
	return s_fileReader;
}

int main() {

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return EXIT_FAILURE;
	}
	SDL_Window* win = SDL_CreateWindow("test_bgfx", 100, 100, AC_SCREEN_WIDTH, AC_SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (win == nullptr) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		return EXIT_FAILURE;
	}

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(win, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	//SDL_GetWindowWMInfo(win, &wmInfo);

	if (SDL_GetWindowWMInfo(win, &wmInfo)) { /* the call returns true on success */
  /* success */
		const char* subsystem = "an unknown system!";
		switch (wmInfo.subsystem) {
		case SDL_SYSWM_UNKNOWN:   break;
		case SDL_SYSWM_WINDOWS:   subsystem = "Microsoft Windows(TM)";  break;
		case SDL_SYSWM_X11:       subsystem = "X Window System";        break;
#if SDL_VERSION_ATLEAST(2, 0, 3)
		case SDL_SYSWM_WINRT:     subsystem = "WinRT";                  break;
#endif
		case SDL_SYSWM_DIRECTFB:  subsystem = "DirectFB";               break;
		case SDL_SYSWM_COCOA:     subsystem = "Apple OS X";             break;
		case SDL_SYSWM_UIKIT:     subsystem = "UIKit";                  break;
#if SDL_VERSION_ATLEAST(2, 0, 2)
		case SDL_SYSWM_WAYLAND:   subsystem = "Wayland";                break;
		case SDL_SYSWM_MIR:       subsystem = "Mir";                    break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
		case SDL_SYSWM_ANDROID:   subsystem = "Android";                break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
		case SDL_SYSWM_VIVANTE:   subsystem = "Vivante";                break;
#endif
		}

		SDL_Log("This program is running SDL version %d.%d.%d on %s",
			(int)wmInfo.version.major,
			(int)wmInfo.version.minor,
			(int)wmInfo.version.patch,
			subsystem);
	}
	else {
		/* call failed */
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get window information: %s", SDL_GetError());
	}



	bgfx::PlatformData pd;
	pd.ndt = NULL;
	pd.nwh = wmInfo.info.win.window;
	bgfx::setPlatformData(pd);

	bgfx::Init init;
	//init.platformData = pd;
	init.type = bgfx::RendererType::OpenGL;
	init.resolution.height = AC_SCREEN_HEIGHT;
	init.resolution.width = AC_SCREEN_WIDTH;
	init.resolution.reset = BGFX_RESET_VSYNC;
	bgfx::init(init);

	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);

	// Enable debug text.
	bgfx::setDebug(BGFX_DEBUG_TEXT);

	//bgfx::TextureFormat();
	s_fileReader = BX_NEW(g_allocator, FileReader);

	PosColorVertex::init();
	//int x, y, n;
	//unsigned char* ayse = stbi_load("assets/ayse.png", &x, &y, &n, 0);
	//const bgfx::Memory* ayseMem = bgfx::makeRef(ayse, (x * y * n));
	//stbi_image_free(ayse);
	//bgfx::TextureHandle ayseTexture = bgfx::createTexture2D(x, y, false, 1,
	//	bgfx::TextureFormat::RGBA8,
	//	BGFX_TEXTURE_NONE | BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT,
	//	ayseMem);
	bgfx::TextureHandle ayseTexture = loadTexture(getFileReader(), "assets/ayse.png", BGFX_TEXTURE_NONE | BGFX_SAMPLER_POINT, 0, NULL, NULL);
	
	
	bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(bgfx::makeRef(planeVerts, sizeof(planeVerts)), PosColorVertex::pcvLayout);
	bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(bgfx::makeRef(planeTriList, sizeof(planeTriList)));

	bgfx::ShaderHandle vsh = loadShader("assets/shaders/windows_hlsl/vs_sprite.bin");
	bgfx::ShaderHandle fsh = loadShader("assets/shaders/windows_hlsl/fs_sprite.bin");
	bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

	bgfx::UniformHandle s_sprite = bgfx::createUniform("s_sprite", bgfx::UniformType::Sampler);

	SDL_Event e;
	bool quit = false;
	bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
	bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };
	while (!quit) {

		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.key.keysym.scancode == SDL_SCANCODE_D) {
				at.x += 0.1f;
				eye.x += 0.1f;
			}
			if (e.key.keysym.scancode == SDL_SCANCODE_A) {
				at.x += -0.1f;
				eye.x += -0.1f;
			}
			if (e.key.keysym.scancode == SDL_SCANCODE_W) {
				at.y += 0.1f;
				eye.y += 0.1f;
			}
			if (e.key.keysym.scancode == SDL_SCANCODE_S) {
				at.y += -0.1f;
				eye.y += -0.1f;
			}
		}
		bgfx::setViewRect(0, 0, 0, AC_SCREEN_WIDTH, AC_SCREEN_HEIGHT);
		float view[16];
		bx::mtxLookAt(view, eye, at);
		float proj[16];
		bx::mtxProj(proj, 60.0f, float(AC_SCREEN_WIDTH) / float(AC_SCREEN_HEIGHT), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
		bgfx::setViewTransform(0, view, proj);
		// This dummy draw call is here to make sure that view 0 is cleared
		// if no other draw calls are submitted to view 0.
		bgfx::touch(0);

		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);
		bgfx::setTexture(0, s_sprite, ayseTexture);

		// Set render states.
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_BLEND_ALPHA
		);

		bgfx::submit(0, program);

		bgfx::dbgTextClear();
		const bgfx::Stats* stats = bgfx::getStats();
		bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
			, stats->width
			, stats->height
			, stats->textWidth
			, stats->textHeight
		);


		bgfx::frame();
	}

	bgfx::shutdown();
	SDL_DestroyWindow(win);
	SDL_Quit();

	return 0;
}