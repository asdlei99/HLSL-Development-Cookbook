#include "DXUT.h"
#include "SDKmisc.h"
#include "DXUTCamera.h"
#include "SDKmisc.h"
#include "LightManager.h"

extern ID3D11Device* g_pDevice;
extern CFirstPersonCamera g_Camera;

// Helpers
HRESULT CompileShader(PWCHAR strPath, D3D10_SHADER_MACRO* pMacros, char* strEntryPoint, char* strProfile, DWORD dwShaderFlags, ID3DBlob** ppVertexShaderBuffer);
const D3DXVECTOR3 GammaToLinear(const D3DXVECTOR3& color)
{
	return D3DXVECTOR3(color.x * color.x, color.y * color.y, color.z * color.z);
}

#pragma pack(push,1)
struct CB_DIRECTIONAL
{
    D3DXVECTOR3 vAmbientLower;
	float pad;
    D3DXVECTOR3 vAmbientRange;
	float pad2;
	D3DXVECTOR3 vDirToLight;
	float pad3;
	D3DXVECTOR3 vDirectionalColor;
	float pad4;
};
#pragma pack(pop)

CLightManager::CLightManager() : m_pForwardLightVertexShader(NULL), m_pForwardLightVSLayout(NULL),
	m_pDirectionalLightPixelShader(NULL), m_pDirectionalLightPixelCB(NULL), m_pForwardLightDS(NULL)
{
}

CLightManager::~CLightManager()
{

}

HRESULT CLightManager::Init()
{
	HRESULT hr;

	// Create constant buffers
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory( &cbDesc, sizeof(cbDesc) );
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof( CB_DIRECTIONAL );
    V_RETURN( g_pDevice->CreateBuffer( &cbDesc, NULL, &m_pDirectionalLightPixelCB ) );
    DXUT_SetDebugName( m_pDirectionalLightPixelCB, "Directional Light CB" );

	// Read the HLSL file
	WCHAR str[MAX_PATH];
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"ForwardLight.hlsl" ) );

    // Compile the shaders
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	// Load the ambient light shaders
	ID3DBlob* pShaderBlob = NULL;
    V_RETURN( CompileShader(str, NULL, "RenderSceneVS", "vs_5_0", dwShaderFlags, &pShaderBlob) );
	V_RETURN( g_pDevice->CreateVertexShader( pShaderBlob->GetBufferPointer(),
                                              pShaderBlob->GetBufferSize(), NULL, &m_pForwardLightVertexShader ) );
    DXUT_SetDebugName( m_pForwardLightVertexShader, "Forward Light VS" );
	
	// Create a layout for the object data
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN( g_pDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pShaderBlob->GetBufferPointer(),
                                             pShaderBlob->GetBufferSize(), &m_pForwardLightVSLayout ) );
    DXUT_SetDebugName( m_pForwardLightVSLayout, "Forward Light Vertex Layout" );
	SAFE_RELEASE( pShaderBlob );

    V_RETURN( CompileShader(str, NULL, "DirectionalLightPS", "ps_5_0", dwShaderFlags, &pShaderBlob) );
    V_RETURN( g_pDevice->CreatePixelShader( pShaderBlob->GetBufferPointer(),
                                             pShaderBlob->GetBufferSize(), NULL, &m_pDirectionalLightPixelShader ) );
    DXUT_SetDebugName( m_pDirectionalLightPixelShader, "Directional Light PS" );
	SAFE_RELEASE( pShaderBlob );

	// Prepare the depth state
	D3D11_DEPTH_STENCIL_DESC descDepth;
	descDepth.DepthEnable = TRUE;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	descDepth.StencilEnable = FALSE;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC noSkyStencilOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL };
	descDepth.FrontFace = noSkyStencilOp;
	descDepth.BackFace = noSkyStencilOp;
	V_RETURN( g_pDevice->CreateDepthStencilState(&descDepth, &m_pForwardLightDS) );
	DXUT_SetDebugName( m_pForwardLightDS, "Less Than Equals Depth DS" );

	return hr;
}

void CLightManager::DeInit()
{
	SAFE_RELEASE( m_pForwardLightVertexShader );
	SAFE_RELEASE( m_pForwardLightVSLayout );
	SAFE_RELEASE( m_pDirectionalLightPixelShader );
	SAFE_RELEASE( m_pDirectionalLightPixelCB );
	SAFE_RELEASE( m_pForwardLightDS );
}

void CLightManager::ForwardSetup(ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr;

	pd3dImmediateContext->OMSetDepthStencilState(m_pForwardLightDS, 0);

	// Fill the directional and ambient values constant buffer
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V( pd3dImmediateContext->Map( m_pDirectionalLightPixelCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_DIRECTIONAL* pDirectionalValuesCB = ( CB_DIRECTIONAL* )MappedResource.pData;
	pDirectionalValuesCB->vAmbientLower = GammaToLinear(m_vAmbientLowerColor);
    pDirectionalValuesCB->vAmbientRange = GammaToLinear(m_vAmbientUpperColor) - GammaToLinear(m_vAmbientLowerColor);
	pDirectionalValuesCB->vDirToLight = -m_vDirectionalDir;
	pDirectionalValuesCB->vDirectionalColor = GammaToLinear(m_vDirectionalColor);
    pd3dImmediateContext->Unmap( m_pDirectionalLightPixelCB, 0 );

	// Set the ambient values as the second constant buffer
    pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_pDirectionalLightPixelCB );

	// Set the vertex layout
	pd3dImmediateContext->IASetInputLayout( m_pForwardLightVSLayout );

	// Set the shaders
	pd3dImmediateContext->VSSetShader(m_pForwardLightVertexShader, NULL, 0);
	pd3dImmediateContext->PSSetShader(m_pDirectionalLightPixelShader, NULL, 0);
}

