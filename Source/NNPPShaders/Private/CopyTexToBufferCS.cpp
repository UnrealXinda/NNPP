// Fill out your copyright notice in the Description page of Project Settings.

#include "CopyTexToBufferCS.h"

IMPLEMENT_GLOBAL_SHADER(FCopyTexToBufferCS, "/Plugin/NNPP/CopyTexToBuffer.usf", "Main", SF_Compute);