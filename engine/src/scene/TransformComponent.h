#pragma once

#include "Component.h"
#include "../rendering/Transform.h"

class TransformComponent final : public Component {
public:
    Transform local;
};
