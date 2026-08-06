#pragma once

#include <pu/Plutonium>
#include <functional>
#include <string>

class MangaListLayout : public pu::ui::Layout {
    public:
        using OnMangaSelected = std::function<void(const std::string &manga_path)>;

        MangaListLayout(const std::string &manga_root);
        PU_SMART_CTOR(MangaListLayout)

        inline void SetOnMangaSelected(OnMangaSelected on_selected) {
            this->on_selected = on_selected;
        }

    private:
        std::string manga_root;
        OnMangaSelected on_selected;
        pu::ui::elm::TextBlock::Ref titleText;
        pu::ui::elm::Menu::Ref menu;
};
