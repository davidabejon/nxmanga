#pragma once

#include <pu/Plutonium>
#include <RoundedRectangle.hpp>
#include <RoundedOutlineRectangle.hpp>
#include <functional>
#include <string>
#include <vector>

// The X-button side panel: a titled list of items sliding in over whatever
// Layout owns it. The owner builds one, adds its own items, and forwards
// input to HandleInput from the very top of its own SetOnInput, so the panel
// can claim X/B before the rest of the Layout's input handling runs.
//
// Also owns a built-in language-picker item, since switching language is a
// menu-shell concern shared by every screen rather than something specific
// to any one of them.
class SideMenu {
    public:
        using ItemCallback = std::function<void()>;
        // Called (again) whenever the menu needs to re-resolve an item's
        // text, e.g. after the active language changes, so items showing
        // translated or state-dependent text stay correct without the
        // caller having to remember to update them by hand.
        using LabelProvider = std::function<std::string()>;

        SideMenu(pu::ui::Layout *owner);
        PU_SMART_CTOR(SideMenu)

        // Adds an item, firing on_selected on both the A button and touch
        // taps. Returns the created MenuItem, mostly useful for tests/debug
        // since label refreshes are handled internally via RefreshLabels.
        pu::ui::elm::MenuItem::Ref AddItem(LabelProvider get_label, ItemCallback on_selected);

        // Removes every item added via AddItem (but keeps whatever built-in
        // items the constructor itself added, like the language picker), so
        // the owner can rebuild its own items from scratch to reflect state
        // that could have changed while the panel was closed. Call before
        // opening the panel, not while it's already open.
        void ClearItems();

        // Re-resolves the title, footer, language item, and every item
        // added via AddItem by calling their LabelProvider again. Call
        // after anything that could change one of those labels (a language
        // switch, or a caller's own item toggling some setting).
        void RefreshLabels();

        inline bool IsOpen() const {
            return this->open;
        }

        void SetOpen(const bool open);

        // Call first, from the owner's SetOnInput. Opens/closes the panel on
        // X/B and returns whether it consumed this frame's input, so the
        // caller knows to skip its own handling this frame.
        bool HandleInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos);

    private:
        static constexpr s32 PanelWidth = 460;
        static constexpr s32 PanelMargin = 24;
        static constexpr s32 PanelBorderRadius = 24;
        static constexpr s32 PanelInset = 32;
        static constexpr s32 ItemsInset = 20;
        static constexpr s32 ItemHeight = 84;
        static constexpr s32 OutlineMarginY = 6;
        static constexpr s32 OutlineRadius = 16;
        static constexpr s32 OutlineThickness = 3;
        static constexpr pu::ui::Color PanelColor = pu::ui::Color(24, 24, 28, 235);
        static constexpr pu::ui::Color AccentColor = pu::ui::Color(10, 189, 227, 0xFF);
        static constexpr pu::ui::Color ItemTextColor = pu::ui::Color(255, 255, 255, 0xFF);
        static constexpr pu::ui::Color FooterTextColor = pu::ui::Color(190, 190, 195, 0xFF);
        static constexpr pu::ui::Color OutlineIdleColor = pu::ui::Color(200, 200, 210, 90);
        static constexpr pu::ui::Color OutlineFocusColor = pu::ui::Color(120, 200, 255, 0xFF);

        void UpdateItemOutlines();
        std::string GetLanguageItemLabel() const;
        void CycleLanguage();

        pu::ui::Layout *owner;
        bool open;
        s32 menu_item_x;
        s32 menu_y;
        s32 menu_w;
        RoundedRectangle::Ref bg;
        pu::ui::elm::TextBlock::Ref titleText;
        pu::ui::elm::Rectangle::Ref divider;
        pu::ui::elm::Menu::Ref menu;
        pu::ui::elm::TextBlock::Ref footer;
        // Position-keyed pool: since a Layout can never have an element
        // removed from it once added, ClearItems() can't delete outlines
        // that no longer have a matching item, only hide them; AddItem
        // reuses whatever's already in the pool at a given index (same
        // position either way) before creating a new one.
        std::vector<RoundedOutlineRectangle::Ref> itemOutlines;
        std::vector<LabelProvider> itemLabelProviders;
        // How many leading items were added by the constructor itself
        // (e.g. the language picker) and so must survive every ClearItems().
        size_t builtinItemCount;
};
