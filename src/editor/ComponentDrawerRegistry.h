#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "../scene/Entity.h"

class Scene;

class ComponentDrawerRegistry {
public:
    using Predicate = std::function<bool(Scene &, Entity &)>;
    using Drawer = std::function<void(Scene &, Entity &)>;
    using ComponentFactory = std::function<void(Entity &)>;

private:
    struct Entry {
        std::string label;
        Predicate predicate;
        Drawer drawer;
    };

    struct FactoryEntry {
        std::string label;
        std::function<bool(const Entity &)> canAdd;
        ComponentFactory add;
    };

    std::vector<Entry> entries;
    std::vector<FactoryEntry> factories;
    std::string componentSearch;

public:
    template<typename T, typename Function>
        requires std::derived_from<T, Component>
    void registerDrawer(std::string label, Function &&function) {
        registerCustomDrawer(
            std::move(label),
            [](Scene &, Entity &entity) {
                return entity.hasComponent<T>();
            },
            [drawer = std::forward<Function>(function)](Scene &scene, Entity &entity) mutable {
                drawer(scene, entity, entity.getComponent<T>());
            }
        );
    }

    template<typename T>
        requires std::derived_from<T, Component> && std::default_initializable<T>
    void registerComponent(std::string label) {
        factories.push_back({
            .label = std::move(label),
            .canAdd = [](const Entity &entity) {
                return !entity.hasComponent<T>();
            },
            .add = [](Entity &entity) {
                entity.addComponent<T>();
            }
        });
    }

    void registerCustomDrawer(std::string label, Predicate predicate, Drawer drawer);
    void drawComponents(Scene &scene, Entity &entity) const;
    void drawAddComponent(Entity &entity);
};

void registerDefaultComponentDrawers(ComponentDrawerRegistry &registry);
