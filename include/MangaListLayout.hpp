#pragma once

#include <pu/Plutonium>
#include <MangaGrid.hpp>
#include <functional>
#include <string>

class MangaListLayout : public pu::ui::Layout {
    public:
        using OnMangaSelected = std::function<void(const std::string &manga_path)>;
        using OnBack = std::function<void()>;

        static constexpr s32 GridColumns = 4;

        MangaListLayout(const std::string &manga_root);
        PU_SMART_CTOR(MangaListLayout)

        inline void SetOnMangaSelected(OnMangaSelected on_selected) {
            this->on_selected = on_selected;
        }

        inline void SetOnBack(OnBack on_back) {
            this->on_back = on_back;
        }

    private:
        std::string manga_root;
        OnMangaSelected on_selected;
        OnBack on_back;
        pu::ui::elm::TextBlock::Ref titleText;
        MangaGrid::Ref grid;
};
