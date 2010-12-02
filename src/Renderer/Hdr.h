#ifndef HDR_H
#define HDR_H

#include "RenderingPass.h"
#include "Fbo.h"
#include "Texture.h"
#include "RsrcPtr.h"
#include "ShaderProg.h"
#include "Properties.h"


/// High dynamic range lighting pass
class Hdr: private RenderingPass
{
	PROPERTY_R(Texture, toneFai, getToneFai) ///< Vertical blur pass FAI
	PROPERTY_R(Texture, hblurFai, getHblurFai) ///< pass0Fai with the horizontal blur FAI
	PROPERTY_R(Texture, fai, getFai) ///< The final FAI

	public:
		Hdr(Renderer& r_, Object* parent): RenderingPass(r_, parent) {}
		void init(const RendererInitializer& initializer);
		void run();

		/// Setters & getters
		/// @{
		float getBlurringDist() {return blurringDist;}
		void setBlurringDist(float f) {blurringDist = f;}
		uint getBlurringIterations() {return blurringIterations;}
		void setBlurringIterations(uint i) {blurringIterations = i;}
		float getExposure() const {return exposure;}
		void setExposure(float f) {exposure = f;}
		bool isEnabled() const {return enabled;}
		float getRenderingQuality() const {return renderingQuality;}
		/// @}

	private:
		Fbo toneFbo;
		Fbo hblurFbo;
		Fbo vblurFbo;
		RsrcPtr<ShaderProg> toneSProg;
		RsrcPtr<ShaderProg> hblurSProg;
		RsrcPtr<ShaderProg> vblurSProg;
		float blurringDist;
		uint blurringIterations;
		float exposure; ///< How bright is the HDR
		bool enabled;
		float renderingQuality;

		void initFbo(Fbo& fbo, Texture& fai);
};


#endif
