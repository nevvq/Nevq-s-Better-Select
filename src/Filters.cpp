#include "Filters.hpp"
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace ssf {
namespace {

    using Ctx = FilterContext;

    // ---- accessors: if a GD binding name differs in your SDK, fix it here only ----

    EffectGameObject* asTrigger(GameObject* o) {
        if (!o || !o->m_isTrigger) return nullptr;
        return typeinfo_cast<EffectGameObject*>(o);
    }

    bool inGroup(GameObject* o, int g) {
        if (!o->m_groups) return false;
        int n = std::min<int>(o->m_groupCount, 10);
        for (int i = 0; i < n; ++i) {
            if ((*o->m_groups)[i] == g) return true;
        }
        return false;
    }

    bool hasAnyGroup(GameObject* o) {
        return o->m_groups && o->m_groupCount > 0 && (*o->m_groups)[0] != 0;
    }

    // ---- predicate builders ----

    Predicate typeIs(std::vector<GameObjectType> types) {
        return [types](GameObject* o, Ctx const&) {
            return std::find(types.begin(), types.end(), o->m_objectType) != types.end();
        };
    }

    Predicate idIs(std::vector<int> ids) {
        return [ids](GameObject* o, Ctx const&) {
            return std::find(ids.begin(), ids.end(), o->m_objectID) != ids.end();
        };
    }

    // trigger whose object ID is in `ids` AND passes `extra`
    Predicate triggerIn(std::vector<int> ids, std::function<bool(EffectGameObject*)> extra) {
        return [ids, extra](GameObject* o, Ctx const&) {
            if (std::find(ids.begin(), ids.end(), o->m_objectID) == ids.end()) return false;
            auto t = asTrigger(o);
            return t && extra(t);
        };
    }

    Predicate triggerWhere(std::function<bool(EffectGameObject*)> f) {
        return [f](GameObject* o, Ctx const&) {
            auto t = asTrigger(o);
            return t && f(t);
        };
    }

    Predicate flag(std::function<bool(GameObject*)> f) {
        return [f](GameObject* o, Ctx const&) { return f(o); };
    }

    // ---- trigger ID groups (2.2 object IDs) ----

    std::vector<int> const kCamera   {1913, 1914, 1916, 2015, 2062};
    std::vector<int> const kItem     {1611, 1811, 1817, 3619, 3620, 3641};
    std::vector<int> const kTime     {3614, 3615, 3617};
    std::vector<int> const kLogic    {1268, 1616, 1912, 2068, 3607, 3618, 1595, 1812, 1815};
    std::vector<int> const kTargets  {901, 1346, 2067, 1007, 1049, 1268, 1616, 1347, 1585};
    std::vector<int> const kHasDur   {901, 1346, 2067, 1007, 899, 1347, 1520};

    std::vector<FilterDef> build() {
        std::vector<FilterDef> v;

        auto add = [&](char const* cat, char const* name, Predicate p,
                       bool needsParam = false, bool forceReplace = false) {
            FilterDef d;
            d.category = cat;
            d.name = name;
            d.needsParam = needsParam;
            d.forceReplace = forceReplace;
            d.test = std::move(p);
            v.push_back(std::move(d));
        };

        using T = GameObjectType;

        // ===================== Types =====================
        char const* C = "Types";
        add(C, "Solid blocks",        typeIs({T::Solid}));
        add(C, "Hazards",             typeIs({T::Hazard}));
        add(C, "Slopes",              typeIs({T::Slope}));
        add(C, "Decorations",         typeIs({T::Decoration}));
        add(C, "Collision blocks",    typeIs({T::CollisionObject}));
        add(C, "Breakable blocks",    typeIs({T::Breakable}));
        add(C, "All portals", typeIs({
            T::InverseGravityPortal, T::NormalGravityPortal, T::ShipPortal, T::CubePortal,
            T::InverseMirrorPortal, T::NormalMirrorPortal, T::BallPortal, T::RegularSizePortal,
            T::MiniSizePortal, T::UfoPortal, T::DualPortal, T::SoloPortal, T::WavePortal,
            T::RobotPortal, T::TeleportPortal, T::SpiderPortal, T::SwingPortal }));
        add(C, "Gamemode portals", typeIs({
            T::ShipPortal, T::CubePortal, T::BallPortal, T::UfoPortal, T::WavePortal,
            T::RobotPortal, T::SpiderPortal, T::SwingPortal }));
        add(C, "Gravity portals",     typeIs({T::InverseGravityPortal, T::NormalGravityPortal}));
        add(C, "Size portals",        typeIs({T::RegularSizePortal, T::MiniSizePortal}));
        add(C, "Mirror portals",      typeIs({T::InverseMirrorPortal, T::NormalMirrorPortal}));
        add(C, "Dual / solo portals", typeIs({T::DualPortal, T::SoloPortal}));
        add(C, "Teleport portals",    typeIs({T::TeleportPortal}));
        add(C, "All pads",            typeIs({T::YellowJumpPad, T::PinkJumpPad, T::GravityPad, T::RedJumpPad}));
        add(C, "All orbs / rings", typeIs({
            T::YellowJumpRing, T::PinkJumpRing, T::GravityRing, T::GreenRing, T::DropRing,
            T::RedJumpRing, T::CustomRing, T::DashRing, T::GravityDashRing }));
        add(C, "Dash orbs",           typeIs({T::DashRing, T::GravityDashRing}));
        add(C, "Coins",               typeIs({T::SecretCoin, T::UserCoin}));
        add(C, "Collectibles",        typeIs({T::Collectible}));
        add(C, "Modifiers",           typeIs({T::Modifier}));
        add(C, "Special objects",     typeIs({T::Special}));

        // ===================== Block flags =====================
        C = "Block flags";
        add(C, "Don't touch",         flag([](GameObject* o) { return o->m_isNoTouch; }));
        add(C, "Don't enter",         flag([](GameObject* o) { return o->m_isDontEnter; }));
        add(C, "Don't fade",          flag([](GameObject* o) { return o->m_isDontFade; }));
        add(C, "High detail",         flag([](GameObject* o) { return o->m_highDetail; }));
        add(C, "No glow",             flag([](GameObject* o) { return o->m_hasNoGlow; }));
        add(C, "Group parent",        flag([](GameObject* o) { return o->m_isGroupParent; }));
        add(C, "Passable",            flag([](GameObject* o) { return o->m_isPassable; }));
        add(C, "Solids: don't touch", flag([](GameObject* o) {
            return o->m_objectType == T::Solid && o->m_isNoTouch; }));
        add(C, "Solids: don't enter", flag([](GameObject* o) {
            return o->m_objectType == T::Solid && o->m_isDontEnter; }));
        add(C, "Hazards: don't touch", flag([](GameObject* o) {
            return o->m_objectType == T::Hazard && o->m_isNoTouch; }));
        add(C, "Flipped X",           flag([](GameObject* o) { return o->m_isFlipX; }));
        add(C, "Flipped Y",           flag([](GameObject* o) { return o->m_isFlipY; }));
        add(C, "Rotated",             flag([](GameObject* o) {
            return std::fabs(std::fmod(o->getRotation(), 360.f)) > 0.01f; }));
        add(C, "Scaled",              flag([](GameObject* o) {
            return std::fabs(o->getScaleX() - 1.f) > 0.001f || std::fabs(o->getScaleY() - 1.f) > 0.001f; }));

        // ===================== Triggers (by type) =====================
        C = "Triggers";
        add(C, "All triggers",        triggerWhere([](EffectGameObject*) { return true; }));
        add(C, "Move",                idIs({901}));
        add(C, "Rotate",              idIs({1346}));
        add(C, "Scale",               idIs({2067}));
        add(C, "Alpha",               idIs({1007}));
        add(C, "Pulse",               idIs({1006}));
        add(C, "Color",               idIs({899}));
        add(C, "Toggle",              idIs({1049}));
        add(C, "Spawn",               idIs({1268}));
        add(C, "Stop",                idIs({1616}));
        add(C, "Follow",              idIs({1347}));
        add(C, "Animate",             idIs({1585}));
        add(C, "Shake",               idIs({1520}));
        add(C, "Touch",               idIs({1595}));
        add(C, "Count / Instant count", idIs({1611, 1811}));
        add(C, "On death",            idIs({1812}));
        add(C, "Collision",           idIs({1815}));
        add(C, "Pickup",              idIs({1817}));
        add(C, "Random / Adv. random", idIs({1912, 2068}));
        add(C, "Song / SFX",          idIs({1934, 3602}));
        add(C, "Gravity",             idIs({2066}));
        add(C, "Options",             idIs({2899}));
        add(C, "Camera (all)",        idIs(kCamera));
        add(C, "Item / counter (all)", idIs(kItem));
        add(C, "Time (all)",          idIs(kTime));
        add(C, "Logic / spawn-ish",   idIs(kLogic));

        // ===================== Trigger settings =====================
        C = "Trigger opts";
        add(C, "Touch triggered",     triggerWhere([](EffectGameObject* t) { return t->m_isTouchTriggered; }));
        add(C, "Spawn triggered",     triggerWhere([](EffectGameObject* t) { return t->m_isSpawnTriggered; }));
        add(C, "Multi trigger",       triggerWhere([](EffectGameObject* t) { return t->m_isMultiTriggered; }));
        add(C, "Toggle: activate",    triggerIn({1049}, [](EffectGameObject* t) { return t->m_activateGroup; }));
        add(C, "Toggle: deactivate",  triggerIn({1049}, [](EffectGameObject* t) { return !t->m_activateGroup; }));
        add(C, "Target group is 0",   triggerIn(kTargets, [](EffectGameObject* t) { return t->m_targetGroupID == 0; }));
        add(C, "Instant (duration 0)", triggerIn(kHasDur, [](EffectGameObject* t) { return t->m_duration <= 0.f; }));
        add(C, "Has duration > 0",    triggerIn(kHasDur, [](EffectGameObject* t) { return t->m_duration > 0.f; }));
        add(C, "Targets group [N]",
            [](GameObject* o, Ctx const& c) {
                auto t = asTrigger(o);
                return t && t->m_targetGroupID == c.param;
            }, /*needsParam*/ true);

        // ===================== Groups & layers =====================
        C = "Groups";
        add(C, "Has any group",       flag([](GameObject* o) { return hasAnyGroup(o); }));
        add(C, "No group",            flag([](GameObject* o) { return !hasAnyGroup(o); }));
        add(C, "In group [N]",
            [](GameObject* o, Ctx const& c) { return inGroup(o, c.param); }, true);
        add(C, "Editor layer [N]",
            [](GameObject* o, Ctx const& c) {
                return o->m_editorLayer == c.param || o->m_editorLayer2 == c.param;
            }, true);
        add(C, "Object ID [N]",
            [](GameObject* o, Ctx const& c) { return o->m_objectID == c.param; }, true);

        // ===================== Selection tools =====================
        C = "Selection";
        add(C, "Invert selection",
            [](GameObject* o, Ctx const& c) { return !c.selected.count(o); },
            false, /*forceReplace*/ true);
        add(C, "Same ID as selection",
            [](GameObject* o, Ctx const& c) { return c.selectedIDs.count(o->m_objectID) > 0; });
        add(C, "Same type as selection",
            [](GameObject* o, Ctx const& c) { return c.selectedTypes.count(static_cast<int>(o->m_objectType)) > 0; });

        return v;
    }
}

std::vector<std::string> const& categories() {
    static std::vector<std::string> const v {
        "Types", "Block flags", "Triggers", "Trigger opts", "Groups", "Selection"
    };
    return v;
}

std::vector<std::string> const& categoryShortNames() {
    static std::vector<std::string> const v {
        "Types", "Flags", "Triggers", "Trig opts", "Groups", "Select"
    };
    return v;
}

std::vector<FilterDef> const& allFilters() {
    static std::vector<FilterDef> const v = build();
    return v;
}

}
