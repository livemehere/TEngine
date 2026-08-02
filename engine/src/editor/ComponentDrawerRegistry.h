#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

#include "../scene/Entity.h"

class Scene;
class ComponentTypeRegistry;

class ComponentDrawerRegistry {
public:
    using Predicate = std::function<bool(Scene &, Entity &)>;
    using Drawer = std::function<void(Scene &, Entity &)>;

private:
    struct Entry {
        std::optional<std::type_index> componentType;
        std::string label;
        Predicate predicate;
        Drawer drawer;
    };

    std::vector<Entry> entries;
    std::string componentSearch;

public:
    template<typename T, typename Function>
        requires std::derived_from<T, Component>
    void registerDrawer(std::string label, Function &&function) {
        entries.push_back({
            .componentType = std::type_index(typeid(T)),
            .label = std::move(label),
            .predicate = [](Scene &, Entity &entity) {
                return entity.hasComponent<T>();
            },
            .drawer = [drawer = std::forward<Function>(function)](
                Scene &scene,
                Entity &entity
            ) mutable {
                drawer(scene, entity, entity.getComponent<T>());
            }
        });
    }

    template<typename T>
        requires std::derived_from<T, Component>
    void registerCustomDrawer(
        std::string label,
        Predicate predicate,
        Drawer drawer
    ) {
        entries.push_back({
            .componentType = std::type_index(typeid(T)),
            .label = std::move(label),
            .predicate = std::move(predicate),
            .drawer = std::move(drawer)
        });
    }

    void registerCustomDrawer(std::string label, Predicate predicate, Drawer drawer);
    void drawComponents(
        Scene &scene,
        Entity &entity,
        const ComponentTypeRegistry &componentTypes
    ) const;
    void drawAddComponent(Entity &entity, const ComponentTypeRegistry &componentTypes);
};

void registerDefaultComponentDrawers(ComponentDrawerRegistry &registry);
