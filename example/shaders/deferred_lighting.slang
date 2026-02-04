struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct PointLight
{
    float3 Position;
    float Radius;
    float3 Color;
    float Intensity;
};

struct DirectionalLight
{
    float3 Direction;
    float Intensity;
    float3 Color;
    float _pad;
};

struct LightingData
{
    PointLight pointLights[64];
    DirectionalLight directionalLight;
    int numPointLights;
    float3 _pad;
};

struct CameraData
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4x4 inverseViewProjection;
    float4 cameraPosition;
};

struct GBufferTextures
{
    Sampler2D albedo;
    Sampler2D normals;
    Sampler2D depth;
};

ParameterBlock<GBufferTextures> gbuffer;
ParameterBlock<LightingData> lighting;
ParameterBlock<CameraData> camera;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    VertexOutput output;
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    #ifdef VULKAN
    output.uv = float2(uv.x, 1.0 - uv.y);
    #else
    output.uv = uv;
    #endif
    return output;
}

float3 reconstructWorldPosition(float2 uv, float depth, float4x4 invVP)
{
    #ifdef VULKAN
    float4 clipPos = float4(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, depth, 1.0);
    #else
    float4 clipPos = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    #endif

    float4 worldPos = mul(invVP, clipPos);
    return worldPos.xyz / worldPos.w;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float2 uv = input.uv;

    float4 albedoSample = gbuffer.albedo.Sample(uv);
    float4 normalSample = gbuffer.normals.Sample(uv);
    float depth = gbuffer.depth.Sample(uv).r;

    float3 albedo = albedoSample.rgb;
    float metallic = albedoSample.a;
    float3 N = normalize(normalSample.xyz);
    float roughness = normalSample.a;

    float3 worldPos = reconstructWorldPosition(uv, depth, camera.inverseViewProjection);
    float3 V = normalize(camera.cameraPosition.xyz - worldPos);

    float3 Lo = float3(0.0, 0.0, 0.0);

    // Directional light
    {
        float3 L = -normalize(lighting.directionalLight.Direction);
        float NdotL = max(dot(N, L), 0.0);
        float3 diffuse = albedo * NdotL;

        float3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        float shininess = lerp(16.0, 128.0, 1.0 - roughness);
        float spec = pow(NdotH, shininess);
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
        float3 specular = F0 * spec;

        Lo += (diffuse + specular) * lighting.directionalLight.Color
            * lighting.directionalLight.Intensity;
    }

    // Point lights
    for (int i = 0; i < lighting.numPointLights; i++)
    {
        PointLight pl = lighting.pointLights[i];
        float3 toLight = pl.Position - worldPos;
        float dist = length(toLight);
        float3 L = toLight / dist;

        float attenuation = saturate(1.0 - dist / pl.Radius);
        attenuation *= attenuation;

        float NdotL = max(dot(N, L), 0.0);
        float3 diffuse = albedo * NdotL;

        float3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        float shininess = lerp(16.0, 128.0, 1.0 - roughness);
        float spec = pow(NdotH, shininess);
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
        float3 specular = F0 * spec;

        Lo += (diffuse + specular) * pl.Color * pl.Intensity * attenuation;
    }

    float3 ambient = albedo * 0.03;
    return float4(Lo + ambient, 1.0);
}
