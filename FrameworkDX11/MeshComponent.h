#pragma once

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>
#include <DirectXCollision.h>
#include "DDSTextureLoader.h"
#include "resource.h"
#include <iostream>
#include "structures.h"

#include "Component.h"
#include "TransformComponent.h"
#include "TextureRendererComponent.h"

using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT3 Tangent;
	XMFLOAT3 BiTangent;
};

class MeshComponent : public Component
{
public:
	MeshComponent();
	~MeshComponent();

	void cleanup();

	HRESULT initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, const char* objFile);

	void update(float t, ID3D11DeviceContext* pContext, TransformComponent transformComponent);
	void draw(ID3D11DeviceContext* pContext, TextureRendererComponent textureRendererComponent, ID3D11ShaderResourceView* overrideTexture = nullptr);
	ID3D11Buffer* getVertexBuffer() { return m_pVertexBuffer; }
	ID3D11Buffer* getIndexBuffer() { return m_pIndexBuffer; }
	ID3D11ShaderResourceView** getTextureResourceView() { return &m_pTextureResourceView; 	}
	XMFLOAT4X4*	getTransform() { return &m_World; }
	ID3D11SamplerState** getTextureSamplerState() { return &m_pSamplerLinear; }
	ID3D11Buffer* getMaterialConstantBuffer() { return m_pMaterialConstantBuffer;}

	void setWorldMatrix(const DirectX::XMMATRIX& matrix);
	DirectX::XMMATRIX getWorldMatrix();

	void setMaterial(MaterialPropertiesConstantBuffer material) { m_material = material; }
	MaterialPropertiesConstantBuffer getMaterial() { return m_material; }

	void setSpecularPower(float power) { m_material.Material.SpecularPower = power; }

private:
	
	int verticesCount;

	XMFLOAT4X4 m_World;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11ShaderResourceView* m_pTextureResourceView;
	ID3D11SamplerState* m_pSamplerLinear;
	MaterialPropertiesConstantBuffer m_material;
	ID3D11Buffer* m_pMaterialConstantBuffer = nullptr;
};