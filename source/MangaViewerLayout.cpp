#include <MangaViewerLayout.hpp>
#include <cmath>

MangaViewerLayout::MangaViewerLayout(const std::string &manga_path) : Layout::Layout(), source(manga::OpenMangaSource(manga_path)), page_count(this->source ? this->source->GetPageCount() : 0), current_page(0), mode(ViewMode::Vertical), orientation(ReadingOrientation::Horizontal), tex_width(0), tex_height(0), target_size(0), image_width(0), image_height(0), scroll_x(0), scroll_y(0), max_scroll_x(0), max_scroll_y(0), center_offset_x(0), center_offset_y(0), menu_open(false), touch_active(false), touch_moved(false), touch_start_x(0), touch_start_y(0), touch_last_x(0), touch_last_y(0), pinch_active(false), pinch_last_distance(0.0), touch_had_multitouch(false) {
    this->SetBackgroundColor(pu::ui::Color(0, 0, 0, 0xFF));

    this->pageIndicator = pu::ui::elm::TextBlock::New(1700, 20, "");
    this->pageIndicator->SetColor(pu::ui::Color(255, 255, 255, 0xFF));
    this->pageIndicatorBg = RoundedRectangle::New(0, 0, 0, 0, pu::ui::Color(0, 0, 0, 160), MangaViewerLayout::PageIndicatorBorderRadius);

    if (this->page_count > 0) {
        this->LoadPage(0);
    }
    else {
        this->SetPageIndicatorText("Sin imagenes");
    }

    this->Add(this->pageIndicatorBg);
    this->Add(this->pageIndicator);

    const auto screen_w = static_cast<s32>(pu::ui::render::ScreenWidth);
    const auto screen_h = static_cast<s32>(pu::ui::render::ScreenHeight);

    const auto panel_w = MangaViewerLayout::MenuPanelWidth;
    const auto panel_h = screen_h - (MangaViewerLayout::MenuPanelMargin * 2);
    const auto panel_x = screen_w - panel_w - MangaViewerLayout::MenuPanelMargin;
    const auto panel_y = MangaViewerLayout::MenuPanelMargin;
    const auto content_x = panel_x + MangaViewerLayout::MenuPanelInset;

    this->menuBg = RoundedRectangle::New(panel_x, panel_y, panel_w, panel_h, MangaViewerLayout::MenuPanelColor, MangaViewerLayout::MenuPanelBorderRadius);
    this->menuBg->SetVisible(false);
    this->Add(this->menuBg);

    this->menuTitle = pu::ui::elm::TextBlock::New(content_x, panel_y + 28, "Opciones");
    this->menuTitle->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->menuTitle->SetColor(MangaViewerLayout::MenuItemTextColor);
    this->menuTitle->SetVisible(false);
    this->Add(this->menuTitle);

    const auto divider_y = this->menuTitle->GetY() + this->menuTitle->GetHeight() + 20;
    this->menuDivider = pu::ui::elm::Rectangle::New(content_x, divider_y, panel_w - (MangaViewerLayout::MenuPanelInset * 2), 3, MangaViewerLayout::MenuAccentColor);
    this->menuDivider->SetVisible(false);
    this->Add(this->menuDivider);

    const auto menu_y = divider_y + 24;
    const auto menu_w = panel_w - (MangaViewerLayout::MenuItemsInset * 2);
    const auto menu_item_x = panel_x + MangaViewerLayout::MenuItemsInset;

    this->menu = pu::ui::elm::Menu::New(menu_item_x, menu_y, menu_w, MangaViewerLayout::MenuPanelColor, MangaViewerLayout::MenuPanelColor, MangaViewerLayout::MenuItemHeight, 3);
    this->menu->SetVisible(false);

    this->orientationItem = pu::ui::elm::MenuItem::New(this->GetOrientationLabel());
    this->orientationItem->SetColor(MangaViewerLayout::MenuItemTextColor);
    const auto orientation_cb = [this]() {
        this->ToggleOrientation();
    };
    this->orientationItem->AddOnKey(orientation_cb);
    this->orientationItem->AddOnKey(orientation_cb, pu::ui::TouchPseudoKey);
    this->menu->AddItem(this->orientationItem);

    auto back_item = pu::ui::elm::MenuItem::New("Volver a la lista");
    back_item->SetColor(MangaViewerLayout::MenuItemTextColor);
    const auto back_cb = [this]() {
        this->SetMenuVisible(false);
        if (this->on_back) {
            this->on_back();
        }
    };
    back_item->AddOnKey(back_cb);
    back_item->AddOnKey(back_cb, pu::ui::TouchPseudoKey);
    this->menu->AddItem(back_item);

    auto close_item = pu::ui::elm::MenuItem::New("Cerrar menu");
    close_item->SetColor(MangaViewerLayout::MenuItemTextColor);
    const auto close_cb = [this]() {
        this->SetMenuVisible(false);
    };
    close_item->AddOnKey(close_cb);
    close_item->AddOnKey(close_cb, pu::ui::TouchPseudoKey);
    this->menu->AddItem(close_item);

    this->Add(this->menu);

    for (u32 i = 0; i < this->menu->GetItems().size(); i++) {
        const auto outline_y = menu_y + (static_cast<s32>(i) * MangaViewerLayout::MenuItemHeight) + MangaViewerLayout::MenuOutlineMarginY;
        const auto outline_h = MangaViewerLayout::MenuItemHeight - (MangaViewerLayout::MenuOutlineMarginY * 2);
        auto outline = RoundedOutlineRectangle::New(menu_item_x, outline_y, menu_w, outline_h, MangaViewerLayout::MenuOutlineIdleColor, MangaViewerLayout::MenuOutlineRadius, MangaViewerLayout::MenuOutlineThickness);
        outline->SetVisible(false);
        this->menuItemOutlines.push_back(outline);
        this->Add(outline);
    }

    this->menu->SetOnSelectionChanged([this]() {
        this->UpdateMenuItemOutlines();
    });
    this->UpdateMenuItemOutlines();

    this->menuFooter = pu::ui::elm::TextBlock::New(content_x, panel_y + panel_h - 56, "A Seleccionar    B Cerrar");
    this->menuFooter->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->menuFooter->SetColor(MangaViewerLayout::MenuFooterTextColor);
    this->menuFooter->SetVisible(false);
    this->Add(this->menuFooter);

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_X) {
            this->SetMenuVisible(!this->menu_open);
            return;
        }

        if (this->menu_open) {
            if (keys_down & HidNpadButton_B) {
                this->SetMenuVisible(false);
            }
            return;
        }

        // pu::ui only surfaces a single touch point through touch_pos, so
        // detecting a second finger (for pinch-to-zoom) needs the raw HID
        // touch state directly.
        HidTouchScreenState raw_touch_state = {};
        hidGetTouchScreenStates(&raw_touch_state, 1);

        if (raw_touch_state.count >= 2) {
            this->touch_active = false;
            this->touch_had_multitouch = true;

            const auto x0 = raw_touch_state.touches[0].x * pu::ui::render::ScreenFactor;
            const auto y0 = raw_touch_state.touches[0].y * pu::ui::render::ScreenFactor;
            const auto x1 = raw_touch_state.touches[1].x * pu::ui::render::ScreenFactor;
            const auto y1 = raw_touch_state.touches[1].y * pu::ui::render::ScreenFactor;
            const auto dx = x1 - x0;
            const auto dy = y1 - y0;
            const auto distance = std::sqrt((dx * dx) + (dy * dy));

            if (this->pinch_active) {
                const auto delta = distance - this->pinch_last_distance;
                this->AdjustZoom(static_cast<s32>(delta * MangaViewerLayout::PinchZoomSensitivity));
            }
            this->pinch_active = true;
            this->pinch_last_distance = distance;
            return;
        }
        this->pinch_active = false;

        if (!touch_pos.IsEmpty()) {
            // Remap the raw (real, never-rotated) touch point into logical
            // coordinates before doing anything else, so every check below
            // (drag deltas, tap zones) can stay written as if the console
            // were always held in Horizontal orientation.
            const auto touch_x = this->ToLogicalTouchX(touch_pos.x, touch_pos.y);
            const auto touch_y = this->ToLogicalTouchY(touch_pos.x, touch_pos.y);

            if (!this->touch_active) {
                this->touch_active = true;
                // If this finger is the tail end of a pinch (the other one
                // lifted a frame earlier), don't treat its eventual release
                // as a fresh tap.
                this->touch_moved = this->touch_had_multitouch;
                this->touch_start_x = touch_x;
                this->touch_start_y = touch_y;
                this->touch_last_x = touch_x;
                this->touch_last_y = touch_y;
            }
            else {
                const auto delta_x = touch_x - this->touch_last_x;
                const auto delta_y = touch_y - this->touch_last_y;
                if ((delta_x != 0) || (delta_y != 0)) {
                    this->SetScroll(this->scroll_x - delta_x, this->scroll_y - delta_y);
                    this->touch_last_x = touch_x;
                    this->touch_last_y = touch_y;
                }

                const auto total_dx = touch_x - this->touch_start_x;
                const auto total_dy = touch_y - this->touch_start_y;
                const auto abs_dx = (total_dx < 0) ? -total_dx : total_dx;
                const auto abs_dy = (total_dy < 0) ? -total_dy : total_dy;
                if ((abs_dx >= MangaViewerLayout::TapMoveTolerance) || (abs_dy >= MangaViewerLayout::TapMoveTolerance)) {
                    this->touch_moved = true;
                }
            }
            return;
        }

        if (this->touch_active) {
            this->touch_active = false;
            if (!this->touch_moved) {
                const auto screen_mid_x = this->GetLogicalScreenWidth() / 2;
                if (this->touch_start_x < screen_mid_x) {
                    if (this->current_page > 0) {
                        this->LoadPage(this->current_page - 1);
                    }
                }
                else {
                    if ((this->current_page + 1) < this->page_count) {
                        this->LoadPage(this->current_page + 1);
                    }
                }
            }
            return;
        }

        if (touch_pos.IsEmpty()) {
            this->touch_had_multitouch = false;
        }

        if (keys_down & HidNpadButton_R) {
            if ((this->current_page + 1) < this->page_count) {
                this->LoadPage(this->current_page + 1);
            }
        }
        else if (keys_down & HidNpadButton_L) {
            if (this->current_page > 0) {
                this->LoadPage(this->current_page - 1);
            }
        }
        else if (keys_down & HidNpadButton_Y) {
            switch (this->mode) {
                case ViewMode::Vertical:
                    this->mode = ViewMode::Intermediate;
                    break;
                case ViewMode::Intermediate:
                    this->mode = ViewMode::Horizontal;
                    break;
                case ViewMode::Horizontal:
                    this->mode = ViewMode::Vertical;
                    break;
            }
            this->ApplyViewMode();
        }
        else if (keys_down & HidNpadButton_B) {
            if (this->on_back) {
                this->on_back();
            }
        }

        if (keys_held & HidNpadButton_StickLLeft) {
            this->SetScroll(this->scroll_x - MangaViewerLayout::ScrollSpeed, this->scroll_y);
        }
        else if (keys_held & HidNpadButton_StickLRight) {
            this->SetScroll(this->scroll_x + MangaViewerLayout::ScrollSpeed, this->scroll_y);
        }

        if (keys_held & HidNpadButton_StickLUp) {
            this->SetScroll(this->scroll_x, this->scroll_y - MangaViewerLayout::ScrollSpeed);
        }
        else if (keys_held & HidNpadButton_StickLDown) {
            this->SetScroll(this->scroll_x, this->scroll_y + MangaViewerLayout::ScrollSpeed);
        }

        if (keys_held & HidNpadButton_StickRUp) {
            this->AdjustZoom(MangaViewerLayout::ZoomSpeed);
        }
        else if (keys_held & HidNpadButton_StickRDown) {
            this->AdjustZoom(-MangaViewerLayout::ZoomSpeed);
        }
    });
}

void MangaViewerLayout::LoadPage(const u32 index) {
    if (index >= this->page_count) {
        return;
    }

    const auto data = this->source->ReadPage(index);
    if (data.empty()) {
        return;
    }

    auto tex = pu::ui::render::LoadImageFromBuffer(data.data(), data.size());
    if (tex == nullptr) {
        return;
    }

    this->tex_width = pu::ui::render::GetTextureWidth(tex);
    this->tex_height = pu::ui::render::GetTextureHeight(tex);

    auto tex_handle = pu::sdl2::TextureHandle::New(tex);
    if (this->pageImage == nullptr) {
        this->pageImage = pu::ui::elm::Image::New(0, 0, tex_handle);
        this->Add(this->pageImage);
    }
    else {
        this->pageImage->SetImage(tex_handle);
    }

    this->current_page = index;
    if (this->target_size <= 0) {
        this->ApplyViewMode();
    }
    else {
        this->ApplyCurrentMode();
    }
    this->SetPageIndicatorText(std::to_string(index + 1) + " / " + std::to_string(this->page_count));
}

void MangaViewerLayout::SetPageIndicatorText(const std::string &text) {
    this->pageIndicator->SetText(text);

    const auto padding = MangaViewerLayout::PageIndicatorPadding;
    this->pageIndicatorBg->SetX(this->pageIndicator->GetX() - padding);
    this->pageIndicatorBg->SetY(this->pageIndicator->GetY() - padding);
    this->pageIndicatorBg->SetWidth(this->pageIndicator->GetWidth() + (padding * 2));
    this->pageIndicatorBg->SetHeight(this->pageIndicator->GetHeight() + (padding * 2));
}

void MangaViewerLayout::SetMenuVisible(const bool visible) {
    this->menu_open = visible;
    if (visible) {
        this->UpdateOrientationMenuItemLabel();
    }
    this->menuBg->SetVisible(visible);
    this->menuTitle->SetVisible(visible);
    this->menuDivider->SetVisible(visible);
    this->menu->SetVisible(visible);
    this->menuFooter->SetVisible(visible);
    for (auto &outline : this->menuItemOutlines) {
        outline->SetVisible(visible);
    }
}

void MangaViewerLayout::UpdateMenuItemOutlines() {
    const auto selected = this->menu->GetSelectedIndex();
    for (size_t i = 0; i < this->menuItemOutlines.size(); i++) {
        const auto is_selected = (static_cast<s32>(i) == selected);
        this->menuItemOutlines.at(i)->SetOutlineColor(is_selected ? MangaViewerLayout::MenuOutlineFocusColor : MangaViewerLayout::MenuOutlineIdleColor);
    }
}

std::string MangaViewerLayout::GetOrientationLabel() const {
    return (this->orientation == ReadingOrientation::Vertical) ? "Vista: Vertical" : "Vista: Horizontal";
}

void MangaViewerLayout::UpdateOrientationMenuItemLabel() {
    this->orientationItem->SetName(this->GetOrientationLabel());
    this->menu->ForceReloadItems();
}

void MangaViewerLayout::ToggleOrientation() {
    this->orientation = (this->orientation == ReadingOrientation::Vertical) ? ReadingOrientation::Horizontal : ReadingOrientation::Vertical;
    // Screen space swaps axes, so re-run the current fit mode against the
    // new logical dimensions and reset scroll, exactly like a resize.
    this->ApplyViewMode();
    this->UpdateOrientationMenuItemLabel();
}

s32 MangaViewerLayout::GetLogicalScreenWidth() const {
    return (this->orientation == ReadingOrientation::Vertical) ? static_cast<s32>(pu::ui::render::ScreenHeight) : static_cast<s32>(pu::ui::render::ScreenWidth);
}

s32 MangaViewerLayout::GetLogicalScreenHeight() const {
    return (this->orientation == ReadingOrientation::Vertical) ? static_cast<s32>(pu::ui::render::ScreenWidth) : static_cast<s32>(pu::ui::render::ScreenHeight);
}

s32 MangaViewerLayout::ToLogicalTouchX(const s32 real_x, const s32 real_y) const {
    if (this->orientation == ReadingOrientation::Vertical) {
        return real_y;
    }
    return real_x;
}

s32 MangaViewerLayout::ToLogicalTouchY(const s32 real_x, const s32 real_y) const {
    if (this->orientation == ReadingOrientation::Vertical) {
        return static_cast<s32>(pu::ui::render::ScreenWidth) - real_x;
    }
    return real_y;
}

void MangaViewerLayout::ApplyViewMode() {
    if ((this->pageImage == nullptr) || (this->tex_width <= 0) || (this->tex_height <= 0)) {
        return;
    }

    const auto logical_screen_w = this->GetLogicalScreenWidth();
    const auto logical_screen_h = this->GetLogicalScreenHeight();

    switch (this->mode) {
        case ViewMode::Vertical: {
            this->target_size = logical_screen_w;
            break;
        }
        case ViewMode::Intermediate: {
            const auto fit_height_width = static_cast<s32>((static_cast<double>(this->tex_width) * logical_screen_h) / this->tex_height);
            this->target_size = (logical_screen_w + fit_height_width) / 2;
            break;
        }
        case ViewMode::Horizontal: {
            this->target_size = logical_screen_h;
            break;
        }
    }

    this->ApplyCurrentMode();
}

void MangaViewerLayout::ApplyCurrentMode() {
    if ((this->pageImage == nullptr) || (this->tex_width <= 0) || (this->tex_height <= 0)) {
        return;
    }

    if (this->mode == ViewMode::Horizontal) {
        this->ApplyHeightMode(this->target_size);
    }
    else {
        this->ApplyWidthMode(this->target_size);
    }

    this->SetScroll(0, 0);
}

void MangaViewerLayout::ApplyWidthMode(const s32 width) {
    const auto height = static_cast<s32>((static_cast<double>(this->tex_height) * width) / this->tex_width);
    this->ApplyDimensions(width, height);
}

void MangaViewerLayout::ApplyHeightMode(const s32 height) {
    const auto width = static_cast<s32>((static_cast<double>(this->tex_width) * height) / this->tex_height);
    this->ApplyDimensions(width, height);
}

void MangaViewerLayout::ApplyDimensions(const s32 width, const s32 height) {
    this->image_width = width;
    this->image_height = height;

    const auto logical_screen_w = this->GetLogicalScreenWidth();
    const auto logical_screen_h = this->GetLogicalScreenHeight();

    this->max_scroll_x = width - logical_screen_w;
    if (this->max_scroll_x < 0) {
        this->center_offset_x = (logical_screen_w - width) / 2;
        this->max_scroll_x = 0;
    }
    else {
        this->center_offset_x = 0;
    }

    this->max_scroll_y = height - logical_screen_h;
    if (this->max_scroll_y < 0) {
        this->center_offset_y = (logical_screen_h - height) / 2;
        this->max_scroll_y = 0;
    }
    else {
        this->center_offset_y = 0;
    }

    this->UpdateImageTransform();
}

void MangaViewerLayout::AdjustZoom(const s32 delta) {
    if ((this->pageImage == nullptr) || (this->tex_width <= 0) || (this->tex_height <= 0)) {
        return;
    }

    const auto is_height_based = (this->mode == ViewMode::Horizontal);
    const auto screen_size = is_height_based ? this->GetLogicalScreenHeight() : this->GetLogicalScreenWidth();

    const auto min_size = static_cast<s32>(screen_size * MangaViewerLayout::MinZoomFraction);
    const auto max_size = static_cast<s32>(screen_size * MangaViewerLayout::MaxZoomFraction);

    auto size = this->target_size + delta;
    if (size < min_size) {
        size = min_size;
    }
    else if (size > max_size) {
        size = max_size;
    }

    this->target_size = size;
    if (is_height_based) {
        this->ApplyHeightMode(size);
    }
    else {
        this->ApplyWidthMode(size);
    }

    this->SetScroll(this->scroll_x, this->scroll_y);
}

void MangaViewerLayout::SetScroll(const s32 x, const s32 y) {
    auto clamped_x = x;
    if (clamped_x < 0) {
        clamped_x = 0;
    }
    else if (clamped_x > this->max_scroll_x) {
        clamped_x = this->max_scroll_x;
    }

    auto clamped_y = y;
    if (clamped_y < 0) {
        clamped_y = 0;
    }
    else if (clamped_y > this->max_scroll_y) {
        clamped_y = this->max_scroll_y;
    }

    this->scroll_x = clamped_x;
    this->scroll_y = clamped_y;
    this->UpdateImageTransform();
}

void MangaViewerLayout::UpdateImageTransform() {
    if (this->pageImage == nullptr) {
        return;
    }

    const auto logical_x = this->center_offset_x - this->scroll_x;
    const auto logical_y = this->center_offset_y - this->scroll_y;

    if (this->orientation == ReadingOrientation::Vertical) {
        // pageImage is drawn rotated clockwise on the real (never-rotated)
        // screen; read it by turning the console counter-clockwise, like a
        // book. SDL first stretches the whole texture into the rect we give
        // it, THEN rotates that rect about its own center — so the rect must
        // keep image_width/image_height as-is (already the correct aspect
        // ratio) or the stretch step distorts the page before it's ever
        // rotated. The swapped on-screen footprint falls out of rotating a
        // non-square rect; it needs no manual width/height swap here.
        const auto real_screen_w = static_cast<s32>(pu::ui::render::ScreenWidth);
        const auto logical_center_x = logical_x + (this->image_width / 2);
        const auto logical_center_y = logical_y + (this->image_height / 2);
        const auto real_center_x = real_screen_w - logical_center_y;
        const auto real_center_y = logical_center_x;

        this->pageImage->SetWidth(this->image_width);
        this->pageImage->SetHeight(this->image_height);
        this->pageImage->SetX(real_center_x - (this->image_width / 2));
        this->pageImage->SetY(real_center_y - (this->image_height / 2));
        this->pageImage->SetRotationAngle(MangaViewerLayout::PortraitRotationAngle);
    }
    else {
        this->pageImage->SetWidth(this->image_width);
        this->pageImage->SetHeight(this->image_height);
        this->pageImage->SetX(logical_x);
        this->pageImage->SetY(logical_y);
        this->pageImage->SetRotationAngle(0.0f);
    }
}
