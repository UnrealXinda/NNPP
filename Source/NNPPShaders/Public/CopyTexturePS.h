// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderParameterStruct.h"

class NNPPSHADERS_API FCopyTexturePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FCopyTexturePS);
	SHADER_USE_PARAMETER_STRUCT(FCopyTexturePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};