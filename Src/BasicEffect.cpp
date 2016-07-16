//--------------------------------------------------------------------------------------
// File: BasicEffect.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EffectCommon.h"

using namespace DirectX;


// Constant buffer layout. Must match the shader!
struct BasicEffectConstants
{
    XMVECTOR diffuseColor;
    XMVECTOR emissiveColor;
    XMVECTOR specularColorAndPower;
    
    XMVECTOR lightDirection[IEffectLights::MaxDirectionalLights];
    XMVECTOR lightDiffuseColor[IEffectLights::MaxDirectionalLights];
    XMVECTOR lightSpecularColor[IEffectLights::MaxDirectionalLights];

    XMVECTOR eyePosition;

    XMVECTOR fogColor;
    XMVECTOR fogVector;

    XMMATRIX world;
    XMVECTOR worldInverseTranspose[3];
    XMMATRIX worldViewProj;
};

static_assert( ( sizeof(BasicEffectConstants) % 16 ) == 0, "CB size not padded correctly" );


// Traits type describes our characteristics to the EffectBase template.
struct BasicEffectTraits
{
    typedef BasicEffectConstants ConstantBufferType;

    static const int VertexShaderCount = 16;
    static const int PixelShaderCount = 10;
    static const int ShaderPermutationCount = 24;
};


// Internal BasicEffect implementation class.
class BasicEffect::Impl : public EffectBase<BasicEffectTraits>
{
public:
    Impl(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription);

    enum RootParameterIndex
    {
        TextureSRV,
        TextureSampler,
        ConstantBuffer,
        RootParameterCount
    };

    bool lightingEnabled;
    bool textureEnabled;

    D3D12_GPU_DESCRIPTOR_HANDLE texture;
    D3D12_GPU_DESCRIPTOR_HANDLE sampler;

    EffectLights lights;

    int GetPipelineStatePermutation(bool preferPerPixelLighting, bool vertexColorEnabled) const;

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);
};


// Include the precompiled shader code.
namespace
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasic.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicNoFog.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicVc.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicVcNoFog.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicTx.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicTxNoFog.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicTxVc.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicTxVcNoFog.inc"
    
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicVertexLighting.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicVertexLightingVc.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicVertexLightingTx.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicVertexLightingTxVc.inc"
    
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicPixelLighting.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicPixelLightingVc.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicPixelLightingTx.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_VSBasicPixelLightingTxVc.inc"

    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasic.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicNoFog.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicTx.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicTxNoFog.inc"

    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicVertexLighting.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicVertexLightingNoFog.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicVertexLightingTx.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicVertexLightingTxNoFog.inc"

    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicPixelLighting.inc"
    #include "Shaders/Compiled/XboxOneBasicEffect_PSBasicPixelLightingTx.inc"
#else
    #include "Shaders/Compiled/BasicEffect_VSBasic.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicNoFog.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicVc.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicVcNoFog.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicTx.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicTxNoFog.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicTxVc.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicTxVcNoFog.inc"
    
    #include "Shaders/Compiled/BasicEffect_VSBasicVertexLighting.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicVertexLightingVc.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicVertexLightingTx.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicVertexLightingTxVc.inc"
    
    #include "Shaders/Compiled/BasicEffect_VSBasicPixelLighting.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicPixelLightingVc.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicPixelLightingTx.inc"
    #include "Shaders/Compiled/BasicEffect_VSBasicPixelLightingTxVc.inc"

    #include "Shaders/Compiled/BasicEffect_PSBasic.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicNoFog.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicTx.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicTxNoFog.inc"

    #include "Shaders/Compiled/BasicEffect_PSBasicVertexLighting.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicVertexLightingNoFog.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicVertexLightingTx.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicVertexLightingTxNoFog.inc"

    #include "Shaders/Compiled/BasicEffect_PSBasicPixelLighting.inc"
    #include "Shaders/Compiled/BasicEffect_PSBasicPixelLightingTx.inc"
#endif
}


const D3D12_SHADER_BYTECODE EffectBase<BasicEffectTraits>::VertexShaderBytecode[] =
{
    { BasicEffect_VSBasic,                   sizeof(BasicEffect_VSBasic)                   },
    { BasicEffect_VSBasicNoFog,              sizeof(BasicEffect_VSBasicNoFog)              },
    { BasicEffect_VSBasicVc,                 sizeof(BasicEffect_VSBasicVc)                 },
    { BasicEffect_VSBasicVcNoFog,            sizeof(BasicEffect_VSBasicVcNoFog)            },
    { BasicEffect_VSBasicTx,                 sizeof(BasicEffect_VSBasicTx)                 },
    { BasicEffect_VSBasicTxNoFog,            sizeof(BasicEffect_VSBasicTxNoFog)            },
    { BasicEffect_VSBasicTxVc,               sizeof(BasicEffect_VSBasicTxVc)               },
    { BasicEffect_VSBasicTxVcNoFog,          sizeof(BasicEffect_VSBasicTxVcNoFog)          },
    
    { BasicEffect_VSBasicVertexLighting,     sizeof(BasicEffect_VSBasicVertexLighting)     },
    { BasicEffect_VSBasicVertexLightingVc,   sizeof(BasicEffect_VSBasicVertexLightingVc)   },
    { BasicEffect_VSBasicVertexLightingTx,   sizeof(BasicEffect_VSBasicVertexLightingTx)   },
    { BasicEffect_VSBasicVertexLightingTxVc, sizeof(BasicEffect_VSBasicVertexLightingTxVc) },
    
    { BasicEffect_VSBasicPixelLighting,      sizeof(BasicEffect_VSBasicPixelLighting)      },
    { BasicEffect_VSBasicPixelLightingVc,    sizeof(BasicEffect_VSBasicPixelLightingVc)    },
    { BasicEffect_VSBasicPixelLightingTx,    sizeof(BasicEffect_VSBasicPixelLightingTx)    },
    { BasicEffect_VSBasicPixelLightingTxVc,  sizeof(BasicEffect_VSBasicPixelLightingTxVc)  },
};


const int EffectBase<BasicEffectTraits>::VertexShaderIndices[] =
{
    0,      // basic
    1,      // no fog
    2,      // vertex color
    3,      // vertex color, no fog
    4,      // texture
    5,      // texture, no fog
    6,      // texture + vertex color
    7,      // texture + vertex color, no fog
    
    8,      // vertex lighting
    8,      // vertex lighting, no fog
    9,      // vertex lighting + vertex color
    9,      // vertex lighting + vertex color, no fog
    10,     // vertex lighting + texture
    10,     // vertex lighting + texture, no fog
    11,     // vertex lighting + texture + vertex color
    11,     // vertex lighting + texture + vertex color, no fog
    
    12,     // pixel lighting
    12,     // pixel lighting, no fog
    13,     // pixel lighting + vertex color
    13,     // pixel lighting + vertex color, no fog
    14,     // pixel lighting + texture
    14,     // pixel lighting + texture, no fog
    15,     // pixel lighting + texture + vertex color
    15,     // pixel lighting + texture + vertex color, no fog
};

const D3D12_SHADER_BYTECODE EffectBase<BasicEffectTraits>::PixelShaderBytecode[] =
{
    { BasicEffect_PSBasic,                      sizeof(BasicEffect_PSBasic)                      },
    { BasicEffect_PSBasicNoFog,                 sizeof(BasicEffect_PSBasicNoFog)                 },
    { BasicEffect_PSBasicTx,                    sizeof(BasicEffect_PSBasicTx)                    },
    { BasicEffect_PSBasicTxNoFog,               sizeof(BasicEffect_PSBasicTxNoFog)               },

    { BasicEffect_PSBasicVertexLighting,        sizeof(BasicEffect_PSBasicVertexLighting)        },
    { BasicEffect_PSBasicVertexLightingNoFog,   sizeof(BasicEffect_PSBasicVertexLightingNoFog)   },
    { BasicEffect_PSBasicVertexLightingTx,      sizeof(BasicEffect_PSBasicVertexLightingTx)      },
    { BasicEffect_PSBasicVertexLightingTxNoFog, sizeof(BasicEffect_PSBasicVertexLightingTxNoFog) },

    { BasicEffect_PSBasicPixelLighting,         sizeof(BasicEffect_PSBasicPixelLighting)         },
    { BasicEffect_PSBasicPixelLightingTx,       sizeof(BasicEffect_PSBasicPixelLightingTx)       },
};


const int EffectBase<BasicEffectTraits>::PixelShaderIndices[] =
{
    0,      // basic
    1,      // no fog
    0,      // vertex color
    1,      // vertex color, no fog
    2,      // texture
    3,      // texture, no fog
    2,      // texture + vertex color
    3,      // texture + vertex color, no fog
    
    4,      // vertex lighting
    5,      // vertex lighting, no fog
    4,      // vertex lighting + vertex color
    5,      // vertex lighting + vertex color, no fog
    6,      // vertex lighting + texture
    7,      // vertex lighting + texture, no fog
    6,      // vertex lighting + texture + vertex color
    7,      // vertex lighting + texture + vertex color, no fog

    8,      // pixel lighting
    8,      // pixel lighting, no fog
    8,      // pixel lighting + vertex color
    8,      // pixel lighting + vertex color, no fog
    9,      // pixel lighting + texture
    9,      // pixel lighting + texture, no fog
    9,      // pixel lighting + texture + vertex color
    9,      // pixel lighting + texture + vertex color, no fog
};

// Global pool of per-device BasicEffect resources.
SharedResourcePool<ID3D12Device*, EffectBase<BasicEffectTraits>::DeviceResources> EffectBase<BasicEffectTraits>::deviceResourcesPool;


// Constructor.
BasicEffect::Impl::Impl(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription)
    : EffectBase(device),
    texture{},
    sampler{}
{
    static_assert( _countof(EffectBase<BasicEffectTraits>::VertexShaderIndices) == BasicEffectTraits::ShaderPermutationCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<BasicEffectTraits>::VertexShaderBytecode) == BasicEffectTraits::VertexShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<BasicEffectTraits>::PixelShaderBytecode) == BasicEffectTraits::PixelShaderCount, "array/max mismatch" );
    static_assert( _countof(EffectBase<BasicEffectTraits>::PixelShaderIndices) == BasicEffectTraits::ShaderPermutationCount, "array/max mismatch" );

    lights.InitializeConstants(constants.specularColorAndPower, constants.lightDirection, constants.lightDiffuseColor, constants.lightSpecularColor);

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount];
    rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV); 
    rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler); 
    rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
    rsigDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ThrowIfFailed(CreateRootSignature(device, &rsigDesc, mRootSignature.ReleaseAndGetAddressOf()));

    fog.enabled = (effectFlags & EffectFlags::Fog) != 0;
    lightingEnabled = (effectFlags & EffectFlags::Lighting) != 0;
    textureEnabled = (effectFlags & EffectFlags::Texture) != 0;
   
    int sp = GetPipelineStatePermutation(
        (effectFlags & EffectFlags::PerPixelLightingBit) != 0,
        (effectFlags & EffectFlags::VertexColor) != 0);
    assert(sp >= 0 && sp < BasicEffectTraits::ShaderPermutationCount);

    int vi = EffectBase<BasicEffectTraits>::VertexShaderIndices[sp];
    assert(vi >= 0 && vi < BasicEffectTraits::VertexShaderCount);
    int pi = EffectBase<BasicEffectTraits>::PixelShaderIndices[sp];
    assert(pi >= 0 && pi < BasicEffectTraits::PixelShaderCount);

    EffectBase::CreatePipelineState(
        mRootSignature.Get(),
        pipelineDescription.inputLayout,
        &EffectBase<BasicEffectTraits>::VertexShaderBytecode[vi],
        &EffectBase<BasicEffectTraits>::PixelShaderBytecode[pi],
        pipelineDescription.blendDesc,
        pipelineDescription.depthStencilDesc,
        pipelineDescription.rasterizerDesc,
        pipelineDescription.renderTargetState,
        pipelineDescription.primitiveTopology,
        pipelineDescription.stripCutValue);
}


int BasicEffect::Impl::GetPipelineStatePermutation(bool preferPerPixelLighting, bool vertexColorEnabled) const
{
    int permutation = 0;

    // Use optimized shaders if fog is disabled.
    if (!fog.enabled)
    {
        permutation += 1;
    }

    // Support vertex coloring?
    if (vertexColorEnabled)
    {
        permutation += 2;
    }

    // Support texturing?
    if (textureEnabled)
    {
        permutation += 4;
    }

    if (lightingEnabled)
    {
        if (preferPerPixelLighting)
        {
            // Do lighting in the pixel shader.
            permutation += 16;
        }
        else
        {
            permutation += 8;
        }
    }

    return permutation;
}


// Sets our state onto the D3D device.
void BasicEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Compute derived parameter values.
    matrices.SetConstants(dirtyFlags, constants.worldViewProj);
    fog.SetConstants(dirtyFlags, matrices.worldView, constants.fogVector);
    lights.SetConstants(dirtyFlags, matrices, constants.world, constants.worldInverseTranspose, constants.eyePosition, constants.diffuseColor, constants.emissiveColor, lightingEnabled);
    
    UpdateConstants();

    // Set the root signature
    commandList->SetGraphicsRootSignature(mRootSignature.Get());

    // Set the texture
    // **NOTE** If D3D asserts or crashes here, you probably need to call commandList->SetDescriptorHeaps() with the required descriptor heaps.
    if (textureEnabled)
    {
        if (!texture.ptr || !sampler.ptr)
        {
            DebugTrace("ERROR: Missing texture or sampler for BasicEffect (texture %llu, sampler %llu)\n", texture.ptr, sampler.ptr);
            throw std::exception("BasicEffect");
        }
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, texture);
        commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSampler, sampler);
    }

    // Set constants
    commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, GetConstantBufferGpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(EffectBase::mPipelineState.Get());
}


// Public constructor.
BasicEffect::BasicEffect(_In_ ID3D12Device* device, int effectFlags, const EffectPipelineStateDescription& pipelineDescription)
  : pImpl(new Impl(device, effectFlags, pipelineDescription))
{
}


// Move constructor.
BasicEffect::BasicEffect(BasicEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
BasicEffect& BasicEffect::operator= (BasicEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
BasicEffect::~BasicEffect()
{
}


// IEffect methods
void BasicEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}


// Camera settings
void XM_CALLCONV BasicEffect::SetWorld(FXMMATRIX value)
{
    pImpl->matrices.world = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV BasicEffect::SetView(FXMMATRIX value)
{
    pImpl->matrices.view = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


void XM_CALLCONV BasicEffect::SetProjection(FXMMATRIX value)
{
    pImpl->matrices.projection = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj;
}


void XM_CALLCONV BasicEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    pImpl->matrices.world = world;
    pImpl->matrices.view = view;
    pImpl->matrices.projection = projection;

    pImpl->dirtyFlags |= EffectDirtyFlags::WorldViewProj | EffectDirtyFlags::WorldInverseTranspose | EffectDirtyFlags::EyePosition | EffectDirtyFlags::FogVector;
}


// Material settings
void XM_CALLCONV BasicEffect::SetDiffuseColor(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV BasicEffect::SetEmissiveColor(FXMVECTOR value)
{
    pImpl->lights.emissiveColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV BasicEffect::SetSpecularColor(FXMVECTOR value)
{
    // Set xyz to new value, but preserve existing w (specular power).
    pImpl->constants.specularColorAndPower = XMVectorSelect(pImpl->constants.specularColorAndPower, value, g_XMSelect1110);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void BasicEffect::SetSpecularPower(float value)
{
    // Set w to new value, but preserve existing xyz (specular color).
    pImpl->constants.specularColorAndPower = XMVectorSetW(pImpl->constants.specularColorAndPower, value);

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void BasicEffect::DisableSpecular()
{
    // Set specular color to black, power to 1
    // Note: Don't use a power of 0 or the shader will generate strange highlights on non-specular materials

    pImpl->constants.specularColorAndPower = g_XMIdentityR3; 

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void BasicEffect::SetAlpha(float value)
{
    pImpl->lights.alpha = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void XM_CALLCONV BasicEffect::SetColorAndAlpha(FXMVECTOR value)
{
    pImpl->lights.diffuseColor = value;
    pImpl->lights.alpha = XMVectorGetW(value);

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


// Light settings
void XM_CALLCONV BasicEffect::SetAmbientLightColor(FXMVECTOR value)
{
    pImpl->lights.ambientLightColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::MaterialColor;
}


void BasicEffect::SetLightEnabled(int whichLight, bool value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightEnabled(whichLight, value, pImpl->constants.lightDiffuseColor, pImpl->constants.lightSpecularColor);
}


void XM_CALLCONV BasicEffect::SetLightDirection(int whichLight, FXMVECTOR value)
{
    EffectLights::ValidateLightIndex(whichLight);

    pImpl->constants.lightDirection[whichLight] = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


void XM_CALLCONV BasicEffect::SetLightDiffuseColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightDiffuseColor(whichLight, value, pImpl->constants.lightDiffuseColor);
}


void XM_CALLCONV BasicEffect::SetLightSpecularColor(int whichLight, FXMVECTOR value)
{
    pImpl->dirtyFlags |= pImpl->lights.SetLightSpecularColor(whichLight, value, pImpl->constants.lightSpecularColor);
}


void BasicEffect::EnableDefaultLighting()
{
    EffectLights::EnableDefaultLighting(this);
}


// Fog settings.
void BasicEffect::SetFogStart(float value)
{
    pImpl->fog.start = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void BasicEffect::SetFogEnd(float value)
{
    pImpl->fog.end = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::FogVector;
}


void XM_CALLCONV BasicEffect::SetFogColor(FXMVECTOR value)
{
    pImpl->constants.fogColor = value;

    pImpl->dirtyFlags |= EffectDirtyFlags::ConstantBuffer;
}


// Texture settings.
void BasicEffect::SetTexture(_In_ D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, _In_ D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor)
{
    pImpl->texture = srvDescriptor;
    pImpl->sampler = samplerDescriptor;
}
