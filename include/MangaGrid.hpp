#pragma once

#include <pu/Plutonium>
#include <functional>
#include <string>
#include <vector>

// A focusable grid of cover cards (thumbnail + title), navigated with the
// d-pad/stick like pu::ui::elm::Menu, but arranged in rows/columns instead of
// a single column. Scrolls by whole rows so a row is either fully visible or
// fully hidden, never partially clipped.
class MangaGrid : public pu::ui::elm::Element {
    public:
        using OnItemSelected = std::function<void(const size_t index)>;

        MangaGrid(const s32 x, const s32 y, const s32 width, const s32 height, const s32 columns);
        PU_SMART_CTOR(MangaGrid)

        inline s32 GetX() override {
            return this->x;
        }

        inline s32 GetY() override {
            return this->y;
        }

        inline s32 GetWidth() override {
            return this->w;
        }

        inline s32 GetHeight() override {
            return this->h;
        }

        inline bool IsEmpty() {
            return this->cards.empty();
        }

        void AddItem(const std::string &title, pu::sdl2::TextureHandle::Ref thumbnail);
        void ClearItems();

        inline void SetOnItemSelected(OnItemSelected on_item_selected) {
            this->on_item_selected = on_item_selected;
        }

        void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
        void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;

    private:
        struct Card {
            pu::sdl2::TextureHandle::Ref thumbnail;
            // Pre-truncated ("...") title, shown whenever this card isn't the
            // marquee-animated focused one.
            pu::sdl2::TextureHandle::Ref clamped_title_tex;
            // Full, untruncated title, only used to scroll through while
            // this card is focused and doesn't fit.
            pu::sdl2::TextureHandle::Ref full_title_tex;
            s32 marquee_x;
            s32 marquee_delay;
        };

        static constexpr s32 CardSpacing = 24;
        static constexpr s32 TitleAreaHeight = 44;
        static constexpr s32 TitleHorizontalPadding = 16;
        static constexpr s32 TitleMarqueeSpeed = 3;
        static constexpr s32 TitleMarqueeDelaySteps = 20;
        static constexpr s32 ThumbnailAspectNumerator = 2;
        static constexpr s32 ThumbnailAspectDenominator = 3;
        static constexpr s32 CardRadius = 18;
        static constexpr s32 CardFrameMargin = 10;
        static constexpr s32 FocusOutlineThickness = 4;
        static constexpr pu::ui::Color CardColor = pu::ui::Color(230, 230, 230, 0xFF);
        static constexpr pu::ui::Color FocusOutlineColor = pu::ui::Color(30, 100, 200, 0xFF);
        static constexpr pu::ui::Color TitleColor = pu::ui::Color(20, 20, 20, 0xFF);

        static constexpr s32 PageIndicatorPadding = 10;
        static constexpr s32 PageIndicatorBorderRadius = 14;
        static constexpr pu::ui::Color PageIndicatorBackgroundColor = pu::ui::Color(0, 0, 0, 160);
        static constexpr pu::ui::Color PageIndicatorTextColor = pu::ui::Color(255, 255, 255, 0xFF);

        s32 GetCardWidth();
        s32 GetCardHeight();
        s32 GetRowHeight();
        s32 GetRowsToShow();
        s32 GetTitleAreaWidth();
        void MoveSelection(const s32 delta_index);
        void EnsureSelectedRowVisible();
        void ResetCardMarquee(const size_t index);
        void UpdatePageIndicator();
        void RenderPageIndicator(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y);
        static void RenderThumbnailCover(pu::sdl2::TextureHandle::Ref thumbnail, const s32 x, const s32 y, const s32 w, const s32 h);

        s32 x;
        s32 y;
        s32 w;
        s32 h;
        s32 columns;
        size_t selected_index;
        s32 first_visible_row;
        std::vector<Card> cards;
        OnItemSelected on_item_selected;
        pu::sdl2::TextureHandle::Ref page_indicator_tex;
};
