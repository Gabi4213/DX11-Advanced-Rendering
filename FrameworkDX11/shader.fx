//--------------------------------------------------------------------------------------
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// the lighting equations in this code have been taken from https://www.3dgep.com/texturing-lighting-directx-11/
// with some modifications by David White

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 vOutputColor;
}

Texture2D txDiffuse : register(t0);
Texture2D txDiffuse2 : register(t1);
Texture2D txNormal : register(t2);

Texture2D txQuad : register(t3);

SamplerState samLinear : register(s0);

Texture2D DiffuseMap : register(t0);
SamplerState Sampler0 : register(s0);

Texture2D g_AlbedoMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_Normal : register(t2);
Texture2D g_PositionMap : register(t3);
Texture2D g_SpecularMap : register(t4);
Texture2D g_DepthMap : register(t5);


#define MAX_LIGHTS 5
// Light types.
// Light types.
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct _Material
{
    float4 Emissive; // 16 bytes
							//----------------------------------- (16 byte boundary)
    float4 Ambient; // 16 bytes
							//------------------------------------(16 byte boundary)
    float4 Diffuse; // 16 bytes
							//----------------------------------- (16 byte boundary)
    float4 Specular; // 16 bytes
							//----------------------------------- (16 byte boundary)
    float SpecularPower; // 4 bytes
    bool UseTexture; // 4 bytes
    float2 Padding; // 8 bytes
							//----------------------------------- (16 byte boundary)
}; // Total:               // 80 bytes ( 5 * 16 )

cbuffer MaterialProperties : register(b1)
{
    _Material Material;
};

struct Light
{
    float4 Position; // 16 bytes
										//----------------------------------- (16 byte boundary)
    float4 Direction; // 16 bytes
										//----------------------------------- (16 byte boundary)
    float4 Color; // 16 bytes
										//----------------------------------- (16 byte boundary)
    float SpotAngle; // 4 bytes
    float ConstantAttenuation; // 4 bytes
    float LinearAttenuation; // 4 bytes
    float QuadraticAttenuation; // 4 bytes
										//----------------------------------- (16 byte boundary)
    int LightType; // 4 bytes
    bool Enabled; // 4 bytes
    int2 Padding; // 8 bytes
										//----------------------------------- (16 byte boundary)
}; // Total:                           // 80 bytes (5 * 16)

cbuffer LightProperties : register(b2)
{
    float4 EyePosition; // 16 bytes
   										//----------------------------------- (16 byte boundary)
    float4 GlobalAmbient; // 16 bytes
										//----------------------------------- (16 byte boundary)
    Light Lights[MAX_LIGHTS]; // 80 * 8 = 640 bytes
}; 

cbuffer PostProcessingParams : register(b0)
{
	//Greyscale
    float enableGreyscale;
    float greyscaleStrength;
   	
	//Chromatic Aberration
    float aberrationStrength;
	
	//Guasian Blur
    float blurStrength;
       
    float nearDistance;
    float farDistance;
};

cbuffer PerObject : register(b0)
{
    matrix gWorld;
    matrix gView;
    matrix gProj;
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 worldPos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
	
    float3 eyeVectorTS : EYEVECTORTS;
    float3 lightVectorTS : LIGHTVECTORTS;
};

struct QUAD_VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct QUAD_VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

struct QUAD_PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
	
    float3 Norm : NORMAL;
	
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
			
    float3 eyeVectorTS : EYEVECTORTS;
    float3 lightVectorTS : LIGHTVECTORTS;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
};

struct PSOutput
{
    float4 Albedo : SV_Target0;
    float4 NormalMap : SV_Target1;
    float4 Normal : SV_Target2;
    float4 Position : SV_Target3;
    float4 SpecularMap : SV_Target4;
    float  Depth : SV_Target5;
};

float3 VectorToTangentSpace(float3 vectorV, float3x3 TBN_inv)
{
    float3 tangentSpaceNormal = normalize(mul(vectorV, TBN_inv));
    return tangentSpaceNormal;
}

float4 DoDiffuse(Light light, float3 L, float3 N)
{
    float NdotL = max(0, dot(N, L));
    return light.Color * NdotL;
}

float4 DoSpecular(Light lightObject, float3 vertexToEye, float3 lightDirectionToVertex, float3 Normal)
{
    float4 lightDir = float4(normalize(-lightDirectionToVertex), 1);
    vertexToEye = normalize(vertexToEye);

    float lightIntensity = saturate(dot(Normal, lightDir));
    float4 specular = float4(0, 0, 0, 0);
    if (lightIntensity > 0.0f)
    {
        float3 reflection = normalize(2 * lightIntensity * Normal - lightDir);
        specular = pow(saturate(dot(reflection, vertexToEye)), Material.SpecularPower); // 32 = specular power
    }

    return specular;
}

float DoAttenuation(Light light, float d)
{
    return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * d + light.QuadraticAttenuation * d * d);
}

float DoSpotCone(Light light, float3 L)
{
    float minCos = cos(light.SpotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(light.Direction.xyz, -L);
    return smoothstep(minCos, maxCos, cosAngle);
}

struct LightingResult
{
    float4 Diffuse;
    float4 Specular;
};

LightingResult DoPointLight(Light light, float3 vertexToEye, float4 vertexPos, float3 N)
{
    LightingResult result;

    float3 LightDirectionToVertex = (vertexPos - light.Position).xyz;
    float distance = length(LightDirectionToVertex);
    LightDirectionToVertex = LightDirectionToVertex / distance;

    float3 vertexToLight = (light.Position - vertexPos).xyz;
    distance = length(vertexToLight);
    vertexToLight = vertexToLight / distance;

    float attenuation = DoAttenuation(light, distance);

    result.Diffuse = DoDiffuse(light, vertexToLight, N) * attenuation;
    result.Specular = DoSpecular(light, vertexToEye, LightDirectionToVertex, N) * attenuation;

    return result;
}

//Both directional and spot light were taken from here: https://www.3dgep.com/texturing-lighting-directx-11/#Lighting
//the same place the point light was taken and given to us from
LightingResult DoDirectionalLight(Light light, float3 V, float4 P, float3 N)
{
    LightingResult result;
 
    float3 L = -light.Direction.xyz;
 
    result.Diffuse = DoDiffuse(light, L, N);
    result.Specular = DoSpecular(light, V, L, N);
 
    return result;
}

LightingResult DoSpotLight(Light light, float3 V, float4 P, float3 N)
{
    LightingResult result;
 
    float3 L = (light.Position - P).xyz;
    float distance = length(L);
    L = L / distance;
 
    float attenuation = DoAttenuation(light, distance);
    float spotIntensity = DoSpotCone(light, L);
 
    result.Diffuse = DoDiffuse(light, L, N) * attenuation * spotIntensity;
    result.Specular = DoSpecular(light, V, L, N) * attenuation * spotIntensity;
 
    return result;
}


LightingResult ComputeLighting(float4 vertexPos, float3 N)
{
    float3 vertexToEye = normalize(EyePosition - vertexPos).xyz;

    LightingResult totalResult = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

	[unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        LightingResult result = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

        if (!Lights[i].Enabled) 
            continue;
		
        
        switch (Lights[i].LightType)
        {
            case 0:
                result = DoPointLight(Lights[i], vertexToEye, vertexPos, N);
                break;
            case 1:
                result = DoDirectionalLight(Lights[i], vertexToEye, vertexPos, N);
                break;
            case 2:
                result = DoSpotLight(Lights[i], vertexToEye, vertexPos, N);
                break;
        }
        		
        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    totalResult.Diffuse = saturate(totalResult.Diffuse);
    totalResult.Specular = saturate(totalResult.Specular);

    return totalResult;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(input.Pos, World);
    output.worldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

	// multiply the normal by the world transform (to go from model space to world space)
    output.Norm = mul(float4(input.Norm, 0), World).xyz;

    output.Tex = input.Tex;
	
	////get the vertex
    float3 vertexToEye = EyePosition - output.worldPos.xyz;
    float3 vertexToLight = Lights[0].Position - output.worldPos.xyz;

   ////build TBN
    float3 T = normalize(mul(input.Tangent, World));
    float3 B = normalize(mul(input.BiTangent, World));
    float3 N = normalize(mul(input.Norm, World));

    float3x3 TBN = float3x3(T, B, N);
    float3x3 TBN_inv = transpose(TBN);

    output.eyeVectorTS = VectorToTangentSpace(vertexToEye.xyz, TBN_inv);
    output.lightVectorTS = VectorToTangentSpace(vertexToLight.xyz, TBN_inv);
	
    return output;
}

QUAD_VS_OUTPUT QUAD_VS(QUAD_VS_INPUT Input)
{
    QUAD_VS_OUTPUT Output;
    Output.Pos = Input.Pos;
    Output.Tex = Input.Tex;
    return Output;
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    
	//World pos
    float4 worldPos = mul(float4(input.Position, 1.0f), gWorld);
    output.WorldPos = worldPos.xyz;
        
    //Normal in world space
    output.Normal = mul(float4(input.Normal, 0.0f), gWorld).xyz;
    
	//View pos
    float4 viewPos = mul(worldPos, gView);
    output.Position = mul(viewPos, gProj);
    
	//Texcoord
    output.TexCoord = input.TexCoord;  

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT IN) : SV_TARGET
{
    float4 normalMap = txNormal.Sample(samLinear, IN.Tex);
    normalMap = (normalMap * 2.0f) - 1.0f;
    normalMap = float4(normalize(normalMap.xyz), 1);
    float3 tangentSpaceNormal = normalize(normalMap.xyz);
	
    float3 lightVectorTS = normalize(IN.lightVectorTS);
    float3 eyeVectorTS = normalize(IN.eyeVectorTS);
	
    LightingResult lit = ComputeLighting(IN.worldPos, tangentSpaceNormal);

    float4 texColor = { 1, 1, 1, 1 };
	
    texColor = txDiffuse.Sample(samLinear, IN.Tex);
	
    float4 emissive = Material.Emissive;
    float4 ambient = Material.Ambient * GlobalAmbient;
    float4 diffuse = Material.Diffuse * lit.Diffuse;
    float4 specular = Material.Specular * lit.Specular;

    float4 finalColor = (emissive + ambient + diffuse + specular) * texColor;

    return finalColor;
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearDistance * farDistance) / (farDistance + nearDistance - z * (farDistance - nearDistance));
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    // albedo
    float4 texColor = DiffuseMap.Sample(Sampler0, input.TexCoord);
    output.Albedo = texColor;
	
	//normal map
    float4 normalMap = g_Normal.Sample(Sampler0, input.TexCoord);
    output.NormalMap = normalMap;

	//normal
    float3 normal = normalize(input.Normal);
    output.Normal = float4(normal * 0.5f + 0.5f, 1.0f);

	//position
    output.Position = float4(input.WorldPos, 1.0f);
    
    //specular
    float4 specularMap = g_SpecularMap.Sample(Sampler0, input.TexCoord);
    output.SpecularMap = specularMap;
    
    output.Depth = (output.Position.z - nearDistance) / (farDistance - nearDistance); 
    
    //output.Depth = (view - nearDistance) / (farDistance - nearDistance);
             
    return output;
}
//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor;
}

// Horizontal Gaussian Blur - Pass 1A
float4 HorizontalBlur(QUAD_PS_INPUT input) : SV_Target
{
	//uses screen width and height - needs a better method of passing those values in rather than hard coding.
    float2 texelSize = float2(1.0f / 1920, 1.0f / 1061);
	
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;

    int kernelSize = int(5 + blurStrength * 10);


    float blur[15] =
    {
        0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f
    };

    for (int i = -(kernelSize / 2); i <= kernelSize / 2; i++)
    {
        float2 offset = float2(i * texelSize.x, 0.0f);
        color += DiffuseMap.Sample(samLinear, input.Tex + offset) * blur[i + kernelSize / 2];
        weightSum += blur[i + kernelSize / 2];
    }

    return color / weightSum;
}

// Vertical Gaussian Blur - Pass 2
float4 VerticalBlur(QUAD_PS_INPUT input) : SV_Target
{
    float2 texelSize = float2(1.0f / 1920, 1.0f / 1061);
	
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;

    // Calculate kernel size based on blur strength (increasing size with strength)
    int kernelSize = int(5 + blurStrength * 10); // This adjusts the kernel size (range from 5 to 15)

    // Define the same Gaussian kernel for vertical pass

    float blur[15] =
    {
        0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f
    };

    // Sample pixels vertically and accumulate the results
    for (int i = -(kernelSize / 2); i <= kernelSize / 2; i++)
    {
        float2 offset = float2(0.0f, i * texelSize.y);
        color += DiffuseMap.Sample(samLinear, input.Tex + offset) * blur[i + kernelSize / 2];
        weightSum += blur[i + kernelSize / 2];
    }

    return color / weightSum;
}

float4 GaussianBlurHorizontal(QUAD_PS_INPUT input)
{
    //uses screen width and height - needs a better method of passing those values in rather than hard coding.
    float2 texelSize = float2(1.0f / 1920, 1.0f / 1061);
	
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;

    int kernelSize = int(5 + 3.0f * 10);

    float blur[35] =
    {
        0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
         0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
        0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f
    };

    for (int i = -(kernelSize / 2); i <= kernelSize / 2; i++)
    {
        float2 offset = float2(i * texelSize.x, 0.0f);
        color += DiffuseMap.Sample(samLinear, input.Tex + offset) * blur[i + kernelSize / 2];
        weightSum += blur[i + kernelSize / 2];
    }

    return color / weightSum;
}

float4 GaussianBlurVerticalBlur(QUAD_PS_INPUT input) : SV_Target
{
    float2 texelSize = float2(1.0f / 1920, 1.0f / 1061);
	
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;

    int kernelSize = int(5 + 3.0 * 10); 

    float blur[35] =
    {
        0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
         0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
		0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f,
        0.05f,
		0.10f,
		0.2f,
		0.2f,
		0.10f
    };

    // Sample pixels vertically and accumulate the results
    for (int i = -(kernelSize / 2); i <= kernelSize / 2; i++)
    {
        float2 offset = float2(0.0f, i * texelSize.y);
        color += DiffuseMap.Sample(samLinear, input.Tex + offset) * blur[i + kernelSize / 2];
        weightSum += blur[i + kernelSize / 2];
    }

    return color / weightSum;
}

float4 QUAD_PS(QUAD_PS_INPUT IN) : SV_TARGET
{
    float4 vColor = DiffuseMap.Sample(samLinear, IN.Tex);

	//greyscale
    if (enableGreyscale != 0.0f)
    {
        float greyscale = dot(vColor.rgb, float3(0.5f, 0.5f, 0.5f)) * greyscaleStrength;
        return float4(greyscale, greyscale, greyscale, vColor.a);
    }
  
	//chromatic abberation
    if (aberrationStrength > 0.0f)
    {
		//using 1.0f so its on the right by defualt - can change so its customizabledirection
        float2 redOffset = 1.0f * aberrationStrength * 0.02f;
        float2 greenOffset = float2(0.0f, 0.0f);
        float2 blueOffset = -1.0f * aberrationStrength * 0.02f;

        float4 redSample = txQuad.Sample(samLinear, IN.Tex + redOffset);
        float4 greenSample = txQuad.Sample(samLinear, IN.Tex + greenOffset);
        float4 blueSample = txQuad.Sample(samLinear, IN.Tex + blueOffset);
		
        vColor.r = redSample.r;
        vColor.g = greenSample.g;
        vColor.b = blueSample.b;
    }
	
	//gaussian blur
    if (blurStrength > 0.0f)
    {
        vColor = HorizontalBlur(IN);
        vColor = VerticalBlur(IN);
    }
    
    return vColor;
}

float4 PSLIGHTING(QUAD_PS_INPUT IN) : SV_TARGET
{
	// Sample albedo
    float4 albedo = DiffuseMap.Sample(samLinear, IN.Tex);
	
    float4 normalTex = g_Normal.Sample(samLinear, IN.Tex);
    float3 normal = normalize(normalTex.xyz * 2.0f - 1.0f);

    float4 positionTex = g_PositionMap.Sample(samLinear, IN.Tex);
    float3 worldPos = positionTex.xyz; 
       
	//normal Map calculations
    float3 vertexToEye = EyePosition - float4(worldPos.x, worldPos.y, worldPos.z, 0.0);
    float3 vertexToLight = Lights[0].Position - float4(worldPos.x, worldPos.y, worldPos.z, 0.0);
	
    float3 T = normalize(mul(IN.Tangent, worldPos));
    float3 B = normalize(mul(IN.BiTangent, worldPos));
    float3 N = normal;

    float3x3 TBN = float3x3(T, B, N);
    float3x3 TBN_inv = transpose(TBN);

    IN.lightVectorTS = VectorToTangentSpace(vertexToEye.xyz, TBN_inv);
    IN.eyeVectorTS = VectorToTangentSpace(vertexToLight.xyz, TBN_inv);
	
	
    float4 normalMap = g_NormalMap.Sample(samLinear, IN.Tex);
		
    normalMap = (normalMap * 2.0f) - 1.0f;
    normalMap = float4(normalize(normalMap.xyz), 1);

    float3 tangentSpaceNormal = normalize(normalMap.xyz);
	
    float3 lightVectorTS = normalize(IN.lightVectorTS);
    float3 eyeVectorTS = normalize(IN.eyeVectorTS);
	
    float4 specular = g_SpecularMap.Sample(samLinear, IN.Tex);


    LightingResult lr = ComputeLighting(float4(worldPos, 1.0f), tangentSpaceNormal);
    
    lr.Specular *= specular;
       
          
	// Combine with albedo + ambient + specular, etc.
    float4 litColor = saturate((Material.Ambient * GlobalAmbient + lr.Diffuse + lr.Specular) * albedo);
    
    //POST PROCESSING
    
    //Greyscale
    if (enableGreyscale != 0.0f)
    {
        float greyscale = dot(litColor.rgb, float3(0.5f, 0.5f, 0.5f)) * greyscaleStrength;
        return float4(greyscale, greyscale, greyscale, litColor.a);
    } 
    
    return litColor;
}

float4 PSDOF(QUAD_PS_INPUT IN) : SV_TARGET
{     
    
    float depth = g_DepthMap.Sample(samLinear, IN.Tex).r;
    float4 originalcolor = DiffuseMap.Sample(samLinear, IN.Tex);
    
    float4 blurredColor = GaussianBlurHorizontal(IN);
    blurredColor = GaussianBlurVerticalBlur(IN);
       
    float blurStrength = smoothstep(0.0f, 0.1f, depth);
      
    return lerp(originalcolor, blurredColor, blurStrength);
    
    
    //test out the depth outut- here i change the cahnnel to be blakc and white and not black ad red
   // return float4(depth.xxx, 1.0f);      
}