//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
cbuffer SceneConstantBuffer : register(b0)
{
    float4 offset;
    float4 cbcolor;
    float padding[16];
}

cbuffer Lerp : register(b1)
{
    float mixColor;
    float paddinglerp[3];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);
SamplerState g_Sampler2 : register(s1);
Texture2D g_Texture2 : register(t1);

PSInput VSMain(float4 position : POSITION, float4 color: COLOR, float2 uv : TEXCOORD)
{
    PSInput result;

    //result.position = position;
    //result.position = position + offset;
    // For "mul" if X is a vector, it is treated as row-major (HLSL logic). 
    result.position = position + offset;
    //color.y = cbcolor.y;
    result.color = color;
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // Linearly interpolate the textures.
    return lerp(g_Texture.Sample(g_Sampler, input.uv), g_Texture2.Sample(g_Sampler2, input.uv), mixColor);
    // Flip overlapping texture. 
    //return lerp(g_Texture.Sample(g_Sampler, input.uv), g_Texture2.Sample(g_Sampler2, float2(1.0f - input.uv.x, input.uv.y)), 0.2f);
    
    // Simply return first texture passed in/created.
    //return g_Texture.Sample(g_Sampler, input.uv);
}

