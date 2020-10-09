struct StandardVertexIn{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

cbuffer ObjectBuffer : register(b0){
    float4x4 world;
    float4x4 transInvWorld;
    float4   color;
}

cbuffer StandardCamera : register(b1){
    float4x4 view;
    float4x4 invView;
    float4x4 proj;
    float4x4 invProj;
    float4x4 viewProj;
    float4x4 invViewProj;

    float3 cameraPosition;
    float timeLine;
    
    float nearPlane;
    float farPlane;
}

struct VertexOut{
    float4 color : TEXCOORD0;
    float4 Pos : SV_POSITION;
};

VertexOut VS(StandardVertexIn vert){
    VertexOut output;
    output.color = color;
    ouput.Pos = mul(world,float4(vert.Position,1.0));

    output.Pos = mul(viewProj,output.Pos);
    return output.Pos;
}

float4 PS(VertexOut input) : SV_TARGET{
    return input.color;
}