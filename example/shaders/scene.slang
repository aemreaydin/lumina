struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// Set 0: Camera data
struct CameraData
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4 cameraPosition;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraData> camera;

// Set 1: Node/Transform data
struct NodeData
{
    float4x4 model;
    float4x4 normalMatrix;
};

[[vk::push_constant]]
ConstantBuffer<NodeData> node;

// Set 2: Material data
struct MaterialProperties
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float padding0;
    float3 emissiveFactor;
    uint flags;
};

[[vk::binding(0, 1)]]
ConstantBuffer<MaterialProperties> material;

[[vk::binding(1, 1)]]
Sampler2D baseColorTex;

[[vk::binding(2, 1)]]
Sampler2D normalTex;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPos = mul(node.model, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(camera.viewProjection, worldPos);

    // Transform normal to world space
    float3x3 normalMat = (float3x3)node.normalMatrix;
    output.normal = normalize(mul(normalMat, input.normal));
    output.tangent = normalize(mul(normalMat, input.tangent.xyz));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;

    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Sample textures
    float4 baseColor = baseColorTex.Sample(input.uv) * material.baseColorFactor;

    // Alpha test
    if (baseColor.a < material.alphaCutoff)
    {
        discard;
    }

    // Sample and decode normal map
    float3 normalMapSample = normalTex.Sample(input.uv).xyz;
    normalMapSample = normalMapSample * 2.0 - 1.0;

    // Construct TBN matrix
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3x3 TBN = float3x3(T, B, N);

    // Transform normal to world space
    float3 worldNormal = normalize(mul(normalMapSample, TBN));

    // Simple directional light for now
    float3 lightDir = normalize(float3(1.0, 1.0, 0.5));
    float3 viewDir = normalize(camera.cameraPosition.xyz - input.worldPos);
    float3 halfDir = normalize(lightDir + viewDir);

    // Lambertian diffuse
    float NdotL = max(dot(worldNormal, lightDir), 0.0);
    float3 diffuse = baseColor.rgb * NdotL;

    // Simple specular (Blinn-Phong approximation)
    float NdotH = max(dot(worldNormal, halfDir), 0.0);
    float shininess = lerp(4.0, 256.0, 1.0 - material.roughnessFactor);
    float spec = pow(NdotH, shininess);
    float3 specular = float3(spec) * material.metallicFactor;

    // Ambient
    float3 ambient = baseColor.rgb * 0.1;

    // Emissive
    float3 emissive = material.emissiveFactor;

    float3 finalColor = ambient + diffuse + specular + emissive;

    return float4(finalColor, baseColor.a);
}
