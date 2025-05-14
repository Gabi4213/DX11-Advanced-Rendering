#pragma once
#include "Component.h"
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>
#include <iostream>

#include "DDSTextureLoader.h"

using namespace DirectX;

class TextureRendererComponent : public Component
{
public:
	TextureRendererComponent()
	{
		m_albedoTexture = nullptr;
		m_normalTexture = nullptr;
		m_specularTexture = nullptr;
	};

	void setAlbedoTexture(ID3D11Device* pd3dDevice, const std::wstring& filename)
	{
		CreateDDSTextureFromFile(pd3dDevice, filename.c_str(), nullptr, &m_albedoTexture);
	};

	void setNormalTexture(ID3D11Device* pd3dDevice, const std::wstring& filename)
	{
		CreateDDSTextureFromFile(pd3dDevice, filename.c_str(), nullptr, &m_normalTexture);
	};

	void setSpecularTexture(ID3D11Device* pd3dDevice, const std::wstring& filename)
	{
		CreateDDSTextureFromFile(pd3dDevice, filename.c_str(), nullptr, &m_specularTexture);
	};

	ID3D11ShaderResourceView* getAlbedoTexture() { return m_albedoTexture; };

	ID3D11ShaderResourceView* getNormalTexture(){ return m_normalTexture; };

	ID3D11ShaderResourceView* getSpecularTexture(){ return m_specularTexture; };

private:
	ID3D11ShaderResourceView* m_albedoTexture;
	ID3D11ShaderResourceView* m_normalTexture;
	ID3D11ShaderResourceView* m_specularTexture;
};