#include "Engine/Core/FileUtility.h"
#include "Engine/Data/SpriteData.h"
#include "Engine/Data/AseData.h"
#include "Engine/Core/Context.h"
#include "Engine/Tools/aseprite.h"
#include "stb_image.h"
#include "SDL.h"
#include <vector>

#include"bimg/decode.h"

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


SpriteData* FileUtility::loadSpriteData(char path[])
{
	SpriteData *spriteData = DBG_NEW SpriteData();
	int req_format = STBI_rgb_alpha;
	spriteData->pixels = stbi_load(path, &spriteData->width, &spriteData->height, 
		&spriteData->orig_format, req_format);
	if (spriteData->pixels == nullptr) {
		SDL_Log("Loading image failed: %s", stbi_failure_reason());
		delete(spriteData);
		return nullptr;
	}
	if (req_format == STBI_rgb) {
		spriteData->depth = 24;
		spriteData->pitch = 3 * spriteData->width; // 3 bytes per pixel * pixels per row
		spriteData->pixel_format = SDL_PIXELFORMAT_RGB24;
	}
	else { // STBI_rgb_alpha (RGBA)
		spriteData->depth = 32;
		spriteData->pitch = 4 * spriteData->width;
		spriteData->pixel_format = SDL_PIXELFORMAT_RGBA32;
	}

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(spriteData->pixels, spriteData->width, spriteData->height,
		spriteData->depth, spriteData->pitch, spriteData->pixel_format);
	//spriteData->texture = SDL_CreateTextureFromSurface(Context::getRenderer(), surface);
	SDL_FreeSurface(surface);

	return spriteData;
}

void FileUtility::unloadSpriteData(SpriteData* spriteData)
{
	if (spriteData->pixels != nullptr) {
		stbi_image_free(spriteData->pixels);
		spriteData->pixels = nullptr;
	}

	if (spriteData->texture != nullptr) {
		SDL_DestroyTexture(spriteData->texture);
		spriteData->texture = nullptr;
	}

	delete(spriteData);
}

AseData* FileUtility::loadAse(char path[])
{
	Aseprite::AsepriteFile* file = DBG_NEW Aseprite::AsepriteFile(path);
	AseData* data = DBG_NEW AseData();
	SDL_Surface* surface;
	SDL_Texture* texture;

	//Get Frames
	data->frames.resize(file->frames.size());
	for (size_t i = 0; i < file->frames.size(); i++){
		data->frames[i].frameDuration = file->frames[i].frameDuration;
		//Get and Setup Sprites
		for (const auto& chunk : file->frames[i].chunks) {
			if (chunk.type == Aseprite::AseChunkType::CEL_0x2005) {
				const auto& celChunk = std::get<Aseprite::AseCelChunk>(chunk.data);

				surface = SDL_CreateRGBSurfaceWithFormatFrom(
					(unsigned char*)celChunk.pixels.data(),
					celChunk.width, celChunk.height, 32,
					4 * celChunk.width, SDL_PIXELFORMAT_RGBA32);
				//texture = SDL_CreateTextureFromSurface(Context::getRenderer(), surface);

				SDL_FreeSurface(surface);

				data->frames[i].sprites.push_back(AseSprite(texture, celChunk.width,
					celChunk.height, celChunk.x, celChunk.y, celChunk.layerIndex));
			}
		}
	}

	//Setup Animations
	for (size_t i = 0; i < file->frames.size(); i++) {
		for (const auto& chunk : file->frames[i].chunks) {
			if (chunk.type == Aseprite::AseChunkType::FRAME_TAGS_0x2018) {
				const auto& celChunk = std::get<Aseprite::AseFrameTagChunk>(chunk.data);
				for (const auto& tag : celChunk.tags) {
					AseAnimation a(tag.tagName.toString());
					for (size_t i = tag.fromFrame; i <= tag.toFrame; i++){
						a.frames.push_back(&data->frames[i]);
					}
					data->animations.push_back(AseAnimation(a));
				}
			}
		}
	}

	delete(file);
	return data;
}

void FileUtility::unloadAse(AseData* data)
{
	delete(data);
}

void FileUtility::init()
{
	g_allocator = getDefaultAllocator();
}

void* FileUtility::load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size)
{
	if (bx::open(_reader, _filePath)) {
		uint32_t size = (uint32_t)bx::getSize(_reader);
		void* data = BX_ALLOC(_allocator, size);
		bx::read(_reader, data, size);
		bx::close(_reader);
		if (NULL != _size) {
			*_size = size;
		}
		return data;
	}
	else {
		SDL_Log("Failed to open: %s.", _filePath);
	}
	if (NULL != _size) {
		*_size = 0;
	}
	return NULL;
}

bgfx::TextureHandle FileUtility::loadTexture(bx::FileReaderI* _reader, const char* _filePath, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
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

bgfx::TextureHandle FileUtility::loadTexture(const char* _name, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
{
	return loadTexture(getFileReader(), _name, _flags, _skip, _info, _orientation);
}

void FileUtility::imageReleaseCb(void* _ptr, void* _userData)
{
	BX_UNUSED(_ptr);
	bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
	bimg::imageFree(imageContainer);
}