#pragma once

#include <pu/Plutonium>
#include <MangaGrid.hpp>
#include <LoadingSpinner.hpp>
#include <RoundedRectangle.hpp>
#include <SideMenu.hpp>
#include <Settings.hpp>
#include <functional>
#include <string>
#include <vector>

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
        // Loads one entry's cover thumbnail per frame (called via a render
        // callback) so the spinner keeps animating while the grid fills in,
        // instead of blocking on every cover up front.
        void LoadNextPendingCover();

        // Recomputes and re-applies the grid badge for pending_paths[index],
        // e.g. right after marking it as read/unread.
        void RefreshGridItemStatus(const size_t index, const std::string &path);

        // Rebuilds every side menu item from scratch: the mark as
        // read/unread option(s) for whatever's currently selected in the
        // grid (which can have changed since the panel was last open), plus
        // the regular settings items. Call right before opening the panel.
        void RebuildSideMenu();

        std::string GetOrientationLabel() const;
        std::string GetCascadeModeLabel() const;

        std::string manga_root;
        OnMangaSelected on_selected;
        OnBack on_back;
        pu::ui::elm::TextBlock::Ref titleText;
        MangaGrid::Ref grid;
        LoadingSpinner::Ref spinner;
        pu::ui::elm::TextBlock::Ref loadingText;
        RoundedRectangle::Ref settingsHintBg;
        pu::ui::elm::TextBlock::Ref settingsHint;
        std::vector<std::string> pending_paths;
        std::vector<std::string> pending_names;
        size_t pending_index;
        SideMenu::Ref sideMenu;
};
