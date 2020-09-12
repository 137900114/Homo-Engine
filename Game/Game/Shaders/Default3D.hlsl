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
    
}

//the camera pass of will always be placed on register b1
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


#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1

cbuffer StanderdLight : register(b2){
    int lightType;       //stands for the type of the light
    float3 lightVec;     //if the light's type is directional light light vector stands for the direction of the light,
                         //if the light type is point light then the light vector stands for the position of the light
    float lightAttuation;//if the only the point light will use this attribute
    float3 lightIntensity;//the light intensity rgb value 
};


struct VertexOut{
    float3 Normal : TEXCOORD0;
    float4 worldPos : TEXCOORD1;
    float4 Position : SV_POSITION;
};

//we only use normal and position information in this shader
VertexOut VS(StandardVertexIn input){
    VertexOut output;
    output.Normal = mul((float3x3)transInvWorld ,input.Normal);
    //output.Normal = input.Normal;
    output.worldPos = mul(world,float4(input.Position,1.f));
    //output.Position = float4(world,1.);
    output.Position = mul(viewProj,output.worldPos);

    return output;
}

float4 PS(VertexOut input) : SV_TARGET {
    float3 target = float3(0.2,0.2,0.2);
    input.Normal = normalize(input.Normal);
    //we don't want to add negative value to the result 
    if(lightType == LIGHT_TYPE_DIRECTIONAL){
        target += (float3)lightIntensity * 
        clamp(dot(lightVec,- input.Normal),0.f,1.f);
    }else if(lightType == LIGHT_TYPE_POINT){
        float3 lightDir  = normalize(lightVec);
        target += (float3)lightIntensity * clamp(dot(lightDir,input.Normal),0.f,1.f);
    }

    return float4(target,1.);
}

