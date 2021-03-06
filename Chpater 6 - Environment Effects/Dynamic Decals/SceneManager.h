#pragma once

#include "SDKmesh.h"

class CSceneManager
{
public:

	CSceneManager();
	~CSceneManager();

	HRESULT Init();
	void Deinit();

	// Render the scene meshes into the GBuffer
	void RenderSceneToGBuffer(ID3D11DeviceContext* pd3dImmediateContext);

	// Render the scene with no shaders
	void RenderSceneNoShaders(ID3D11DeviceContext* pd3dImmediateContext);

	// Render the related elements
	void RenderSky(ID3D11DeviceContext* pd3dImmediateContext, const D3DXVECTOR3& vSunDir, const D3DXVECTOR3& vSunColor);

	// Find the closest intersection between a ray and the opaque mesh
	bool RayIntersection(const D3DXVECTOR3& vRayPos, const D3DXVECTOR3& vRayDir, D3DXVECTOR3& vHitPos, D3DXVECTOR3& vHitNorm, CDXUTSDKMesh*& hitMesh, UINT& nMeshIdx, UINT& nSubMeshIdx);

	void SetSceneInputLayout(ID3D11DeviceContext* pd3dImmediateContext);

private:

	// Test if a ray intersects with a given triangle
	static bool RayTriangleIntersection(const D3DXVECTOR3& vRayPos, const D3DXVECTOR3& vRayDir, const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2, D3DXVECTOR3& vHitPos, D3DXVECTOR3& vHitNorm, float& fT);

	// Scene meshes
	CDXUTSDKMesh m_MeshOpaque;
	CDXUTSDKMesh m_MeshSphere;

	float m_fSunRadius;

	// Deferred shading shaders
	ID3D11Buffer* m_pSceneVertexShaderCB;
	ID3D11Buffer* m_pScenePixelShaderCB;
	ID3D11VertexShader* m_pSceneVertexShader;
	ID3D11InputLayout* m_pSceneVSLayout;
	ID3D11PixelShader* m_pScenePixelShader;

	// Emissive shaders
	ID3D11Buffer* m_pEmissiveCB;
	ID3D11VertexShader* m_pEmissiveVertexShader;
	ID3D11InputLayout* m_pEmissiveVSLayout;
	ID3D11PixelShader* m_pEmissivePixelShader;

	// No depth with sky stencil test
	ID3D11DepthStencilState* m_pSkyNoDepthStencilMaskState;
};