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
class SideMenu {
    public:
        using ItemCallback = std::function<void()>;

        SideMenu(pu::ui::Layout *owner, const std::string &title);
        PU_SMART_CTOR(SideMenu)

        // Adds an item, firing on_selected on both the A button and touch
        // taps. Returns the created MenuItem so callers needing to change
        // its label later (e.g. a toggle showing the current setting) can.
        pu::ui::elm::MenuItem::Ref AddItem(const std::string &name, ItemCallback on_selected);

        // Renames an item previously returned by AddItem and refreshes its
        // rendered text.
        void SetItemName(pu::ui::elm::MenuItem::Ref &item, const std::string &name);

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
        std::vector<RoundedOutlineRectangle::Ref> itemOutlines;
};
