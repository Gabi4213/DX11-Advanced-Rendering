#pragma once
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <string>
#include "Component.h"

using namespace std;

class Entity
{
public:

    template <typename T, typename... Args>
    T* addComponent(Args&&... args) 
    {
        static_assert(is_base_of<Component, T>::value, "make it a component type");
        auto comp = make_unique<T>(forward<Args>(args)...);
        T* compPtr = comp.get();
        components.push_back(move(comp));
        return compPtr;
    }

    template <typename T>
    T* getComponent() 
    {
        for (const auto& comp : components)
        {
            T* casted = dynamic_cast<T*>(comp.get());
            if (casted) return casted;
        }
        return nullptr;
    }


    string getEntityName() { return entityName; };
    void setEntityName(string name) { entityName = name; };

private:

    vector<unique_ptr<Component>> components;
    string entityName;
};