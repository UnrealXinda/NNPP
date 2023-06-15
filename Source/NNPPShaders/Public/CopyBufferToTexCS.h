// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderParameterStruct.h"

class NNPPSHADERS_API FCopyBufferToTexCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FCopyBufferToTexCS);
	SHADER_USE_PARAMETER_STRUCT(FCopyBufferToTexCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, InputBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutputTexture)
		SHADER_PARAMETER(FIntVector, InputDim)
	END_SHADER_PARAMETER_STRUCT()
};