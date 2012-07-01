#include "anki/gl/Texture.h"
#include "anki/gl/GlException.h"
#include "anki/util/Exception.h"

namespace anki {

//==============================================================================
// TextureManager                                                              =
//==============================================================================

//==============================================================================
TextureManager::TextureManager()
{
	GLint tmp;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tmp);
	unitsCount = tmp;
	
	mipmapping = true;
	anisotropyLevel = 8;
	compression = true;
}

//==============================================================================
// TextureUnits                                                                =
//==============================================================================

//==============================================================================
TextureUnits::TextureUnits()
{
	units.resize(TextureManagerSingleton::get().getMaxUnitsCount(), 
		Unit{0, 0});
	ANKI_ASSERT(units.size() > 7);

	activeUnit = -1;
	choseUnitTimes = 0;
}

//==============================================================================
int TextureUnits::whichUnit(const Texture& tex)
{
	GLuint glid = tex.getGlId();
	int i = units.size();

	do
	{
		--i;
	} while(i >= 0 && glid != units[i].tex);

	return i;
}

//==============================================================================
void TextureUnits::activateUnit(uint32_t unit)
{
	ANKI_ASSERT(unit < units.size());
	if(activeUnit != (int)unit)
	{
		activeUnit = (int)unit;
		glActiveTexture(GL_TEXTURE0 + activeUnit);
	}
}

//==============================================================================
uint32_t TextureUnits::choseUnit(const Texture& tex, bool& allreadyBinded)
{
	++choseUnitTimes;
	int myTexUnit = whichUnit(tex);

	// Already binded => renew it
	//
	if(myTexUnit != -1)
	{
		ANKI_ASSERT(units[myTexUnit].tex == tex.getGlId());
		units[myTexUnit].born = choseUnitTimes;
		allreadyBinded = true;
		return myTexUnit;
	}

	allreadyBinded = false;

	// Find an empty slot for it
	//
	for(uint32_t i = 0; i < units.size(); i++)
	{
		if(units[i].tex == 0)
		{
			units[i].tex = tex.getGlId();
			units[i].born = choseUnitTimes;
			return i;
		}
	}

	// Find the older unit and replace the texture
	//
	uint64_t older = 0;
	for(uint32_t i = 1; i < units.size(); ++i)
	{
		if(units[i].born < units[older].born)
		{
			older = i;
		}
	}

	units[older].tex = tex.getGlId();
	units[older].born = choseUnitTimes;
	return older;
}

//==============================================================================
uint32_t TextureUnits::bindTexture(const Texture& tex)
{
	bool allreadyBinded;
	uint32_t unit = choseUnit(tex, allreadyBinded);

	if(!allreadyBinded)
	{
		activateUnit(unit);
		glBindTexture(tex.getTarget(), tex.getGlId());
	}

	return unit;
}

//==============================================================================
void TextureUnits::bindTextureAndActivateUnit(const Texture& tex)
{
	bool allreadyBinded;
	uint32_t unit = choseUnit(tex, allreadyBinded);

	activateUnit(unit);

	if(!allreadyBinded)
	{
		glBindTexture(tex.getTarget(), tex.getGlId());
	}
}

//==============================================================================
void TextureUnits::unbindTexture(const Texture& tex)
{
	int unit = whichUnit(tex);
	if(unit == -1)
	{
		return;
	}

	units[unit].tex = 0;
	units[unit].born = 0;
	// There is no need to use GL to unbind
}

//==============================================================================
// Texture                                                                     =
//==============================================================================

//==============================================================================
Texture::~Texture()
{
	TextureUnitsSingleton::get().unbindTexture(*this);
	if(isCreated())
	{
		glDeleteTextures(1, &glId);
	}
}

//==============================================================================
void Texture::create(const Initializer& init)
{
	// Sanity checks
	//
	ANKI_ASSERT(!isCreated());
	ANKI_ASSERT(init.internalFormat > 4 && "Deprecated internal format");
	ANKI_ASSERT(init.width > 0 && init.height > 0);

	// Create
	//
	glGenTextures(1, &glId);
	ANKI_ASSERT(glId != 0);
	target = GL_TEXTURE_2D;
	internalFormat = init.internalFormat;
	format = init.format;
	type = init.type;
	width = init.width;
	height = init.height;

	// Bind
	TextureUnitsSingleton::get().bindTextureAndActivateUnit(*this);

	switch(internalFormat)
	{
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		glCompressedTexImage2D(target, 0, internalFormat,
			width, height, 0, init.dataSize, init.data);
		break;

	default:
		glTexImage2D(target, 0, internalFormat, width,
			height, 0, format, type, init.data);
	}

	if(init.repeat)
	{
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if(init.mipmapping && init.data)
	{
		glGenerateMipmap(target);
	}

	// If not mipmapping then the filtering cannot be trilinear
	if(init.filteringType == TFT_TRILINEAR && !init.mipmapping)
	{
		setFilteringNoBind(TFT_LINEAR);
	}
	else
	{
		setFilteringNoBind(init.filteringType);
	}

	if(init.anisotropyLevel > 1)
	{
		glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 
			GLint(init.anisotropyLevel));
	}

	ANKI_CHECK_GL_ERROR();
}

//==============================================================================
uint32_t Texture::bind() const
{
	return TextureUnitsSingleton::get().bindTexture(*this);
}

//==============================================================================
void Texture::genMipmap()
{
	TextureUnitsSingleton::get().bindTextureAndActivateUnit(*this);
	glGenerateMipmap(target);
}

//==============================================================================
void Texture::setFilteringNoBind(TextureFilteringType filterType) const
{
	switch(filterType)
	{
	case TFT_NEAREST:
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case TFT_LINEAR:
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case TFT_TRILINEAR:
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}

} // end namespace
