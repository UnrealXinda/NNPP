// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RenderCore/Public/GlobalShader.h"
#include "RenderCore/Public/ShaderParameterUtils.h"
#include "RenderCore/Public/ShaderParameterMacros.h"

#include "Public/GlobalShader.h"
#include "Public/ShaderParameterUtils.h"
#include "RHI/Public/RHICommandList.h"

#define DECLARE_NNLAYER_COMPUTE_SHADER(ShaderName)                                             \
DECLARE_GLOBAL_SHADER(ShaderName);                                                             \
public:                                                                                        \
static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)     \
{                                                                                              \
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);                \
}                                                                                              \
static bool ShouldCache(EShaderPlatform Platform)                                              \
{                                                                                              \
	return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);                           \
}                                                                                              \
static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, \
	FShaderCompilerEnvironment& OutEnvironment)                                                \
{                                                                                              \
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);                   \
}                                                                                              \
void SetShaderParameters(FRHICommandList& RHICmdList, const FParameters& Parameters)           \
{                                                                                              \
	auto Param = GetUniformBufferParameter<FParameters>();                                     \
	FRHIComputeShader* ComputeShader = RHICmdList.GetBoundComputeShader();                     \
	SetUniformBufferParameterImmediate(RHICmdList, ComputeShader, Param, Parameters);          \
}                                                                                              \
public: