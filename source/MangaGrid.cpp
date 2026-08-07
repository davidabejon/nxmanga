#include <MangaGrid.hpp>
#include <algorithm>

MangaGrid::MangaGrid(const s32 x, const s32 y, const s32 width, const s32 height, const s32 columns) : Element(), x(x), y(y), w(width), h(height), columns(columns), selected_index(0), first_visible_row(0) {}

s32 MangaGrid::GetCardWidth() {
    return (this->w - (MangaGrid::CardSpacing * (this->columns - 1))) / this->columns;
}

s32 MangaGrid::GetCardHeight() {
    const auto thumbnail_h = (this->GetCardWidth() * MangaGrid::ThumbnailAspectDenominator) / MangaGrid::ThumbnailAspectNumerator;
    return thumbnail_h + MangaGrid::TitleAreaHeight;
}

s32 MangaGrid::GetRowHeight() {
    return this->GetCardHeight() + MangaGrid::CardSpacing;
}

s32 MangaGrid::GetRowsToShow() {
    const auto rows = (this->h + MangaGrid::CardSpacing) / this->GetRowHeight();
    return (rows < 1) ? 1 : rows;
}

s32 MangaGrid::GetTitleAreaWidth() {
    return this->GetCardWidth() - (MangaGrid::TitleHorizontalPadding * 2);
}

void MangaGrid::AddItem(const std::string &title, pu::sdl2::TextureHandle::Ref thumbnail) {
    const auto font_name = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge);
    const auto title_area_w = static_cast<u32>(this->GetTitleAreaWidth());

    auto clamped_title_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(font_name, title, MangaGrid::TitleColor, title_area_w));
    auto full_title_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(font_name, title, MangaGrid::TitleColor));
    this->cards.push_back({thumbnail, clamped_title_tex, full_title_tex, 0, 0});
    this->UpdatePageIndicator();
}

void MangaGrid::ClearItems() {
    this->cards.clear();
    this->selected_index = 0;
    this->first_visible_row = 0;
    this->page_indicator_tex = nullptr;
}

void MangaGrid::UpdatePageIndicator() {
    const auto text = std::to_string(this->selected_index + 1) + " / " + std::to_string(this->cards.size());
    this->page_indicator_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium), text, MangaGrid::PageIndicatorTextColor));
}

void MangaGrid::EnsureSelectedRowVisible() {
    const auto row = static_cast<s32>(this->selected_index) / this->columns;
    const auto rows_to_show = this->GetRowsToShow();
    if (row < this->first_visible_row) {
        this->first_visible_row = row;
    }
    else if (row >= (this->first_visible_row + rows_to_show)) {
        this->first_visible_row = row - rows_to_show + 1;
    }
}

void MangaGrid::ResetCardMarquee(const size_t index) {
    auto &card = this->cards.at(index);
    card.marquee_x = 0;
    card.marquee_delay = 0;
}

void MangaGrid::MoveSelection(const s32 delta_index) {
    const auto new_index = static_cast<s32>(this->selected_index) + delta_index;
    if ((new_index < 0) || (static_cast<size_t>(new_index) >= this->cards.size())) {
        return;
    }

    this->ResetCardMarquee(this->selected_index);
    this->selected_index = static_cast<size_t>(new_index);
    this->ResetCardMarquee(this->selected_index);
    this->EnsureSelectedRowVisible();
    this->UpdatePageIndicator();
}

void MangaGrid::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
    if (this->cards.empty()) {
        return;
    }

    const auto card_w = this->GetCardWidth();
    const auto card_h = this->GetCardHeight();
    const auto row_h = this->GetRowHeight();
    const auto thumbnail_h = card_h - MangaGrid::TitleAreaHeight;
    const auto rows_to_show = this->GetRowsToShow();

    const auto row_count = static_cast<s32>((this->cards.size() + this->columns - 1) / this->columns);
    const auto last_row = std::min(row_count, this->first_visible_row + rows_to_show);

    for (auto row = this->first_visible_row; row < last_row; row++) {
        const auto card_y = y + ((row - this->first_visible_row) * row_h);
        for (s32 col = 0; col < this->columns; col++) {
            const size_t index = (static_cast<size_t>(row) * this->columns) + col;
            if (index >= this->cards.size()) {
                break;
            }

            const auto card_x = x + (col * (card_w + MangaGrid::CardSpacing));
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
            MangaGrid::RenderThumbnailCover(card.thumbnail, card_x + margin, card_y + margin, card_w - (margin * 2), thumbnail_h - (margin * 2));

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

    this->RenderPageIndicator(drawer, x, y);
}

void MangaGrid::RenderThumbnailCover(pu::sdl2::TextureHandle::Ref thumbnail, const s32 x, const s32 y, const s32 w, const s32 h) {
    if ((thumbnail == nullptr) || (w <= 0) || (h <= 0)) {
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

void MangaGrid::RenderPageIndicator(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
    if (this->page_indicator_tex == nullptr) {
        return;
    }

    const auto tex = this->page_indicator_tex->Get();
    const auto text_w = pu::ui::render::GetTextureWidth(tex);
    const auto text_h = pu::ui::render::GetTextureHeight(tex);

    const auto padding = MangaGrid::PageIndicatorPadding;
    const auto bg_w = text_w + (padding * 2);
    const auto bg_h = text_h + (padding * 2);
    const auto bg_x = x + this->w - bg_w;
    const auto bg_y = y + this->h - bg_h;

    drawer->RenderRoundedRectangleFill(MangaGrid::PageIndicatorBackgroundColor, bg_x, bg_y, bg_w, bg_h, MangaGrid::PageIndicatorBorderRadius);
    drawer->RenderTexture(tex, bg_x + padding, bg_y + padding);
}

void MangaGrid::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
    if (this->cards.empty()) {
        return;
    }

    if (keys_down & HidNpadButton_Right) {
        this->MoveSelection(1);
    }
    else if (keys_down & HidNpadButton_Left) {
        this->MoveSelection(-1);
    }
    else if (keys_down & HidNpadButton_Down) {
        this->MoveSelection(this->columns);
    }
    else if (keys_down & HidNpadButton_Up) {
        this->MoveSelection(-this->columns);
    }
    else if (keys_down & HidNpadButton_A) {
        if (this->on_item_selected) {
            this->on_item_selected(this->selected_index);
        }
    }
}
