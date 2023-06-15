// Fill out your copyright notice in the Description page of Project Settings.

#include "CopyBufferToTexCS.h"

IMPLEMENT_GLOBAL_SHADER(FCopyBufferToTexCS, "/Plugin/NNPP/CopyBufferToTex.usf", "Main", SF_Compute);