#pragma once
#include "Component.h"
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>

using namespace DirectX;

class TransformComponent : public Component
{
public:
	TransformComponent()
	{
		m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	};

	void setPosition(XMFLOAT3 position) { m_position = position; };
	void setRotation(XMFLOAT3 rotation) { m_rotation = rotation; };
	void setScale(XMFLOAT3 scale) { m_scale = scale; };

	XMFLOAT3 getPosition() { return m_position; };
	XMFLOAT3 getRotation() { return m_rotation; };
	XMFLOAT3 getScale() { return m_scale; };

private:
	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;
	XMFLOAT3 m_scale;
};