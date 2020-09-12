struct VertexIn{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

#ifndef MAX_LIGHT_NUM
#define MAX_LIGHT_NUM 1
#endif

struct Light{
    float3 direction;
    float4 intensity;
};

cbuffer LightPass : register(b2){
    Light lights[MAX_LIGHT_NUM];
    float3 ambient;
};



cbuffer CameraPass : register(b1){
    float4x4 proj;
    float4x4 invProj;

    float timeLine;

};

cbuffer ObjectPass : register(b0){
    float4x4 world;
    float4x4 transInvWorld;
};

struct VertexOut{
    float3 Normal : NORMAL;
    float4 Position : SV_POSITION;
};

//we only use normal and position information in this shader
VertexOut VS(VertexIn input){
    VertexOut output;
    output.Normal = mul((float3x3)transInvWorld ,input.Normal);
    //output.Normal = input.Normal;
    output.Position = mul(world,float4(input.Position,1.f));
    //output.Position = float4(world,1.);
    output.Position = mul(proj,output.Position);

    return output;
}

float4 PS(VertexOut input) : SV_TARGET {
    float3 target = ambient;
    for(int i = 0;i != MAX_LIGHT_NUM;i++){

        //we don't want to add negative value to the result 
        target += (float3)lights[i].intensity * 
        clamp(dot(lights[i].direction,- input.Normal),0.f,1.f);
    }
    
    return float4(target,1.);
}