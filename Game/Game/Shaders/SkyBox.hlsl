#include "Common.hlsl"

struct VertexIn{
    float3 Position : POSITION;
};

struct VertexOut{
    float3 WorldPos : POSITION;
    float4 Position : SV_Position;
};

TextureCube skyBox : register(t0);
SamplerState skySampler : register(s0);

USE_STANDARD_CAMERA(0,0)


VertexOut VS(VertexIn input){
    VertexOut output;

    output.WorldPos = normalize(input.Position);
    output.Position = mul(proj,input.Position).xyww;

    return output;
}

float4 PS(VertexOut input) : SV_TARGET{
    return skyBox.Sample(skySampler,input.WorldPos);
}