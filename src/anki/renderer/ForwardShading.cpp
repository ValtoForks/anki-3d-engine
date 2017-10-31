// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/renderer/ForwardShading.h>
#include <anki/renderer/Renderer.h>
#include <anki/renderer/RenderQueue.h>
#include <anki/renderer/GBuffer.h>
#include <anki/renderer/LightShading.h>
#include <anki/renderer/ShadowMapping.h>
#include <anki/renderer/Volumetric.h>
#include <anki/renderer/DepthDownscale.h>

namespace anki
{

ForwardShading::~ForwardShading()
{
}

Error ForwardShading::init(const ConfigSet& cfg)
{
	ANKI_R_LOGI("Initializing forward shading");

	Error err = initInternal(cfg);
	if(err)
	{
		ANKI_R_LOGE("Failed to initialize forward shading");
	}

	return err;
}

Error ForwardShading::initInternal(const ConfigSet&)
{
	m_width = m_r->getWidth() / FS_FRACTION;
	m_height = m_r->getHeight() / FS_FRACTION;

	// Create RT descr
	m_rtDescr = m_r->create2DRenderTargetDescription(m_width,
		m_height,
		FORWARD_SHADING_COLOR_ATTACHMENT_PIXEL_FORMAT,
		TextureUsageBit::SAMPLED_FRAGMENT | TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE,
		SamplingFilter::LINEAR,
		"forward");
	m_rtDescr.bake();

	// Create FB descr
	m_fbDescr.m_colorAttachmentCount = 1;
	m_fbDescr.m_colorAttachments[0].m_loadOperation = AttachmentLoadOperation::CLEAR;
	m_fbDescr.m_colorAttachments[0].m_clearValue.m_colorf = {{0.0, 0.0, 0.0, 1.0}};
	m_fbDescr.m_depthStencilAttachment.m_loadOperation = AttachmentLoadOperation::LOAD;
	m_fbDescr.m_depthStencilAttachment.m_aspect = DepthStencilAspectBit::DEPTH;
	m_fbDescr.bake();

	ANKI_CHECK(initVol());
	ANKI_CHECK(initUpscale());

	return Error::NONE;
}

Error ForwardShading::initVol()
{
	ANKI_CHECK(getResourceManager().loadResource("engine_data/BlueNoiseLdrRgb64x64.ankitex", m_vol.m_noiseTex));

	ANKI_CHECK(getResourceManager().loadResource("programs/ForwardShadingVolumetricUpscale.ankiprog", m_vol.m_prog));

	ShaderProgramResourceConstantValueInitList<3> consts(m_vol.m_prog);
	consts.add("NOISE_TEX_SIZE", U32(m_vol.m_noiseTex->getWidth()))
		.add("SRC_SIZE", Vec2(m_r->getWidth() / VOLUMETRIC_FRACTION, m_r->getHeight() / VOLUMETRIC_FRACTION))
		.add("FB_SIZE", Vec2(m_width, m_height));

	const ShaderProgramResourceVariant* variant;
	m_vol.m_prog->getOrCreateVariant(consts.get(), variant);
	m_vol.m_grProg = variant->getProgram();

	return Error::NONE;
}

Error ForwardShading::initUpscale()
{
	ANKI_CHECK(getResourceManager().loadResource("engine_data/BlueNoiseLdrRgb64x64.ankitex", m_upscale.m_noiseTex));

	// Shader
	ANKI_CHECK(getResourceManager().loadResource("programs/ForwardShadingUpscale.ankiprog", m_upscale.m_prog));

	ShaderProgramResourceConstantValueInitList<3> consts(m_upscale.m_prog);
	consts.add("NOISE_TEX_SIZE", U32(m_upscale.m_noiseTex->getWidth()))
		.add("SRC_SIZE", Vec2(m_r->getWidth() / FS_FRACTION, m_r->getHeight() / FS_FRACTION))
		.add("FB_SIZE", Vec2(m_r->getWidth(), m_r->getWidth()));

	const ShaderProgramResourceVariant* variant;
	m_upscale.m_prog->getOrCreateVariant(consts.get(), variant);
	m_upscale.m_grProg = variant->getProgram();

	return Error::NONE;
}

void ForwardShading::drawVolumetric(RenderingContext& ctx, CommandBufferPtr& cmdb, const RenderGraph& rgraph)
{
	cmdb->bindShaderProgram(m_vol.m_grProg);
	cmdb->setBlendFactors(0, BlendFactor::ONE, BlendFactor::ONE);
	cmdb->setDepthWrite(false);
	cmdb->setDepthCompareOperation(CompareOperation::ALWAYS);

	Vec4* unis = allocateAndBindUniforms<Vec4*>(sizeof(Vec4), cmdb, 0, 0);
	computeLinearizeDepthOptimal(ctx.m_renderQueue->m_cameraNear, ctx.m_renderQueue->m_cameraFar, unis->x(), unis->y());

	cmdb->bindTextureAndSampler(
		0, 0, rgraph.getTexture(m_r->getDepthDownscale().getHalfDepthColorRt()), m_r->getNearestSampler());
	cmdb->bindTextureAndSampler(
		0, 1, rgraph.getTexture(m_r->getDepthDownscale().getQuarterColorRt()), m_r->getNearestSampler());
	cmdb->bindTexture(0, 2, rgraph.getTexture(m_r->getVolumetric().getRt()));
	cmdb->bindTexture(0, 3, m_vol.m_noiseTex->getGrTexture());

	m_r->drawQuad(cmdb);

	// Restore state
	cmdb->setBlendFactors(0, BlendFactor::ONE, BlendFactor::ZERO);
	cmdb->setDepthWrite(true);
	cmdb->setDepthCompareOperation(CompareOperation::LESS);
}

void ForwardShading::drawUpscale(const RenderingContext& ctx, const RenderGraph& rgraph, CommandBufferPtr& cmdb)
{
	// **WARNING** Remember to update the consumers of the render pass that calls this method
	Vec4* linearDepth = allocateAndBindUniforms<Vec4*>(sizeof(Vec4), cmdb, 0, 0);
	computeLinearizeDepthOptimal(
		ctx.m_renderQueue->m_cameraNear, ctx.m_renderQueue->m_cameraFar, linearDepth->x(), linearDepth->y());

	cmdb->bindTexture(0, 0, rgraph.getTexture(m_r->getGBuffer().getDepthRt()));
	cmdb->bindTextureAndSampler(
		0, 1, rgraph.getTexture(m_r->getDepthDownscale().getHalfDepthColorRt()), m_r->getNearestSampler());
	cmdb->bindTexture(0, 2, rgraph.getTexture(m_runCtx.m_rt));
	cmdb->bindTexture(0, 3, m_upscale.m_noiseTex->getGrTexture());

	cmdb->setBlendFactors(0, BlendFactor::ONE, BlendFactor::SRC_ALPHA);

	cmdb->bindShaderProgram(m_upscale.m_grProg);
	cmdb->setViewport(0, 0, m_r->getWidth(), m_r->getHeight());

	m_r->drawQuad(cmdb);

	// Restore state
	cmdb->setBlendFactors(0, BlendFactor::ONE, BlendFactor::ZERO);
}

void ForwardShading::run(
	RenderingContext& ctx, CommandBufferPtr& cmdb, U threadId, U threadCount, const RenderGraph& rgraph)
{
	const U problemSize = ctx.m_renderQueue->m_forwardShadingRenderables.getSize();
	PtrSize start, end;
	ThreadPoolTask::choseStartEnd(threadId, threadCount, problemSize, start, end);

	if(start == end)
	{
		// Early exit
		return;
	}

	cmdb->bindTexture(0, 0, rgraph.getTexture(m_r->getDepthDownscale().getQuarterColorRt()));
	cmdb->bindTexture(0, 1, rgraph.getTexture(m_r->getShadowMapping().getShadowmapRt()));
	bindUniforms(cmdb, 0, 0, ctx.m_lightShading.m_commonToken);
	bindUniforms(cmdb, 0, 1, ctx.m_lightShading.m_pointLightsToken);
	bindUniforms(cmdb, 0, 2, ctx.m_lightShading.m_spotLightsToken);
	bindStorage(cmdb, 0, 0, ctx.m_lightShading.m_clustersToken);
	bindStorage(cmdb, 0, 1, ctx.m_lightShading.m_lightIndicesToken);

	cmdb->setViewport(0, 0, m_width, m_height);
	cmdb->setBlendFactors(
		0, BlendFactor::ONE_MINUS_SRC_ALPHA, BlendFactor::SRC_ALPHA, BlendFactor::DST_ALPHA, BlendFactor::ZERO);
	cmdb->setBlendOperation(0, BlendOperation::ADD);
	cmdb->setDepthWrite(false);

	// Start drawing
	m_r->getSceneDrawer().drawRange(Pass::GB_FS,
		ctx.m_renderQueue->m_viewMatrix,
		ctx.m_viewProjMatJitter,
		cmdb,
		ctx.m_renderQueue->m_forwardShadingRenderables.getBegin() + start,
		ctx.m_renderQueue->m_forwardShadingRenderables.getBegin() + end);

	if(threadId == threadCount - 1)
	{
		drawVolumetric(ctx, cmdb, rgraph);
	}
}

void ForwardShading::populateRenderGraph(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;
	m_runCtx.m_ctx = &ctx;

	// Create RT
	m_runCtx.m_rt = rgraph.newRenderTarget(m_rtDescr);

	// Create pass
	GraphicsRenderPassDescription& pass = rgraph.newGraphicsRenderPass("Forward shading");

	pass.setWork(runCallback,
		this,
		computeNumberOfSecondLevelCommandBuffers(ctx.m_renderQueue->m_forwardShadingRenderables.getSize()));
	pass.setFramebufferInfo(m_fbDescr, {{m_runCtx.m_rt}}, m_r->getDepthDownscale().getHalfDepthDepthRt());

	pass.newConsumer({m_runCtx.m_rt, TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE});
	pass.newConsumer({m_r->getDepthDownscale().getHalfDepthDepthRt(), TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ});
	pass.newConsumer({m_r->getDepthDownscale().getHalfDepthColorRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	pass.newConsumer({m_r->getDepthDownscale().getQuarterColorRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	pass.newConsumer({m_r->getVolumetric().getRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	pass.newConsumer({m_r->getShadowMapping().getShadowmapRt(), TextureUsageBit::SAMPLED_FRAGMENT});

	pass.newProducer({m_runCtx.m_rt, TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE});
	pass.newProducer({m_r->getDepthDownscale().getHalfDepthDepthRt(), TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ});
}

} // end namespace anki
