#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "FilterPopup.hpp"

using namespace geode::prelude;

class $modify(SSFEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;
        this->addSmartSelectButton();
        return true;
    }

    void addSmartSelectButton() {
        // If this line fails to compile on your SDK, look the delete tab menu up by node ID instead:
        //   auto menu = this->getChildByIDRecursive("delete-category-menu");
        auto menu = m_deleteMenu;
        if (!menu) {
            log::warn("Smart Select: delete tab menu not found, button not added");
            return;
        }

        auto spr = ButtonSprite::create("Smart Select", 90, true, "bigFont.fnt", "GJ_button_01.png", 30.f, .6f);
        spr->setScale(.7f);

        auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCObject*) {
            if (auto popup = FilterPopup::create()) popup->show();
        });
        btn->setID("smart-select-button"_spr);

        // place the button to the right of the rightmost existing delete-tab item
        float right = 0.f;
        float y = 0.f;
        bool any = false;
        if (auto children = menu->getChildren()) {
            for (auto child : CCArrayExt<CCNode*>(children)) {
                auto box = child->boundingBox();
                if (!any || box.getMaxX() > right) {
                    right = box.getMaxX();
                    y = box.getMidY();
                    any = true;
                }
            }
        }

        float offX = static_cast<float>(Mod::get()->getSettingValue<int64_t>("button-offset-x"));
        float offY = static_cast<float>(Mod::get()->getSettingValue<int64_t>("button-offset-y"));
        float x = any ? right + btn->getContentWidth() * .5f + 8.f : 0.f;

        btn->setPosition({x + offX, y + offY});
        menu->addChild(btn);
    }
};
