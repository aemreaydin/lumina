[__AttributeUsage(_AttributeTargets.Var)]
struct DynamicAttribute {};

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
};

struct GBufferOutput
{
    float4 albedo : SV_Target0;   // rgb = albedo, a = metallic
    float4 normals : SV_Target1;  // xyz = world normal, w = roughness
};

struct CameraData
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4x4 inverseViewProjection;
    float4 cameraPosition;
};

ParameterBlock<CameraData> camera;

struct NodeData
{
    float4x4 model;
    float4x4 normalMatrix;
};

[Dynamic]
ParameterBlock<NodeData> node;

struct MaterialData
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float padding0;
    float3 emissiveFactor;
    uint flags;
    Sampler2D baseColorTex;
    Sampler2D normalTex;
};

ParameterBlock<MaterialData> material;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPos = mul(node.model, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(camera.viewProjection, worldPos);

    float3x3 normalMat = (float3x3)node.normalMatrix;
    output.normal = normalize(mul(normalMat, input.normal));
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
GBufferOutput fragmentMain(VertexOutput input)
{
    float4 baseColor = material.baseColorTex.Sample(input.uv) * material.baseColorFactor;

    if (baseColor.a < material.alphaCutoff)
    {
        discard;
    }

    GBufferOutput output;
    output.albedo = float4(baseColor.rgb, material.metallicFactor);
    output.normals = float4(normalize(input.normal), material.roughnessFactor);
    return output;
}
