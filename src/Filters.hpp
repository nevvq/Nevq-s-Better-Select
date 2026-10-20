#pragma once

#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ssf {

    // Data a filter may need besides the object itself.
    struct FilterContext {
        std::unordered_set<GameObject*> selected;   // current selection
        std::unordered_set<int> selectedIDs;        // object IDs in current selection
        std::unordered_set<int> selectedTypes;      // GameObjectType values in current selection
        int param = 0;                              // value of the "N" box
    };

    using Predicate = std::function<bool(GameObject*, FilterContext const&)>;

    struct FilterDef {
        std::string category;
        std::string name;
        bool needsParam = false;    // requires a number in the "N" box
        bool forceReplace = false;  // ignores the Replace/Add/Subtract/Intersect mode
        Predicate test;
    };

    std::vector<std::string> const& categories();
    std::vector<std::string> const& categoryShortNames();
    std::vector<FilterDef> const& allFilters();
}
