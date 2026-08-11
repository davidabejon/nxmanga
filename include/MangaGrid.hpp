#pragma once

#include <pu/Plutonium>
#include <functional>
#include <string>
#include <vector>

// A focusable grid of cover cards (thumbnail + title), navigated with the
// d-pad/stick like pu::ui::elm::Menu, but arranged in rows/columns instead of
// a single column. Scrolls continuously by pixels (clipped to its bounds via
// SDL_RenderSetClipRect), like a normal scrollable list.
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

        inline size_t GetSelectedIndex() {
            return this->selected_index;
        }

        // Lets an owning Layout keep the grid on screen (e.g. dimmed behind
        // an overlay) while ignoring input, instead of hiding it outright
        // via SetVisible, which would also stop it from rendering.
        inline void SetInputEnabled(const bool enabled) {
            this->input_enabled = enabled;
        }

        // in_progress marks a manga/chapter that's been started but not
        // finished, showing a "current_page/page_count" badge (current_page
        // is 0-based, page_count the total); ignored when completed is true.
        void AddItem(const std::string &title, pu::sdl2::TextureHandle::Ref thumbnail, const bool completed, const bool in_progress, const u32 current_page, const size_t page_count);
        void ClearItems();

        // Updates an already-added item's completed/in-progress badge in
        // place (e.g. after the owner marks it as read/unread), without
        // touching its thumbnail or title.
        void UpdateItemStatus(const size_t index, const bool completed, const bool in_progress, const u32 current_page, const size_t page_count);

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
            // True if this manga/chapter (or, for a series folder, every
            // entry under it) has been read all the way to its last page.
            bool completed;
            // Pre-rendered "current/total" label for a manga/chapter that's
            // been started but not finished, or nullptr if there's none to
            // show (either completed, or never opened).
            pu::sdl2::TextureHandle::Ref progress_tex;
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
        // Inner margin kept between the grid's own bounds and the cards, so
        // the focus outline (which is drawn a few pixels outside the card)
        // always has room and never gets cut by the grid's clip rect.
        static constexpr s32 GridPadding = 8;
        static constexpr pu::ui::Color CardColor = pu::ui::Color(230, 230, 230, 0xFF);
        static constexpr pu::ui::Color FocusOutlineColor = pu::ui::Color(30, 100, 200, 0xFF);
        static constexpr pu::ui::Color TitleColor = pu::ui::Color(20, 20, 20, 0xFF);
        static constexpr pu::ui::Color ThumbnailPlaceholderColor = pu::ui::Color(200, 200, 200, 0xFF);
        static constexpr pu::ui::Color ThumbnailPlaceholderMarkColor = pu::ui::Color(130, 130, 130, 0xFF);
        static constexpr s32 CompletedBadgeMargin = 18;
        static constexpr s32 CompletedBadgeRadius = 32;
        static constexpr s32 CompletedBadgeCheckThickness = 5;
        // A lighter tint of FocusOutlineColor's blue, so the badge still
        // reads as part of the grid's accent language without looking as
        // heavy/dark as the focus outline itself.
        static constexpr pu::ui::Color CompletedBadgeColor = pu::ui::Color(90, 170, 240, 0xFF);
        static constexpr pu::ui::Color CompletedBadgeCheckColor = pu::ui::Color(255, 255, 255, 0xFF);
        // Same corner/margin as the completed badge (the two never show at
        // once), but sized to fit its "current/total" text instead of a
        // fixed circle.
        static constexpr s32 ProgressBadgeMargin = 18;
        static constexpr s32 ProgressBadgeHorizontalPadding = 14;
        static constexpr s32 ProgressBadgeVerticalPadding = 8;
        static constexpr s32 ProgressBadgeRadius = 16;

        // Maximum finger movement, in pixels, still considered a tap rather
        // than the start of a drag.
        static constexpr s32 TapMoveTolerance = 12;

        s32 GetCardWidth();
        s32 GetCardHeight();
        s32 GetRowHeight();
        s32 GetRowCount();
        s32 GetContentHeight();
        s32 GetMaxScrollY();
        s32 GetTitleAreaWidth();
        void SelectIndex(const size_t index);
        void MoveSelection(const s32 delta_index);
        void EnsureSelectedVisible();
        void ScrollBy(const s32 delta_y);
        void ResetCardMarquee(const size_t index);
        static pu::sdl2::TextureHandle::Ref BuildProgressTexture(const bool completed, const bool in_progress, const u32 current_page, const size_t page_count);
        static void RenderThumbnailCover(pu::ui::render::Renderer::Ref &drawer, pu::sdl2::TextureHandle::Ref thumbnail, const s32 x, const s32 y, const s32 w, const s32 h);
        static void RenderCompletedBadge(pu::ui::render::Renderer::Ref &drawer, const s32 card_x, const s32 card_y, const s32 card_w);
        static void RenderProgressBadge(pu::ui::render::Renderer::Ref &drawer, const s32 card_x, const s32 card_y, const s32 card_w, pu::sdl2::TextureHandle::Ref progress_tex);
        void HandleTap(const s32 touch_x, const s32 touch_y);

        s32 x;
        s32 y;
        s32 w;
        s32 h;
        s32 columns;
        bool input_enabled = true;
        size_t selected_index;
        s32 scroll_y;
        std::vector<Card> cards;
        OnItemSelected on_item_selected;
        bool touch_active;
        bool touch_moved;
        s32 touch_start_x;
        s32 touch_start_y;
        s32 touch_last_y;
};
