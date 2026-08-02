#pragma once

#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include "Entity.h"

struct ComponentTypeDescriptor {
    std::type_index type{typeid(void)};
    std::string name;
    bool addable = true;
    std::function<bool(const Entity &)> has;
    std::function<void(Entity &)> addDefault;
    std::function<std::any(const Component &)> serialize;
    std::function<std::unique_ptr<Component>(const std::any &)> instantiate;
};

class ComponentTypeRegistry {
    std::vector<ComponentTypeDescriptor> descriptors;

    void addDescriptor(ComponentTypeDescriptor descriptor);

public:
    template<typename T>
        requires std::derived_from<T, Component> && std::default_initializable<T>
    void registerComponent(std::string name, bool addable = true) {
        static_assert(
            std::is_copy_constructible_v<T>,
            "Default component serialization requires a copy-constructible component"
        );

        addDescriptor({
            .type = std::type_index(typeid(T)),
            .name = std::move(name),
            .addable = addable,
            .has = [](const Entity &entity) {
                return entity.hasComponent<T>();
            },
            .addDefault = [](Entity &entity) {
                entity.addComponent<T>();
            },
            .serialize = [](const Component &component) -> std::any {
                return static_cast<const T &>(component);
            },
            .instantiate = [](const std::any &data) -> std::unique_ptr<Component> {
                return std::make_unique<T>(std::any_cast<const T &>(data));
            }
        });
    }

    template<typename T, typename Snapshot, typename Serialize, typename Instantiate>
        requires std::derived_from<T, Component> && std::default_initializable<T>
    void registerComponent(
        std::string name,
        Serialize &&serialize,
        Instantiate &&instantiate,
        bool addable = true
    ) {
        addDescriptor({
            .type = std::type_index(typeid(T)),
            .name = std::move(name),
            .addable = addable,
            .has = [](const Entity &entity) {
                return entity.hasComponent<T>();
            },
            .addDefault = [](Entity &entity) {
                entity.addComponent<T>();
            },
            .serialize = [serializer = std::forward<Serialize>(serialize)](
                const Component &component
            ) -> std::any {
                return std::any(serializer(static_cast<const T &>(component)));
            },
            .instantiate = [factory = std::forward<Instantiate>(instantiate)](
                const std::any &data
            ) -> std::unique_ptr<Component> {
                std::unique_ptr<T> component = factory(
                    std::any_cast<const Snapshot &>(data)
                );
                return component;
            }
        });
    }

    [[nodiscard]] const ComponentTypeDescriptor *find(std::type_index type) const;
    [[nodiscard]] const std::vector<ComponentTypeDescriptor> &getDescriptors() const {
        return descriptors;
    }
};

void registerBuiltinComponentTypes(ComponentTypeRegistry &registry);
