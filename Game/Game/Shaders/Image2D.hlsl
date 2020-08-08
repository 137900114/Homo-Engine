
SamplerState warpSampler : register(s0);

Texture2D image : register(t0);

struct VertIn{
    float3 Position : POSITION;
    float2 Coord    : TEXCOORD0;    
};

struct VertOut{
    float4 Position : SV_POSITION;
    float2 Coord    : TEXCOORD0;
};

cbuffer Proj : register(b0){
    float4x4 worldProj;
}


VertOut VS(VertIn input){
    VertOut output;
    output.Position = mul(worldProj,float4(input.Position,1.f));
    output.Coord    = input.Coord;

    return output;
}

float4 PS(VertOut input):SV_TARGET{
    float4 color = image.Sample(warpSampler,input.Coord);

    return color;
}
