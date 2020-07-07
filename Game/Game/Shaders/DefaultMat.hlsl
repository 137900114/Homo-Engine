#define MAX_LIGHT_NUM 4
#define STD_VERTEX_IN
#define STD_LIGHT_PASS
#define STD_CAMERA_PASS

#include "STDPass.hlsli"

cbuffer ObjectPass : register(b0){
    float4x4 world;
    float4x4 transInvWorld;
    
};

struct VertexOut{
    float3 Normal : NORMAL;
    float4 Position : SV_TARGET;
};

//we only use normal and position information in this shader
VertexOut VS(VertexIn input){
    VertexOut output;
    output.Normal = mul(transInvWorld ,float4(input.Normal,0.f));
    ouput.Position = mul(world,float4(input.Position,1.f));

    return output;
}

float4 PS(VertexOut input) : SV_TARGET {
    float4 target = float4(0.f);
    for(int i = 0;i != MAX_LIGHT_NUM;i++){

        //we don't want to add negative value to the result 
        target += lights[i].intensity * 
        clamp(dot(lights[i].direction,- input.Normal,0.f,1.f);
    }

    return target;
}