#pragma once
#include "Component.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>

using namespace DirectX;

#define MAX_LIGHTS 4

class LightComponent : public Component
{
public:
	LightComponent() {};

	void setColor(DirectX::XMFLOAT4 color) { m_Light.Color = color; };
	DirectX::XMFLOAT4 getColor() { return m_Light.Color; };

	void setPosition(DirectX::XMFLOAT4 position) { m_Light.Position = position; };
	DirectX::XMFLOAT4 getPosition() { return m_Light.Position; };

	void setDirection(DirectX::XMFLOAT4 direction) { m_Light.Direction = direction; };
	DirectX::XMFLOAT4 getDirection() { return m_Light.Direction; };


	void setSpotAngle(float spotAngle) { m_Light.SpotAngle = spotAngle; };
	float getSpotAngle() { return m_Light.SpotAngle; };

	void setConstantAttenuation(float constantAttenuation) { m_Light.ConstantAttenuation = constantAttenuation; };
	float getConstantAttenuation() { return m_Light.ConstantAttenuation; };

	void setLinearAttenuation(float linearAttenuation) { m_Light.LinearAttenuation = linearAttenuation; };
	float getLinearAttenuation() { return m_Light.LinearAttenuation; };

	void setQuadraticAttenuation(float quadraticAttenuation) { m_Light.QuadraticAttenuation = quadraticAttenuation; };
	float getQuadraticAttenuation() { return m_Light.QuadraticAttenuation; };

	void setLightType(int lightType) { m_Light.LightType = lightType; };
	int getLightType() { return m_Light.LightType; };

	void setEnabled(int enabled) { m_Light.Enabled = enabled; };
	int getEnabled() { return m_Light.Enabled; };

	struct Light
	{
		Light()
			: Position(0.0f, 0.0f, 0.0f, 1.0f)
			, Direction(0.0f, 0.0f, 1.0f, 0.0f)
			, Color(1.0f, 1.0f, 1.0f, 1.0f)
			, SpotAngle(DirectX::XM_PIDIV2)
			, ConstantAttenuation(1.0f)
			, LinearAttenuation(0.0f)
			, QuadraticAttenuation(0.0f)
			, LightType(0) // 0 is directiona;
			, Enabled(0)
		{}

		DirectX::XMFLOAT4 Position;
		DirectX::XMFLOAT4 Direction;
		DirectX::XMFLOAT4 Color;

		float SpotAngle;
		float ConstantAttenuation;
		float LinearAttenuation;
		float QuadraticAttenuation;

		int LightType;
		int Enabled;
		int Padding[2];
	}; 

	Light m_Light;
};