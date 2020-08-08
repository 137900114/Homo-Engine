#define PI 3.14159265359

//normal distribution function in pbr.
//discribs how many microsurface have the same direction with a vector
//in a surface 
float NormalDistribution(float3 normal,float3 halfn,float r){
    float NDotH = max(0.,dot(normal,halfn));
    float a = r * r;
    float a2 = a * a;

    float denum = NDotH * NDotH * (a2 - 1) + 1;
    denum = denum * denum * PI;

    return a2 / denum;
}

//discribs the refraction ratio of the surface in some direction
float3 SchlickFresnel(float3 normal,float3 halfn,float3 F0){
    //prevent negative 
    float NDotH = max(0.,dot(normal,halfn));
    float NDotH5 = pow(NDotH,5);

    return F0 + (float3(1.,1.,1.) - F0) * (1 - NDotH5);
}

//the function discribs in a direction how many 
//micro surface is occluded.
float SchlickGeometry(float3 normal,float3 v,float k){
    float NDotV = max(0.,dot(normal,v));
    
    return NDotV / ( NDotV * (1. - k) + k );
}

//surfaces are occulded both in light direction and view direction
float SmithGeometry(float3 normal,float3 light,float3 view,float k){
    float ggx1 = SchlickGeometry(normal,light,k);
    float ggx2 = SchlickGeometry(normal,view,k);

    return ggx1 * ggx2;
}

//to prevent memory alises
struct PointLight{
    float3 Position;
    float paddle0;
    float3 intensity;
    float paddle1;
};

struct SurfaceAttribute{
    float3 albedo;
    float metallic;
    float3 F0;
    float roughness;
    float normal;
    float ao;
    float kd;
    float ks;
};


float pointLightAttuation(float distance){
    return 1. / (distance * distance);
}

float directLightK(float roughness){
    float r = roughness + 1.;
    return r * r / 8.;
}

float IBLLightK(float roughness){
    return roughness * roughness / 2.;
}


//calculate the PBR result for point lights
float3 PBR_PointLight(SurfaceAttribute surface,
            float3 worldPos,float3 cameraPos,PointLight light){
        
        float3 V = normalize(cameraPos - worldPos);
        float3 L = normalize(light.Position - worldPos);
        float3 H = normalize(V + L);
        float3 N = normalize(surface.normal);

        float lightDistance = length(light.Position - worldPos);
        float attuation = pointLightAttuation(lightDistance);

        float3 radiance = attuation * light.intensity;
        

        float D = NormalDistribution(N,H,surface.roughness);
        float3 F = SchlickFresnel(N,H,surface.F0);
        float G = SmithGeometry(N,L,V,directLightK(surface.roughness));

        float denum = max(dot(V,N),0) * max(dot(L,N),0.) * 4.;
        float specular = D * F * G / denum;

        float3 Lout = (surface.kd * surface.albedo / PI + 
                surface.ks * specular) * radiance * max(dot(N , L) , 0.);

        return Lout;
}