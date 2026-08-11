#include <MangaGrid.hpp>
#include <Lang.hpp>
#include <algorithm>
#include <cmath>

MangaGrid::MangaGrid(const s32 x, const s32 y, const s32 width, const s32 height, const s32 columns) : Element(), x(x), y(y), w(width), h(height), columns(columns), selected_index(0), scroll_y(0), touch_active(false), touch_moved(false), touch_start_x(0), touch_start_y(0), touch_last_y(0) {}

s32 MangaGrid::GetCardWidth() {
    const auto usable_w = this->w - (MangaGrid::GridPadding * 2);
    return (usable_w - (MangaGrid::CardSpacing * (this->columns - 1))) / this->columns;
}

s32 MangaGrid::GetCardHeight() {
    const auto thumbnail_h = (this->GetCardWidth() * MangaGrid::ThumbnailAspectDenominator) / MangaGrid::ThumbnailAspectNumerator;
    return thumbnail_h + MangaGrid::TitleAreaHeight;
}

s32 MangaGrid::GetRowHeight() {
    return this->GetCardHeight() + MangaGrid::CardSpacing;
}

s32 MangaGrid::GetRowCount() {
    return static_cast<s32>((this->cards.size() + this->columns - 1) / this->columns);
}

s32 MangaGrid::GetContentHeight() {
    return this->h - (MangaGrid::GridPadding * 2);
}

s32 MangaGrid::GetMaxScrollY() {
    const auto row_count = this->GetRowCount();
    if (row_count == 0) {
        return 0;
    }

    const auto content_h = (row_count * this->GetRowHeight()) - MangaGrid::CardSpacing;
    const auto max_scroll = content_h - this->GetContentHeight();
    return (max_scroll < 0) ? 0 : max_scroll;
}

s32 MangaGrid::GetTitleAreaWidth() {
    return this->GetCardWidth() - (MangaGrid::TitleHorizontalPadding * 2);
}

pu::sdl2::TextureHandle::Ref MangaGrid::BuildProgressTexture(const bool completed, const bool in_progress, const u32 current_page, const size_t page_count) {
    if (completed || !in_progress) {
        return nullptr;
    }

    const auto progress_text = std::to_string(current_page + 1) + "/" + std::to_string(page_count);
    return pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small), progress_text, MangaGrid::CompletedBadgeCheckColor));
}

void MangaGrid::AddItem(const std::string &title, pu::sdl2::TextureHandle::Ref thumbnail, const bool completed, const bool in_progress, const u32 current_page, const size_t page_count) {
    const auto font_name = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge);
    const auto title_area_w = static_cast<u32>(this->GetTitleAreaWidth());

    auto clamped_title_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(font_name, title, MangaGrid::TitleColor, title_area_w));
    auto full_title_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(font_name, title, MangaGrid::TitleColor));
    auto progress_tex = MangaGrid::BuildProgressTexture(completed, in_progress, current_page, page_count);

    this->cards.push_back({thumbnail, clamped_title_tex, full_title_tex, 0, 0, completed, progress_tex});
}

void MangaGrid::UpdateItemStatus(const size_t index, const bool completed, const bool in_progress, const u32 current_page, const size_t page_count) {
    if (index >= this->cards.size()) {
        return;
    }

    auto &card = this->cards.at(index);
    card.completed = completed;
    card.progress_tex = MangaGrid::BuildProgressTexture(completed, in_progress, current_page, page_count);
}

void MangaGrid::ClearItems() {
    this->cards.clear();
    this->selected_index = 0;
    this->scroll_y = 0;
}

void MangaGrid::ScrollBy(const s32 delta_y) {
    auto new_scroll = this->scroll_y + delta_y;
    const auto max_scroll = this->GetMaxScrollY();
    if (new_scroll < 0) {
        new_scroll = 0;
    }
    else if (new_scroll > max_scroll) {
        new_scroll = max_scroll;
    }
    this->scroll_y = new_scroll;
}

void MangaGrid::EnsureSelectedVisible() {
    const auto row = static_cast<s32>(this->selected_index) / this->columns;
    const auto row_h = this->GetRowHeight();
    const auto card_h = this->GetCardHeight();
    const auto card_top = row * row_h;
    const auto card_bottom = card_top + card_h;

    if (card_top < this->scroll_y) {
        this->scroll_y = card_top;
    }
    else if (card_bottom > (this->scroll_y + this->GetContentHeight())) {
        this->scroll_y = card_bottom - this->GetContentHeight();
    }

    const auto max_scroll = this->GetMaxScrollY();
    if (this->scroll_y > max_scroll) {
        this->scroll_y = max_scroll;
    }
    if (this->scroll_y < 0) {
        this->scroll_y = 0;
    }
}

void MangaGrid::ResetCardMarquee(const size_t index) {
    auto &card = this->cards.at(index);
    card.marquee_x = 0;
    card.marquee_delay = 0;
}

void MangaGrid::SelectIndex(const size_t index) {
    this->ResetCardMarquee(this->selected_index);
    this->selected_index = index;
    this->ResetCardMarquee(this->selected_index);
}

void MangaGrid::MoveSelection(const s32 delta_index) {
    const auto new_index = static_cast<s32>(this->selected_index) + delta_index;
    if ((new_index < 0) || (static_cast<size_t>(new_index) >= this->cards.size())) {
        return;
    }

    this->SelectIndex(static_cast<size_t>(new_index));
    this->EnsureSelectedVisible();
}

void MangaGrid::HandleTap(const s32 touch_x, const s32 touch_y) {
    const auto origin_x = this->GetProcessedX() + MangaGrid::GridPadding;
    const auto origin_y = this->GetProcessedY() + MangaGrid::GridPadding;

    const auto rel_y = touch_y - origin_y + this->scroll_y;
    if (rel_y < 0) {
        return;
    }

    const auto row_h = this->GetRowHeight();
    const auto card_h = this->GetCardHeight();
    const auto row = rel_y / row_h;
    if ((rel_y - (row * row_h)) >= card_h) {
        return; // Tapped in the gap between rows.
    }

    const auto rel_x = touch_x - origin_x;
    if (rel_x < 0) {
        return;
    }

    const auto card_w = this->GetCardWidth();
    const auto column_stride = card_w + MangaGrid::CardSpacing;
    const auto col = rel_x / column_stride;
    if ((col >= this->columns) || ((rel_x - (col * column_stride)) >= card_w)) {
        return; // Tapped in the gap between columns, or past the last one.
    }

    const auto index = static_cast<size_t>((row * this->columns) + col);
    if (index >= this->cards.size()) {
        return;
    }

    this->SelectIndex(index);
    if (this->on_item_selected) {
        this->on_item_selected(index);
    }
}

void MangaGrid::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
    if (this->cards.empty()) {
        return;
    }

    const auto card_w = this->GetCardWidth();
    const auto card_h = this->GetCardHeight();
    const auto row_h = this->GetRowHeight();
    const auto thumbnail_h = card_h - MangaGrid::TitleAreaHeight;
    const auto row_count = this->GetRowCount();

    const auto renderer = pu::ui::render::GetMainRenderer();
    const SDL_Rect clip_rect = { x, y, this->w, this->h };
    SDL_RenderSetClipRect(renderer, &clip_rect);

    const auto content_x = x + MangaGrid::GridPadding;
    const auto content_y = y + MangaGrid::GridPadding;
    const auto content_bottom = content_y + this->GetContentHeight();

    const auto first_row = this->scroll_y / row_h;
    for (auto row = first_row; row < row_count; row++) {
        const auto card_y = content_y + (row * row_h) - this->scroll_y;
        if (card_y >= content_bottom) {
            break;
        }

        for (s32 col = 0; col < this->columns; col++) {
            const size_t index = (static_cast<size_t>(row) * this->columns) + col;
            if (index >= this->cards.size()) {
                break;
            }

            const auto card_x = content_x + (col * (card_w + MangaGrid::CardSpacing));
            auto &card = this->cards.at(index);
            const auto is_selected = (index == this->selected_index);

            if (is_selected) {
                for (s32 i = 0; i < MangaGrid::FocusOutlineThickness; i++) {
                    drawer->RenderRoundedRectangle(MangaGrid::FocusOutlineColor, card_x - i - 1, card_y - i - 1, card_w + ((i + 1) * 2), card_h + ((i + 1) * 2), MangaGrid::CardRadius);
                }
            }

            // Single rounded background spanning the whole card (image area +
            // title strip), sharing the outline's exact radius, so the two
            // stay concentric and the outline never looks cut at the corners.
            drawer->RenderRoundedRectangleFill(MangaGrid::CardColor, card_x, card_y, card_w, card_h, MangaGrid::CardRadius);

            const auto margin = MangaGrid::CardFrameMargin;
            MangaGrid::RenderThumbnailCover(drawer, card.thumbnail, card_x + margin, card_y + margin, card_w - (margin * 2), thumbnail_h - (margin * 2));

            if (card.completed) {
                MangaGrid::RenderCompletedBadge(drawer, card_x, card_y, card_w);
            }
            else if (card.progress_tex != nullptr) {
                MangaGrid::RenderProgressBadge(drawer, card_x, card_y, card_w, card.progress_tex);
            }

            const auto title_area_w = card_w - (MangaGrid::TitleHorizontalPadding * 2);
            const auto full_title_w = (card.full_title_tex != nullptr) ? pu::ui::render::GetTextureWidth(card.full_title_tex->Get()) : 0;
            const auto title_overflows = full_title_w > title_area_w;

            if (is_selected && title_overflows) {
                const auto title_tex = card.full_title_tex->Get();
                const auto title_h = pu::ui::render::GetTextureHeight(title_tex);
                const auto title_x = card_x + MangaGrid::TitleHorizontalPadding;
                const auto title_y = card_y + thumbnail_h + ((MangaGrid::TitleAreaHeight - title_h) / 2);

                drawer->RenderTexture(title_tex, title_x, title_y, pu::ui::render::TextureRenderOptions({}, title_area_w, {}, {}, card.marquee_x, 0));

                if (card.marquee_x >= (full_title_w - title_area_w)) {
                    card.marquee_delay++;
                    if (card.marquee_delay >= MangaGrid::TitleMarqueeDelaySteps) {
                        card.marquee_x = 0;
                        card.marquee_delay = 0;
                    }
                }
                else if (card.marquee_x == 0) {
                    card.marquee_delay++;
                    if (card.marquee_delay >= MangaGrid::TitleMarqueeDelaySteps) {
                        card.marquee_x++;
                        card.marquee_delay = 0;
                    }
                }
                else {
                    card.marquee_x += MangaGrid::TitleMarqueeSpeed;
                }
            }
            else if (card.clamped_title_tex != nullptr) {
                const auto title_tex = card.clamped_title_tex->Get();
                const auto title_w = pu::ui::render::GetTextureWidth(title_tex);
                const auto title_h = pu::ui::render::GetTextureHeight(title_tex);
                const auto title_x = card_x + ((card_w - title_w) / 2);
                const auto title_y = card_y + thumbnail_h + ((MangaGrid::TitleAreaHeight - title_h) / 2);
                drawer->RenderTexture(title_tex, title_x, title_y);
            }
        }
    }

    SDL_RenderSetClipRect(renderer, nullptr);
}

void MangaGrid::RenderThumbnailCover(pu::ui::render::Renderer::Ref &drawer, pu::sdl2::TextureHandle::Ref thumbnail, const s32 x, const s32 y, const s32 w, const s32 h) {
    if ((w <= 0) || (h <= 0)) {
        return;
    }

    if (thumbnail == nullptr) {
        drawer->RenderRectangleFill(MangaGrid::ThumbnailPlaceholderColor, x, y, w, h);

        static pu::sdl2::TextureHandle::Ref mark_tex = nullptr;
        if (mark_tex == nullptr) {
            mark_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large), lang::Get("manga_grid.cover_placeholder"), MangaGrid::ThumbnailPlaceholderMarkColor));
        }

        const auto tex = mark_tex->Get();
        const auto mark_w = pu::ui::render::GetTextureWidth(tex);
        const auto mark_h = pu::ui::render::GetTextureHeight(tex);
        drawer->RenderTexture(tex, x + ((w - mark_w) / 2), y + ((h - mark_h) / 2));
        return;
    }

    const auto tex = thumbnail->Get();
    s32 tex_w = 0;
    s32 tex_h = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &tex_w, &tex_h);
    if ((tex_w <= 0) || (tex_h <= 0)) {
        return;
    }

    // pu::ui::render::Renderer::RenderTexture can either stretch the whole
    // texture to fit, or crop a region at 1:1 scale, but not both at once, so
    // a "cover" crop (scale to fill, cropping the overflow) needs a direct
    // SDL_RenderCopy call with independent source/destination rects.
    const auto dst_aspect = static_cast<double>(w) / h;
    const auto src_aspect = static_cast<double>(tex_w) / tex_h;

    SDL_Rect src_rect;
    if (src_aspect > dst_aspect) {
        src_rect.h = tex_h;
        src_rect.w = static_cast<s32>(tex_h * dst_aspect);
        src_rect.x = (tex_w - src_rect.w) / 2;
        src_rect.y = 0;
    }
    else {
        src_rect.w = tex_w;
        src_rect.h = static_cast<s32>(tex_w / dst_aspect);
        src_rect.x = 0;
        src_rect.y = (tex_h - src_rect.h) / 2;
    }

    SDL_Rect dst_rect = { x, y, w, h };
    SDL_RenderCopy(pu::ui::render::GetMainRenderer(), tex, &src_rect, &dst_rect);
}

namespace {

    // Draws a line with actual width (SDL_RenderDrawLine is always 1px) by
    // offsetting several copies of it along its own perpendicular, rather
    // than along x/y, so it thickens evenly regardless of the line's angle.
    void RenderThickLine(SDL_Renderer *renderer, const double x1, const double y1, const double x2, const double y2, const s32 thickness) {
        const auto dx = x2 - x1;
        const auto dy = y2 - y1;
        const auto length = std::sqrt((dx * dx) + (dy * dy));
        if (length <= 0.0) {
            return;
        }

        const auto perp_x = -dy / length;
        const auto perp_y = dx / length;
        const auto half = (thickness - 1) / 2.0;

        for (s32 i = 0; i < thickness; i++) {
            const auto offset = i - half;
            const auto ox = perp_x * offset;
            const auto oy = perp_y * offset;
            SDL_RenderDrawLine(renderer, static_cast<s32>(x1 + ox), static_cast<s32>(y1 + oy), static_cast<s32>(x2 + ox), static_cast<s32>(y2 + oy));
        }
    }

}

void MangaGrid::RenderCompletedBadge(pu::ui::render::Renderer::Ref &drawer, const s32 card_x, const s32 card_y, const s32 card_w) {
    const auto radius = MangaGrid::CompletedBadgeRadius;
    const auto center_x = card_x + card_w - MangaGrid::CompletedBadgeMargin - radius;
    const auto center_y = card_y + MangaGrid::CompletedBadgeMargin + radius;

    drawer->RenderRoundedRectangleFill(MangaGrid::CompletedBadgeColor, center_x - radius, center_y - radius, radius * 2, radius * 2, radius);

    const auto renderer = pu::ui::render::GetMainRenderer();
    SDL_SetRenderDrawColor(renderer, MangaGrid::CompletedBadgeCheckColor.r, MangaGrid::CompletedBadgeCheckColor.g, MangaGrid::CompletedBadgeCheckColor.b, MangaGrid::CompletedBadgeCheckColor.a);

    // A simple check mark: a short stroke down-right, then a longer stroke
    // up-right, both relative to the badge's own radius so it scales with it.
    const auto ax = center_x - (radius * 0.5);
    const auto ay = center_y;
    const auto bx = center_x - (radius * 0.1);
    const auto by = center_y + (radius * 0.35);
    const auto cx = center_x + (radius * 0.5);
    const auto cy = center_y - (radius * 0.35);

    RenderThickLine(renderer, ax, ay, bx, by, MangaGrid::CompletedBadgeCheckThickness);
    RenderThickLine(renderer, bx, by, cx, cy, MangaGrid::CompletedBadgeCheckThickness);
}

void MangaGrid::RenderProgressBadge(pu::ui::render::Renderer::Ref &drawer, const s32 card_x, const s32 card_y, const s32 card_w, pu::sdl2::TextureHandle::Ref progress_tex) {
    if (progress_tex == nullptr) {
        return;
    }

    const auto tex = progress_tex->Get();
    const auto text_w = pu::ui::render::GetTextureWidth(tex);
    const auto text_h = pu::ui::render::GetTextureHeight(tex);

    const auto badge_w = text_w + (MangaGrid::ProgressBadgeHorizontalPadding * 2);
    const auto badge_h = text_h + (MangaGrid::ProgressBadgeVerticalPadding * 2);
    const auto badge_x = card_x + card_w - MangaGrid::ProgressBadgeMargin - badge_w;
    const auto badge_y = card_y + MangaGrid::ProgressBadgeMargin;

    drawer->RenderRoundedRectangleFill(MangaGrid::CompletedBadgeColor, badge_x, badge_y, badge_w, badge_h, MangaGrid::ProgressBadgeRadius);
    drawer->RenderTexture(tex, badge_x + MangaGrid::ProgressBadgeHorizontalPadding, badge_y + MangaGrid::ProgressBadgeVerticalPadding);
}

void MangaGrid::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
    if (!this->input_enabled || this->cards.empty()) {
        return;
    }

    if (!touch_pos.IsEmpty()) {
        if (!this->touch_active) {
            this->touch_active = true;
            this->touch_moved = false;
            this->touch_start_x = touch_pos.x;
            this->touch_start_y = touch_pos.y;
            this->touch_last_y = touch_pos.y;
        }
        else {
            const auto frame_delta = touch_pos.y - this->touch_last_y;
            if (frame_delta != 0) {
                this->ScrollBy(-frame_delta);
                this->touch_last_y = touch_pos.y;
            }

            const auto total_delta = touch_pos.y - this->touch_start_y;
            const auto abs_total_delta = (total_delta < 0) ? -total_delta : total_delta;
            if (abs_total_delta >= MangaGrid::TapMoveTolerance) {
                this->touch_moved = true;
            }
        }
        return;
    }

    if (this->touch_active) {
        if (!this->touch_moved) {
            this->HandleTap(this->touch_start_x, this->touch_start_y);
        }
        this->touch_active = false;
        return;
    }

    if (keys_down & (HidNpadButton_Right | HidNpadButton_StickLRight)) {
        this->MoveSelection(1);
    }
    else if (keys_down & (HidNpadButton_Left | HidNpadButton_StickLLeft)) {
        this->MoveSelection(-1);
    }
    else if (keys_down & (HidNpadButton_Down | HidNpadButton_StickLDown)) {
        this->MoveSelection(this->columns);
    }
    else if (keys_down & (HidNpadButton_Up | HidNpadButton_StickLUp)) {
        this->MoveSelection(-this->columns);
    }
    else if (keys_down & HidNpadButton_A) {
        if (this->on_item_selected) {
            this->on_item_selected(this->selected_index);
        }
    }
}
