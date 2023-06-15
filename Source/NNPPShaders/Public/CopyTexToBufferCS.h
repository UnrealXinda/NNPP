// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderParameterStruct.h"

class NNPPSHADERS_API FCopyTexToBufferCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FCopyTexToBufferCS);
	SHADER_USE_PARAMETER_STRUCT(FCopyTexToBufferCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, OutputBuffer)
		SHADER_PARAMETER(FIntVector, InputDim)
	END_SHADER_PARAMETER_STRUCT()
};