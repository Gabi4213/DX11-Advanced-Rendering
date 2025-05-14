#pragma once
#include "Component.h"
#include "TransformComponent.h"

class AnimationComponent : public Component
{
public:

    AnimationComponent(TransformComponent* transformComponent)
    {
        transform = transformComponent;
        durationTime = 0.0f;

        //Position
        isPositionAnimating = false;
        startPosition = transform->getPosition();
        targetPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
        positionTime = 0.0f;

        //Rotation
        isRotationAnimating = false;
        startRotation = transform->getRotation();
        targetRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
        rotationTime = 0.0f;
    }

    void animatePosition(XMFLOAT3 newPosition, float duration)
    {
        startPosition = transform->getPosition();
        targetPosition = newPosition;
        durationTime = duration;

        positionTime = 0.0f;
        isPositionAnimating = true;
    }

    void animateRotation(XMFLOAT3 newRotation, float duration)
    {
        startRotation = transform->getRotation();
        targetRotation = newRotation;
        durationTime = duration;

        rotationTime = 0.0f;
        isRotationAnimating = true;
    }

    void update(float deltaTime)
    {
        //Position
        if (isPositionAnimating)
        {
            positionTime += deltaTime;
            float t = positionTime / durationTime;
            if (t >= 1.0f)
            {
                t = 1.0f;
                isPositionAnimating = false;
            }

            XMFLOAT3 interpolatedPosition = lerp(startPosition, targetPosition, t);
            transform->setPosition(interpolatedPosition);
        }

        //Rotation
        if (isRotationAnimating)
        {
            rotationTime += deltaTime;
            float t = rotationTime / durationTime;
            if (t >= 1.0f)
            {
                t = 1.0f;
                isRotationAnimating = false;
            }

            XMFLOAT3 interpolatedRotation = lerp(startRotation, targetRotation, t);
            transform->setRotation(interpolatedRotation);
        }
    }

    void setTransform(TransformComponent* transformComponent) { transform = transformComponent; };

    void setDuration(float duration) { durationTime = duration; };
    float getDuration() { return durationTime; };

    void setTargetPosition(XMFLOAT3 target) { targetPosition = target; };
    XMFLOAT3 getTargetPosition() { return targetPosition; };

    void setTargetRotation(XMFLOAT3 target) { targetRotation = target;};
    XMFLOAT3 getTargetRotation() { return targetRotation; };

    void stopAnimations() { isPositionAnimating = false;  isRotationAnimating= false; }

private:
    TransformComponent* transform;

    float durationTime;

    //Position
    XMFLOAT3 startPosition;
    XMFLOAT3 targetPosition;
    float positionTime;
    bool isPositionAnimating;

    //Rotation
    XMFLOAT3 startRotation;
    XMFLOAT3 targetRotation;
    float rotationTime;
    bool isRotationAnimating;

    XMFLOAT3 lerp(const XMFLOAT3& startPos, const XMFLOAT3& targetPos, float time)
    {
        return XMFLOAT3(startPos.x + (targetPos.x - startPos.x) * time, startPos.y + (targetPos.y - startPos.y) * time, startPos.z + (targetPos.z - startPos.z) * time);
    }
};