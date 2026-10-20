#include "FilterPopup.hpp"
#include <algorithm>
#include <cctype>

using namespace geode::prelude;

namespace {
    constexpr int kCols = 3;
    constexpr float kCellH = 34.f;

    std::string lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    // Scale9 button body with a width-limited label (child tag 1).
    CCScale9Sprite* makeBtn(std::string const& text, float w, float h, char const* tex) {
        auto bg = CCScale9Sprite::create(tex, {0.f, 0.f, 40.f, 40.f});
        bg->setContentSize({w, h});
        auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
        label->limitLabelWidth(w - 12.f, .5f, .1f);
        label->setPosition({w / 2.f, h / 2.f});
        label->setTag(1);
        bg->addChild(label);
        return bg;
    }
}

FilterPopup* FilterPopup::create() {
    auto ret = new FilterPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool FilterPopup::init() {
    if (!Popup::init(400.f, 270.f)) return false;
    this->setTitle("Smart Select");

    m_mode = static_cast<Mode>(Mod::get()->getSavedValue<int>("mode", 0));

    float W = m_size.width;
    float H = m_size.height;

    // list background
    float listW = W - 30.f;
    float listH = H - 118.f;
    auto listBg = CCScale9Sprite::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
    listBg->setColor({0, 0, 0});
    listBg->setOpacity(90);
    listBg->setContentSize({listW, listH});
    listBg->setAnchorPoint({0.f, 0.f});
    listBg->setPosition({15.f, 15.f});
    m_mainLayer->addChild(listBg);

    m_scroll = ScrollLayer::create({listW, listH});
    m_scroll->setPosition({15.f, 15.f});
    m_mainLayer->addChild(m_scroll);

    // tabs + grid menus
    m_tabMenu = CCMenu::create();
    m_tabMenu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(m_tabMenu, 5);

    // control row: search | N | mode
    float rowY = H - 82.f;

    m_search = TextInput::create(150.f, "Search filters...", "bigFont.fnt");
    m_search->setPosition({15.f + 75.f, rowY});
    m_search->setMaxCharCount(30);
    m_search->setCallback([this](std::string const& s) {
        m_query = s;
        this->rebuildTabs();
        this->rebuildGrid();
    });
    m_mainLayer->addChild(m_search);

    auto nLabel = CCLabelBMFont::create("N:", "goldFont.fnt");
    nLabel->setScale(.55f);
    nLabel->setPosition({188.f, rowY});
    m_mainLayer->addChild(nLabel);

    m_param = TextInput::create(60.f, "0", "bigFont.fnt");
    m_param->setCommonFilter(CommonFilter::Int);
    m_param->setMaxCharCount(6);
    m_param->setPosition({225.f, rowY});
    m_mainLayer->addChild(m_param);

    m_modeLabelWidth = 130.f;
    auto modeBg = makeBtn("Mode", m_modeLabelWidth, 26.f, "GJ_button_02.png");
    m_modeLabel = static_cast<CCLabelBMFont*>(modeBg->getChildByTag(1));
    auto modeBtn = CCMenuItemExt::createSpriteExtra(modeBg, [this](CCObject*) {
        m_mode = static_cast<Mode>((static_cast<int>(m_mode) + 1) % 4);
        Mod::get()->setSavedValue<int>("mode", static_cast<int>(m_mode));
        this->updateModeLabel();
    });
    modeBtn->setPosition({W - 15.f - m_modeLabelWidth / 2.f, rowY});
    m_tabMenu->addChild(modeBtn);
    this->updateModeLabel();

    this->rebuildTabs();
    this->rebuildGrid();

    handleTouchPriority(this);
    return true;
}

void FilterPopup::updateModeLabel() {
    char const* names[] = {"Replace", "Add", "Subtract", "Intersect"};
    m_modeLabel->setString(fmt::format("Mode: {}", names[static_cast<int>(m_mode)]).c_str());
    m_modeLabel->limitLabelWidth(m_modeLabelWidth - 12.f, .5f, .1f);
}

void FilterPopup::queueRebuild() {
    Loader::get()->queueInMainThread([self = Ref<FilterPopup>(this)] {
        self->rebuildTabs();
        self->rebuildGrid();
    });
}

void FilterPopup::rebuildTabs() {
    // keep the mode button (first child) and drop the old tab buttons
    auto children = CCArrayExt<CCNode*>(m_tabMenu->getChildren());
    std::vector<CCNode*> toRemove;
    for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) toRemove.push_back(children[i]);
    }
    for (auto n : toRemove) n->removeFromParent();

    auto const& names = ssf::categoryShortNames();
    float tabW = (m_size.width - 30.f) / static_cast<float>(names.size());

    for (size_t i = 0; i < names.size(); ++i) {
        bool active = m_query.empty() && i == m_category;
        auto bg = makeBtn(names[i], tabW - 4.f, 26.f, active ? "GJ_button_01.png" : "GJ_button_04.png");
        auto btn = CCMenuItemExt::createSpriteExtra(bg, [this, i](CCObject*) {
            m_category = i;
            m_query.clear();
            m_search->setString("", false);
            this->queueRebuild(); // rebuild next frame: this button gets destroyed
        });
        btn->setPosition({15.f + tabW * (static_cast<float>(i) + .5f), m_size.height - 50.f});
        m_tabMenu->addChild(btn);
    }
    handleTouchPriority(this);
}

void FilterPopup::rebuildGrid() {
    auto content = m_scroll->m_contentLayer;
    content->removeAllChildren();
    m_gridMenu = nullptr;

    auto const& cats = ssf::categories();
    auto q = lower(m_query);

    std::vector<ssf::FilterDef const*> shown;
    for (auto const& f : ssf::allFilters()) {
        if (q.empty()) {
            if (f.category == cats[m_category]) shown.push_back(&f);
        } else if (lower(f.name).find(q) != std::string::npos) {
            shown.push_back(&f);
        }
    }

    float viewW = m_scroll->getContentWidth();
    float viewH = m_scroll->getContentHeight();
    float cellW = viewW / kCols;
    size_t rows = (shown.size() + kCols - 1) / kCols;
    float contentH = std::max(viewH, static_cast<float>(rows) * kCellH + 6.f);

    content->setContentSize({viewW, contentH});

    m_gridMenu = CCMenu::create();
    m_gridMenu->setContentSize({viewW, contentH});
    m_gridMenu->setAnchorPoint({0.f, 0.f});
    m_gridMenu->setPosition({0.f, 0.f});
    content->addChild(m_gridMenu);

    if (shown.empty()) {
        auto none = CCLabelBMFont::create("No filters match", "bigFont.fnt");
        none->setScale(.5f);
        none->setOpacity(150);
        none->setPosition({viewW / 2.f, contentH / 2.f});
        content->addChild(none);
    }

    for (size_t i = 0; i < shown.size(); ++i) {
        auto const* f = shown[i];
        size_t col = i % kCols;
        size_t row = i / kCols;

        std::string label = f->name;
        auto bg = makeBtn(label, cellW - 8.f, kCellH - 6.f,
            f->needsParam ? "GJ_button_05.png" : "GJ_button_04.png");
        auto btn = CCMenuItemExt::createSpriteExtra(bg, [this, f](CCObject*) {
            this->run(*f);
        });
        btn->setPosition({
            cellW * (static_cast<float>(col) + .5f),
            contentH - kCellH * (static_cast<float>(row) + .5f) - 3.f
        });
        m_gridMenu->addChild(btn);
    }

    m_scroll->moveToTop();
    handleTouchPriority(this);
}

void FilterPopup::run(ssf::FilterDef const& f) {
    auto ui = EditorUI::get();
    if (!ui || !ui->m_editorLayer) return;

    ssf::FilterContext ctx;

    if (f.needsParam) {
        auto str = m_param->getString();
        if (str.empty()) {
            Notification::create("Type a number in the N box first", NotificationIcon::Warning)->show();
            return;
        }
        ctx.param = utils::numFromString<int>(str).unwrapOr(0);
    }

    // snapshot current selection
    std::vector<GameObject*> selected;
    if (auto arr = ui->getSelectedObjects()) {
        for (auto o : CCArrayExt<GameObject*>(arr)) {
            selected.push_back(o);
            ctx.selected.insert(o);
            ctx.selectedIDs.insert(o->m_objectID);
            ctx.selectedTypes.insert(static_cast<int>(o->m_objectType));
        }
    }

    // run the filter over every object in the level
    std::vector<GameObject*> hits;
    std::unordered_set<GameObject*> hitSet;
    for (auto o : CCArrayExt<GameObject*>(ui->m_editorLayer->m_objects)) {
        if (f.test(o, ctx)) {
            hits.push_back(o);
            hitSet.insert(o);
        }
    }

    Mode mode = f.forceReplace ? Mode::Replace : m_mode;
    std::vector<GameObject*> result;

    switch (mode) {
        case Mode::Replace:
            result = hits;
            break;
        case Mode::Add:
            result = selected;
            for (auto o : hits) {
                if (!ctx.selected.count(o)) result.push_back(o);
            }
            break;
        case Mode::Subtract:
            for (auto o : selected) {
                if (!hitSet.count(o)) result.push_back(o);
            }
            break;
        case Mode::Intersect:
            for (auto o : selected) {
                if (hitSet.count(o)) result.push_back(o);
            }
            break;
    }

    if (hits.empty() && (mode == Mode::Replace || mode == Mode::Add)) {
        Notification::create("Nothing matched this filter", NotificationIcon::Warning)->show();
        return;
    }

    auto arr = CCArray::create();
    for (auto o : result) arr->addObject(o);

    ui->deselectAll();
    ui->selectObjects(arr, true);

    Notification::create(
        fmt::format("Selected {} object{}", result.size(), result.size() == 1 ? "" : "s"),
        NotificationIcon::Success, 1.f
    )->show();

    if (Mod::get()->getSettingValue<bool>("close-after-select")) {
        this->onClose(nullptr);
    }
}
