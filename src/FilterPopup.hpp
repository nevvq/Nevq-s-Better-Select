#pragma once

#include <Geode/Geode.hpp>
#include "Filters.hpp"

class FilterPopup : public geode::Popup {
protected:
    enum class Mode { Replace, Add, Subtract, Intersect };

    Mode m_mode = Mode::Replace;
    size_t m_category = 0;
    std::string m_query;

    geode::ScrollLayer* m_scroll = nullptr;
    geode::TextInput* m_search = nullptr;
    geode::TextInput* m_param = nullptr;
    cocos2d::CCMenu* m_tabMenu = nullptr;
    cocos2d::CCMenu* m_gridMenu = nullptr;
    cocos2d::CCLabelBMFont* m_modeLabel = nullptr;
    float m_modeLabelWidth = 0.f;

    bool init();
    void rebuildTabs();
    void rebuildGrid();
    void queueRebuild();
    void updateModeLabel();
    void run(ssf::FilterDef const& filter);

public:
    static FilterPopup* create();
};
