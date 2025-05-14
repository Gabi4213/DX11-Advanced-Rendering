#pragma once

#include <DirectXMath.h>
#include <windows.h>
#include <windowsx.h>
#include "constants.h"

#include "Component.h"
#include "TransformComponent.h"

using namespace DirectX;

class CameraComponent : public Component
{
public:
    CameraComponent(XMFLOAT3 lookDirIn, XMFLOAT3 upIn)
    {
        lookDir = lookDirIn;
        up = upIn;

        XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
    }

    void UpdateLookAt(POINTS delta)
    {
        // Sensitivity factor for mouse movement
        const float sensitivity = 0.001f;
    
        // Apply sensitivity
        float dx = delta.x * sensitivity; // Yaw change
        float dy = delta.y * sensitivity; // Pitch change
    
    
        // Get the current look direction and up vector
        XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
        lookDirVec = XMVector3Normalize(lookDirVec);
        XMVECTOR upVec = XMLoadFloat3(&up);
        upVec = XMVector3Normalize(upVec);
    
        // Calculate the camera's right vector
        XMVECTOR rightVec = XMVector3Cross(upVec, lookDirVec);
        rightVec = XMVector3Normalize(rightVec);
    
    
    
            // Rotate the lookDir vector left or right based on the yaw
            lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(upVec, dx));
            lookDirVec = XMVector3Normalize(lookDirVec);
    
            // Rotate the lookDir vector up or down based on the pitch
            lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(rightVec, dy));
            lookDirVec = XMVector3Normalize(lookDirVec);
    
    
        // Re-calculate the right vector after the yaw rotation
        rightVec = XMVector3Cross(upVec, lookDirVec);
        rightVec = XMVector3Normalize(rightVec);
    
        // Re-orthogonalize the up vector to be perpendicular to the look direction and right vector
        upVec = XMVector3Cross(lookDirVec, rightVec);
        upVec = XMVector3Normalize(upVec);
    
        // Store the updated vectors back to the class members
        XMStoreFloat3(&lookDir, lookDirVec);
        XMStoreFloat3(&up, upVec);
    }

    void Update(TransformComponent transformComponent) { UpdateViewMatrix(transformComponent); }

    XMMATRIX GetViewMatrix(TransformComponent transformComponent) const
    {
        UpdateViewMatrix(transformComponent);
        return XMLoadFloat4x4(&viewMatrix);
    }

    XMVECTORF32 GetBackgroundColor()
    {
        return backgroundColor;
    }

    void SetBackgroundColor(XMVECTORF32 color)
    {
        backgroundColor = color;
    }

    void setLookDirection(XMFLOAT3 direction) { lookDir = direction; };
    XMFLOAT3 getLookDirection() { return lookDir; };

    void setUp(XMFLOAT3 inUp) { up = inUp; };
    XMFLOAT3 getUp() { return up; };

private:
    void UpdateViewMatrix(TransformComponent transformComponent) const
    {
        // Calculate the look-at point based on the position and look direction
        XMFLOAT3 m_position = transformComponent.getPosition();

        XMVECTOR posVec = XMLoadFloat3(&m_position);
        XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
        XMVECTOR lookAtPoint = posVec + lookDirVec; // This is the new look-at point

        // Update the view matrix to look from the camera's position to the look-at point
        XMStoreFloat4x4(&viewMatrix, XMMatrixLookAtLH(posVec, lookAtPoint, XMLoadFloat3(&up)));
    }

    XMFLOAT3 lookDir;
    XMFLOAT3 up;
    mutable XMFLOAT4X4 viewMatrix;

    XMVECTORF32 backgroundColor;
};

