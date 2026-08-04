#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "../Transform.h"

using InstanceId = std::uint64_t;

struct InstanceData {
    InstanceId id = 0;
    Transform transform;
};

class InstanceCollection {
    std::vector<InstanceData> items;
    InstanceId nextId = 1;

public:
    [[nodiscard]] InstanceId add(const Transform &transform = {}) {
        const InstanceId id = nextId++;
        items.push_back({.id = id, .transform = transform});
        return id;
    }

    [[nodiscard]] bool remove(InstanceId id) {
        const auto item = std::ranges::find(items, id, &InstanceData::id);
        if (item == items.end()) {
            return false;
        }
        items.erase(item);
        return true;
    }

    void resize(std::size_t count) {
        while (items.size() < count) {
            (void)add();
        }
        if (items.size() > count) {
            items.resize(count);
        }
    }

    void clear() { items.clear(); }

    [[nodiscard]] InstanceData *find(InstanceId id) {
        const auto item = std::ranges::find(items, id, &InstanceData::id);
        return item == items.end() ? nullptr : &*item;
    }

    [[nodiscard]] const InstanceData *find(InstanceId id) const {
        const auto item = std::ranges::find(items, id, &InstanceData::id);
        return item == items.end() ? nullptr : &*item;
    }

    [[nodiscard]] std::vector<InstanceData> &getItems() { return items; }
    [[nodiscard]] const std::vector<InstanceData> &getItems() const { return items; }
    [[nodiscard]] std::size_t size() const { return items.size(); }
    [[nodiscard]] bool empty() const { return items.empty(); }
};
