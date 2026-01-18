#pragma once

class Component {
public:
    explicit Component(const class Entity* entity) : myEntity(entity) {}

    //virtual void Update() = 0;

    virtual ~Component() = default;
protected:
    const Entity* myEntity;

    friend Entity;
};