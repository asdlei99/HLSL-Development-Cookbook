#pragma once

#include <vector>
#include "CascadedMatrixSet.h"

class CLightManager
{
public:

	// Maximum supported point shadowmaps
	static const int m_iTotalPointShadowmaps = 3;

	CLightManager();
	~CLightManager();

	HRESULT Init();
	void Deinit();

	void Update();

	// Set the ambient values
	void SetAmbient(const D3DXVECTOR3& vAmbientLowerColor, const D3DXVECTOR3& vAmbientUpperColor)
	{
		m_vAmbientLowerColor = vAmbientLowerColor;
		m_vAmbientUpperColor = vAmbientUpperColor;
	}

	// Set the directional light values
	void SetDirectional(const D3DXVECTOR3& vDirectionalDir, const D3DXVECTOR3& vDirectionalCorol, bool bCastShadow, bool bAntiFlickerOn)
	{
		D3DXVec3Normalize( &m_vDirectionalDir, &vDirectionalDir ); 
		m_vDirectionalColor = vDirectionalCorol;
		m_bDirectionalCastShadow = bCastShadow;

		m_CascadedMatrixSet.SetAntiFlicker(bAntiFlickerOn);
	}

	// Clear the lights from the previous frame
	void ClearLights() { m_arrLights.clear(); m_iLastShadowLight = -1; m_iNextFreeSpotShadowmap = -1; m_iNextFreePointShadowmap = -1; }

	// Add a single point light
	void AddPointLight(const D3DXVECTOR3& vPointPosition, float fPointRange, const D3DXVECTOR3& vPointColor, bool bCastShadow)
	{
		LIGHT pointLight;

		pointLight.eLightType = TYPE_POINT;
		pointLight.vPosition = vPointPosition;
		pointLight.fRange = fPointRange;
		pointLight.vColor = vPointColor;
		pointLight.iShadowmapIdx = -1;
		pointLight.iShadowmapIdx = bCastShadow ? GetNextFreePointShadowmapIdx() : -1;

		m_arrLights.push_back(pointLight);
	}

	void AddSpotLight(const D3DXVECTOR3& vSpotPosition, const D3DXVECTOR3& vSpotDirection, float fSpotRange,
		float fSpotOuterAngle, float fSpotInnerAngle, const D3DXVECTOR3& vSpotColor, bool bCastShadow)
	{
		LIGHT spotLight;

		spotLight.eLightType = TYPE_SPOT;
		spotLight.vPosition = vSpotPosition;
		spotLight.vDirection = vSpotDirection;
		spotLight.fRange = fSpotRange;
		spotLight.fOuterAngle = D3DX_PI * fSpotOuterAngle / 180.0f;
		spotLight.fInnerAngle = D3DX_PI * fSpotInnerAngle / 180.0f;
		spotLight.vColor = vSpotColor;
		spotLight.iShadowmapIdx = bCastShadow ? GetNextFreeSpotShadowmapIdx() : -1;

		m_arrLights.push_back(spotLight);
	}

	void AddCapsuleLight(const D3DXVECTOR3& vCapsulePosition, const D3DXVECTOR3& vCapsuleDirection, float fCapsuleRange,
		float fCapsuleLength, const D3DXVECTOR3& vCapsuleColor)
	{
		LIGHT capsuleLight;

		capsuleLight.eLightType = TYPE_CAPSULE;
		capsuleLight.vPosition = vCapsulePosition;
		capsuleLight.vDirection = vCapsuleDirection;
		capsuleLight.fRange = fCapsuleRange;
		capsuleLight.fLength = fCapsuleLength;
		capsuleLight.vColor = vCapsuleColor;
		capsuleLight.iShadowmapIdx = -1;

		m_arrLights.push_back(capsuleLight);
	}

	void DoLighting(ID3D11DeviceContext* pd3dImmediateContext);

	// Render each light volume in wireframe mode
	void DoDebugLightVolume(ID3D11DeviceContext* pd3dImmediateContext);

	// Color the pixels affected by each cascade
	void DoDebugCascadedShadows(ID3D11DeviceContext* pd3dImmediateContext);

	// Prepare shadow generation for the next shadow casting light
	bool PrepareNextShadowLight(ID3D11DeviceContext* pd3dImmediateContext);

private:

	typedef enum
	{
		TYPE_POINT = 0,
		TYPE_SPOT,
		TYPE_CAPSULE
	} LIGHT_TYPE;

	// Light storage
	typedef struct
	{
		LIGHT_TYPE eLightType;
		D3DXVECTOR3 vPosition;
		D3DXVECTOR3 vDirection;
		float fRange;
		float fLength;
		float fOuterAngle;
		float fInnerAngle;
		D3DXVECTOR3 vColor;
		int iShadowmapIdx;
	} LIGHT;

	// Do the directional light calculation
	void DirectionalLight(ID3D11DeviceContext* pd3dImmediateContext);

	// Based on the value of bWireframe, either do the lighting or render the volume
	void PointLight(ID3D11DeviceContext* pd3dImmediateContext, const D3DXVECTOR3& vPos ,float fRange, const D3DXVECTOR3& vColor, int iShadowmapIdx, bool bWireframe);
	
	// Based on the value of bWireframe, either do the lighting or render the volume
	void SpotLight(ID3D11DeviceContext* pd3dImmediateContext, const D3DXVECTOR3& vPos, const D3DXVECTOR3& vDir, float fRange, float fInnerAngle, float fOuterAngle, const D3DXVECTOR3& vColor, int iShadowmapIdx, bool bWireframe);

	// Based on the value of bWireframe, either do the lighting or render the volume
	void CapsuleLight(ID3D11DeviceContext* pd3dImmediateContext, const D3DXVECTOR3& vPos, const D3DXVECTOR3& vDir, float fRange, float fLen, const D3DXVECTOR3& vColor, bool bWireframe);

	// Get a spot shadowmap index - first come first served
	int GetNextFreeSpotShadowmapIdx() { return (m_iNextFreeSpotShadowmap + 1 < m_iTotalSpotShadowmaps) ? ++m_iNextFreeSpotShadowmap : -1; }

	// Prepare a spot shadowmap for casters rendering
	void SpotShadowGen(ID3D11DeviceContext* pd3dImmediateContext, const LIGHT& light);

	// Get a point shadowmap index - first come first served
	int GetNextFreePointShadowmapIdx() { return (m_iNextFreePointShadowmap + 1 < m_iTotalPointShadowmaps) ? ++m_iNextFreePointShadowmap : -1; }

	// Prepare a point shadowmap for casters rendering
	void PointShadowGen(ID3D11DeviceContext* pd3dImmediateContext, const LIGHT& light);

	// Prepare cascaded shadow maps for casters rendering
	void CascadedShadowsGen(ID3D11DeviceContext* pd3dImmediateContext);

	// Directional light
	ID3D11VertexShader* m_pDirLightVertexShader;
	ID3D11PixelShader* m_pDirLightPixelShader;
	ID3D11PixelShader* m_pDirLightShadowPixelShader;
	ID3D11Buffer* m_pDirLightCB;

	// Point light
	ID3D11VertexShader* m_pPointLightVertexShader;
	ID3D11HullShader* m_pPointLightHullShader;
	ID3D11DomainShader* m_pPointLightDomainShader;
	ID3D11PixelShader* m_pPointLightPixelShader;
	ID3D11PixelShader* m_pPointLightShadowPixelShader;
	ID3D11Buffer* m_pPointLightDomainCB;
	ID3D11Buffer* m_pPointLightPixelCB;

	// Spot light
	ID3D11VertexShader* m_pSpotLightVertexShader;
	ID3D11HullShader* m_pSpotLightHullShader;
	ID3D11DomainShader* m_pSpotLightDomainShader;
	ID3D11PixelShader* m_pSpotLightPixelShader;
	ID3D11PixelShader* m_pSpotLightShadowPixelShader;
	ID3D11Buffer* m_pSpotLightDomainCB;
	ID3D11Buffer* m_pSpotLightPixelCB;

	// Capsule light
	ID3D11VertexShader* m_pCapsuleLightVertexShader;
	ID3D11HullShader* m_pCapsuleLightHullShader;
	ID3D11DomainShader* m_pCapsuleLightDomainShader;
	ID3D11PixelShader* m_pCapsuleLightPixelShader;
	ID3D11Buffer* m_pCapsuleLightDomainCB;
	ID3D11Buffer* m_pCapsuleLightPixelCB;

	// Shadowmap generation layout
	ID3D11InputLayout* m_pShadowGenVSLayout;

	// Spot generation assets
	ID3D11VertexShader* m_pSpotShadowGenVertexShader;
	ID3D11Buffer* m_pSpotShadowGenVertexCB;

	// Point generation assets
	ID3D11VertexShader* m_pPointShadowGenVertexShader;
	ID3D11GeometryShader* m_pPointShadowGenGeometryShader;
	ID3D11Buffer* m_pPointShadowGenGeometryCB;

	// Cascaded shadow maps generation assets
	ID3D11VertexShader* m_pCascadedShadowGenVertexShader;
	ID3D11GeometryShader* m_pCascadedShadowGenGeometryShader;
	ID3D11Buffer* m_pCascadedShadowGenGeometryCB;

	// Light volume debug shader
	ID3D11PixelShader* m_pDebugLightPixelShader;

	// Cascaded shadows debug shader
	ID3D11PixelShader* m_pDebugCascadesPixelShader;

	// Depth state with no writes and stencil test on
	ID3D11DepthStencilState* m_pNoDepthWriteLessStencilMaskState;
	ID3D11DepthStencilState* m_pNoDepthWriteGreatherStencilMaskState;

	// Depth state for shadow generation
	ID3D11DepthStencilState* m_pShadowGenDepthState;

	// Additive blend state to accumulate light influence
	ID3D11BlendState* m_pAdditiveBlendState;

	// Front face culling for lights volume
	ID3D11RasterizerState* m_pNoDepthClipFrontRS;

	// Wireframe render state for light volume debugging
	ID3D11RasterizerState* m_pWireframeRS;

	// Depth bias for shadow generation
	ID3D11RasterizerState* m_pShadowGenRS;
	ID3D11RasterizerState* m_pCascadedShadowGenRS;

	// PCF sampler state for shadow mapping
	ID3D11SamplerState* m_pPCFSamplerState;

	// Visualize the lights volume
	bool m_bShowLightVolume;

	// Near plane distance for shadow map generation
	static const float m_fShadowNear;

	// Index to the last shadow casting light a map was generated for
	int m_iLastShadowLight;

	// Index to the next available spot shadowmap
	int m_iNextFreeSpotShadowmap;

	// Size in pixels of the shadow map
	static const int m_iShadowMapSize = 1024;

	// Maximum supported spot shadowmaps
	static const int m_iTotalSpotShadowmaps = 3;

	// Spot light shadowmap resources
	ID3D11Texture2D* m_SpotDepthStencilRT[m_iTotalSpotShadowmaps];
	ID3D11DepthStencilView* m_SpotDepthStencilDSV[m_iTotalSpotShadowmaps];
	ID3D11ShaderResourceView* m_SpotDepthStencilSRV[m_iTotalSpotShadowmaps];

	// Index to the next available point shadowmap
	int m_iNextFreePointShadowmap;

	// Point light shadowmap resources
	ID3D11Texture2D* m_PointDepthStencilRT[m_iTotalPointShadowmaps];
	ID3D11DepthStencilView* m_PointDepthStencilDSV[m_iTotalSpotShadowmaps];
	ID3D11ShaderResourceView* m_PointDepthStencilSRV[m_iTotalSpotShadowmaps];

	// Cascaded shadow maps
	ID3D11Texture2D* m_pCascadedDepthStencilRT;
	ID3D11DepthStencilView* m_pCascadedDepthStencilDSV;
	ID3D11ShaderResourceView* m_pCascadedDepthStencilSRV;

	// Cascaded shadow maps matrices
	CCascadedMatrixSet m_CascadedMatrixSet;

	// Ambient light information
	D3DXVECTOR3 m_vAmbientLowerColor;
	D3DXVECTOR3 m_vAmbientUpperColor;

	// Directional light information
	D3DXVECTOR3 m_vDirectionalDir;
	D3DXVECTOR3 m_vDirectionalColor;
	bool m_bDirectionalCastShadow;

	// Linked list with the active lights
	std::vector<LIGHT> m_arrLights;
};