#include <MangaViewerLayout.hpp>
#include <manga/ReadingProgress.hpp>
#include <Lang.hpp>
#include <algorithm>
#include <cmath>

MangaViewerLayout::MangaViewerLayout(const std::string &manga_path) : Layout::Layout(), manga_path(manga_path), progress_tracking_suspended(false), source(manga::OpenMangaSource(manga_path)), page_count(this->source ? this->source->GetPageCount() : 0), current_page(0), mode(ViewMode::Vertical), orientation(settings::GetReadingOrientation()), tex_width(0), tex_height(0), target_size(0), image_width(0), image_height(0), scroll_x(0), scroll_y(0), max_scroll_x(0), max_scroll_y(0), center_offset_x(0), center_offset_y(0), cascade_mode(false), cascade_catching_up(false), cascade_loaded_count(0), cascade_total_height(0), cascade_zoom_width(0), cascade_scroll_x(0), cascade_max_scroll_x(0), cascade_center_offset_x(0), touch_active(false), touch_moved(false), touch_start_x(0), touch_start_y(0), touch_last_x(0), touch_last_y(0), pinch_active(false), pinch_last_distance(0.0), touch_had_multitouch(false) {
    this->SetBackgroundColor(pu::ui::Color(0, 0, 0, 0xFF));

    // Pre-create every Image this Layout could ever need (one for
    // single-page mode, plus one per cascade page) and add them all before
    // any UI chrome below. pu::ui::Layout has no way to remove or reorder
    // elements once added, so an Image created lazily at runtime (e.g. from
    // a menu click, switching modes) would render ON TOP of chrome added
    // earlier instead of behind it. Pre-creating everything up front, in
    // this order, guarantees pages always stay beneath the indicator/menu.
    this->pageImage = pu::ui::elm::Image::New(0, 0, nullptr);
    this->pageImage->SetVisible(false);
    this->Add(this->pageImage);

    this->cascade_zoom_width = this->GetLogicalScreenWidth();

    this->cascade_images.reserve(this->page_count);
    this->cascade_heights.assign(this->page_count, 0);
    this->cascade_offsets.assign(this->page_count, 0);
    for (size_t i = 0; i < this->page_count; i++) {
        auto image = pu::ui::elm::Image::New(0, 0, nullptr);
        image->SetVisible(false);
        this->cascade_images.push_back(image);
        this->Add(image);
    }

    this->cascadeLoadingOverlay = pu::ui::elm::Rectangle::New(0, 0, static_cast<s32>(pu::ui::render::ScreenWidth), static_cast<s32>(pu::ui::render::ScreenHeight), pu::ui::Color(0, 0, 0, 0xFF));
    this->cascadeLoadingOverlay->SetVisible(false);
    this->Add(this->cascadeLoadingOverlay);

    this->pageIndicator = pu::ui::elm::TextBlock::New(1700, 20, "");
    this->pageIndicator->SetColor(pu::ui::Color(255, 255, 255, 0xFF));
    this->pageIndicatorBg = RoundedRectangle::New(0, 0, 0, 0, pu::ui::Color(0, 0, 0, 160), MangaViewerLayout::PageIndicatorBorderRadius);

    // Registered once here rather than inside EnterCascadeMode: pu::ui::Layout
    // has no way to remove a render callback, so registering it there would
    // add one more permanent no-op closure every time cascade mode toggles on.
    this->AddRenderCallback([this]() {
        this->AdvanceCascadeCatchup();
        this->AdvanceCascadeTexturePreload();
    });

    // A manga already fully read when opened always restarts at page 0, and
    // this session never overwrites its saved (completed) progress, even if
    // the user leaves again without reaching the last page.
    this->progress_tracking_suspended = manga::IsCompleted(this->manga_path);
    if ((this->page_count > 0) && !this->progress_tracking_suspended) {
        const auto progress = manga::GetProgress(this->manga_path);
        if (progress.page_count > 0) {
            this->current_page = (progress.current_page < static_cast<u32>(this->page_count)) ? progress.current_page : static_cast<u32>(this->page_count - 1);
        }
    }

    if (this->page_count == 0) {
        this->SetPageIndicatorText(lang::Get("manga_viewer.no_images"));
    }
    else if (settings::GetCascadeMode()) {
        this->SetCascadeMode(true);
    }
    else {
        this->LoadPage(this->current_page);
    }

    this->Add(this->pageIndicatorBg);
    this->Add(this->pageIndicator);

    this->sideMenu = SideMenu::New(this);

    this->sideMenu->AddItem([this]() {
        return this->GetOrientationLabel();
    }, [this]() {
        this->ToggleOrientation();
    });

    if (this->page_count > 0) {
        this->sideMenu->AddItem([this]() {
            return this->GetCascadeModeLabel();
        }, [this]() {
            this->SetCascadeMode(!this->cascade_mode);
            this->sideMenu->RefreshLabels();
        });
    }

    this->sideMenu->AddItem([]() {
        return lang::Get("manga_viewer.back_to_list");
    }, [this]() {
        this->sideMenu->SetOpen(false);
        if (this->on_back) {
            this->on_back();
        }
    });

    this->sideMenu->AddItem([]() {
        return lang::Get("common.side_menu_close");
    }, [this]() {
        this->sideMenu->SetOpen(false);
    });

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (this->sideMenu->HandleInput(keys_down, keys_up, keys_held, touch_pos)) {
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
                if (this->cascade_mode) {
                    this->AdjustCascadeZoom(static_cast<s32>(delta * MangaViewerLayout::PinchZoomSensitivity));
                }
                else {
                    this->AdjustZoom(static_cast<s32>(delta * MangaViewerLayout::PinchZoomSensitivity));
                }
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
                    if (this->cascade_mode) {
                        this->SetCascadeScroll(this->scroll_y - delta_y);
                        this->SetCascadeScrollX(this->cascade_scroll_x - delta_x);
                    }
                    else {
                        this->SetScroll(this->scroll_x - delta_x, this->scroll_y - delta_y);
                    }
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
            // Cascade mode has no discrete page to flip to; navigation there
            // is drag/stick scrolling only.
            if (!this->touch_moved && !this->cascade_mode) {
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
            if (this->cascade_mode) {
                this->SetCascadeScroll(this->scroll_y + this->GetLogicalScreenHeight());
            }
            else if ((this->current_page + 1) < this->page_count) {
                this->LoadPage(this->current_page + 1);
            }
        }
        else if (keys_down & HidNpadButton_L) {
            if (this->cascade_mode) {
                this->SetCascadeScroll(this->scroll_y - this->GetLogicalScreenHeight());
            }
            else if (this->current_page > 0) {
                this->LoadPage(this->current_page - 1);
            }
        }
        else if ((keys_down & HidNpadButton_Y) && !this->cascade_mode) {
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

        if (this->cascade_mode) {
            if (keys_held & HidNpadButton_StickLLeft) {
                this->SetCascadeScrollX(this->cascade_scroll_x - MangaViewerLayout::ScrollSpeed);
            }
            else if (keys_held & HidNpadButton_StickLRight) {
                this->SetCascadeScrollX(this->cascade_scroll_x + MangaViewerLayout::ScrollSpeed);
            }

            if (keys_held & HidNpadButton_StickLUp) {
                this->SetCascadeScroll(this->scroll_y - MangaViewerLayout::ScrollSpeed);
            }
            else if (keys_held & HidNpadButton_StickLDown) {
                this->SetCascadeScroll(this->scroll_y + MangaViewerLayout::ScrollSpeed);
            }

            if (keys_held & HidNpadButton_StickRUp) {
                this->AdjustCascadeZoom(MangaViewerLayout::ZoomSpeed);
            }
            else if (keys_held & HidNpadButton_StickRDown) {
                this->AdjustCascadeZoom(-MangaViewerLayout::ZoomSpeed);
            }
        }
        else {
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

    this->pageImage->SetImage(pu::sdl2::TextureHandle::New(tex));
    this->pageImage->SetVisible(true);

    this->current_page = index;
    if (this->target_size <= 0) {
        this->ApplyViewMode();
    }
    else {
        this->ApplyCurrentMode();
    }
    this->SetPageIndicatorText(lang::Get("manga_viewer.page_indicator", {{"current", std::to_string(index + 1)}, {"total", std::to_string(this->page_count)}}));

    if (!this->progress_tracking_suspended) {
        manga::SaveProgress(this->manga_path, this->current_page, this->page_count);
    }
}

void MangaViewerLayout::SetPageIndicatorText(const std::string &text) {
    this->pageIndicator->SetText(text);

    const auto padding = MangaViewerLayout::PageIndicatorPadding;
    this->pageIndicatorBg->SetX(this->pageIndicator->GetX() - padding);
    this->pageIndicatorBg->SetY(this->pageIndicator->GetY() - padding);
    this->pageIndicatorBg->SetWidth(this->pageIndicator->GetWidth() + (padding * 2));
    this->pageIndicatorBg->SetHeight(this->pageIndicator->GetHeight() + (padding * 2));
}

std::string MangaViewerLayout::GetOrientationLabel() const {
    return (this->orientation == ReadingOrientation::Vertical) ? lang::Get("common.orientation_vertical") : lang::Get("common.orientation_horizontal");
}

void MangaViewerLayout::ToggleOrientation() {
    this->orientation = (this->orientation == ReadingOrientation::Vertical) ? ReadingOrientation::Horizontal : ReadingOrientation::Vertical;
    settings::SetReadingOrientation(this->orientation);
    this->sideMenu->RefreshLabels();

    if (this->cascade_mode) {
        // Every loaded page's fit-to-width height depends on the logical
        // screen width, which just changed, so there's no cheap way to
        // reposition in place: rebuild the whole cascade from scratch.
        this->ResetCascadeMode();
    }
    else {
        // Screen space swaps axes, so re-run the current fit mode against the
        // new logical dimensions and reset scroll, exactly like a resize.
        this->ApplyViewMode();
    }
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
    this->PositionImage(this->pageImage, logical_x, logical_y, this->image_width, this->image_height);
}

void MangaViewerLayout::PositionImage(const pu::ui::elm::Image::Ref &image, const s32 logical_x, const s32 logical_y, const s32 width, const s32 height) const {
    if (image == nullptr) {
        return;
    }

    if (this->orientation == ReadingOrientation::Vertical) {
        // image is drawn rotated clockwise on the real (never-rotated)
        // screen; read it by turning the console counter-clockwise, like a
        // book. SDL first stretches the whole texture into the rect we give
        // it, THEN rotates that rect about its own center — so the rect must
        // keep width/height as-is (already the correct aspect ratio) or the
        // stretch step distorts the page before it's ever rotated. The
        // swapped on-screen footprint falls out of rotating a non-square
        // rect; it needs no manual width/height swap here.
        const auto real_screen_w = static_cast<s32>(pu::ui::render::ScreenWidth);
        const auto logical_center_x = logical_x + (width / 2);
        const auto logical_center_y = logical_y + (height / 2);
        const auto real_center_x = real_screen_w - logical_center_y;
        const auto real_center_y = logical_center_x;

        image->SetWidth(width);
        image->SetHeight(height);
        image->SetX(real_center_x - (width / 2));
        image->SetY(real_center_y - (height / 2));
        image->SetRotationAngle(MangaViewerLayout::PortraitRotationAngle);
    }
    else {
        image->SetWidth(width);
        image->SetHeight(height);
        image->SetX(logical_x);
        image->SetY(logical_y);
        image->SetRotationAngle(0.0f);
    }
}

std::string MangaViewerLayout::GetCascadeModeLabel() const {
    return this->cascade_mode ? lang::Get("common.cascade_on") : lang::Get("common.cascade_off");
}

void MangaViewerLayout::SetCascadeMode(const bool enabled) {
    if (this->cascade_mode == enabled) {
        return;
    }
    settings::SetCascadeMode(enabled);

    if (enabled) {
        this->EnterCascadeMode();
    }
    else {
        this->LeaveCascadeMode();
    }
}

void MangaViewerLayout::EnterCascadeMode() {
    this->cascade_mode = true;
    this->pageImage->SetVisible(false);

    // Any previous prefetcher's worker thread must be fully stopped before
    // a new one starts (both would otherwise read this->source at once,
    // which isn't safe for every source implementation), so this always
    // tears down and replaces it, never reuses one across cascade sessions.
    this->cascade_prefetcher.reset();
    this->cascade_prefetcher = std::make_unique<CascadePagePrefetcher>(this->source.get());
    // Anything queued/cached against the old prefetcher is meaningless
    // against this new one, which knows nothing about those requests.
    this->cascade_awaiting_texture.clear();
    this->cascade_pending_textures.clear();
    this->PrefetchCascadePagesAhead();

    // Jumping straight to current_page (single-page mode, or a saved resume
    // position) can mean loading far more than a screen's worth of pages
    // sequentially, and LoadCascadePage fully decodes each one just to
    // measure it — doing that all at once, synchronously, would freeze the
    // app for as long as it takes to decode every page up to current_page.
    // AdvanceCascadeCatchup instead spreads that work over several frames.
    if (this->cascade_loaded_count > this->current_page) {
        this->SetCascadeScroll(this->cascade_offsets.at(this->current_page));
        return;
    }

    // Hides every cascade page behind a plain black screen until catch-up
    // finishes and lays them out correctly: LoadCascadePage below makes each
    // newly decoded page visible immediately, at whatever stale position its
    // Image last had, so without this a wrong page would flash on screen for
    // however many frames catch-up takes.
    this->cascadeLoadingOverlay->SetVisible(true);
    this->cascade_catching_up = true;
    this->SetPageIndicatorText(lang::Get("common.loading"));
}

void MangaViewerLayout::AdvanceCascadeCatchup() {
    if (!this->cascade_mode || !this->cascade_catching_up) {
        this->cascade_catching_up = false;
        this->cascadeLoadingOverlay->SetVisible(false);
        return;
    }

    for (s32 i = 0; (i < MangaViewerLayout::CascadeCatchupPagesPerFrame) && (this->cascade_loaded_count <= this->current_page) && (this->cascade_loaded_count < static_cast<u32>(this->page_count)); i++) {
        this->LoadCascadePage(this->cascade_loaded_count);
    }

    if (this->cascade_loaded_count > this->current_page) {
        this->cascade_catching_up = false;
        this->SetCascadeScroll(this->cascade_offsets.at(this->current_page));
        this->cascadeLoadingOverlay->SetVisible(false);
    }
}

void MangaViewerLayout::LeaveCascadeMode() {
    this->cascade_mode = false;
    // Stops the background worker (joining it) before LoadPage below reads
    // from this->source on the main thread: the two must never overlap.
    this->cascade_prefetcher.reset();
    this->cascade_awaiting_texture.clear();
    this->cascade_pending_textures.clear();

    for (auto &image : this->cascade_images) {
        image->SetVisible(false);
        // Frees the decoded texture; heights/offsets stay cached so
        // re-entering cascade mode later doesn't need to re-measure.
        image->SetImage(nullptr);
    }

    this->pageImage->SetVisible(true);
    this->LoadPage(this->current_page);
}

void MangaViewerLayout::ResetCascadeMode() {
    for (auto &image : this->cascade_images) {
        image->SetVisible(false);
        image->SetImage(nullptr);
    }
    this->cascade_heights.assign(this->page_count, 0);
    this->cascade_offsets.assign(this->page_count, 0);
    this->cascade_loaded_count = 0;
    this->cascade_total_height = 0;
    // The fit-to-width basis just changed along with the logical width, so
    // there's no ratio to preserve: reset to unzoomed instead of carrying
    // over a zoom level computed against the old orientation.
    this->cascade_zoom_width = this->GetLogicalScreenWidth();
    this->cascade_scroll_x = 0;
    this->UpdateCascadeHorizontalBounds();

    this->EnterCascadeMode();
}

void MangaViewerLayout::LoadCascadePage(const u32 index) {
    if (index >= this->page_count) {
        return;
    }

    // Blocks only if cascade_prefetcher hasn't finished decoding this page
    // yet, and TakeCascadeTexture hasn't already converted it to a texture
    // ahead of time — normally one of those already happened, having been
    // requested (by an earlier call's PrefetchCascadePagesAhead, below)
    // well before this point.
    s32 height = 0;
    auto tex_handle = this->TakeCascadeTexture(index);
    if (tex_handle != nullptr) {
        const auto tex_w = pu::ui::render::GetTextureWidth(tex_handle->Get());
        const auto tex_h = pu::ui::render::GetTextureHeight(tex_handle->Get());
        height = static_cast<s32>((static_cast<double>(tex_h) * this->cascade_zoom_width) / tex_w);

        this->cascade_images.at(index)->SetImage(tex_handle);
        this->cascade_images.at(index)->SetVisible(true);
    }
    // A broken/unreadable page just contributes zero height rather than
    // getting stuck retrying it forever.

    this->cascade_offsets.at(index) = this->cascade_total_height;
    this->cascade_heights.at(index) = height;
    this->cascade_total_height += height;
    this->cascade_loaded_count = index + 1;

    this->PrefetchCascadePagesAhead();
}

void MangaViewerLayout::PrefetchCascadePagesAhead() {
    const auto target = std::min(static_cast<u32>(this->page_count), this->cascade_loaded_count + MangaViewerLayout::CascadePrefetchAheadCount);
    for (auto i = this->cascade_loaded_count; i < target; i++) {
        this->RequestCascadeDecode(i);
    }
}

void MangaViewerLayout::ReloadCascadePageTexture(const u32 index) {
    auto tex_handle = this->TakeCascadeTexture(index);
    if (tex_handle == nullptr) {
        return;
    }

    this->cascade_images.at(index)->SetImage(tex_handle);
    // LeaveCascadeMode hides every cascade image on the way out; a page
    // that's merely being re-textured here (as opposed to loaded for the
    // first time via LoadCascadePage) needs its visibility restored too.
    this->cascade_images.at(index)->SetVisible(true);
}

void MangaViewerLayout::RequestCascadeDecode(const u32 index) {
    // Already converted and waiting in cascade_pending_textures: the
    // prefetcher itself no longer knows about it (TryTakeDecoded removed it
    // from `known` when claiming the surface), so without this check
    // RequestAhead below would think it's a fresh request and decode it a
    // second time for nothing.
    if (this->cascade_pending_textures.find(index) != this->cascade_pending_textures.end()) {
        return;
    }

    this->cascade_prefetcher->RequestAhead(index);
    this->cascade_awaiting_texture.insert(index);
}

pu::sdl2::TextureHandle::Ref MangaViewerLayout::TakeCascadeTexture(const u32 index) {
    auto pending = this->cascade_pending_textures.find(index);
    if (pending != this->cascade_pending_textures.end()) {
        auto tex_handle = pending->second;
        this->cascade_pending_textures.erase(pending);
        return tex_handle;
    }

    // Not preloaded yet (e.g. requested only just now, or scrolling faster
    // than AdvanceCascadeTexturePreload's one-per-frame pace): fall back to
    // blocking on the prefetcher and converting it right here, exactly like
    // before this cache existed.
    this->cascade_awaiting_texture.erase(index);
    auto surface = this->cascade_prefetcher->TakeDecoded(index);
    if (surface == nullptr) {
        return nullptr;
    }

    auto tex = pu::ui::render::ConvertToTexture(surface);
    if (tex == nullptr) {
        return nullptr;
    }
    return pu::sdl2::TextureHandle::New(tex);
}

void MangaViewerLayout::AdvanceCascadeTexturePreload() {
    if (!this->cascade_mode || (this->cascade_prefetcher == nullptr) || this->cascade_awaiting_texture.empty()) {
        return;
    }

    for (auto it = this->cascade_awaiting_texture.begin(); it != this->cascade_awaiting_texture.end(); ++it) {
        const auto index = *it;
        pu::sdl2::Surface surface = nullptr;
        if (!this->cascade_prefetcher->TryTakeDecoded(index, surface)) {
            continue;
        }

        pu::sdl2::TextureHandle::Ref tex_handle = nullptr;
        if (surface != nullptr) {
            auto tex = pu::ui::render::ConvertToTexture(surface);
            if (tex != nullptr) {
                tex_handle = pu::sdl2::TextureHandle::New(tex);
            }
        }
        this->cascade_pending_textures.emplace(index, tex_handle);
        this->cascade_awaiting_texture.erase(it);
        // One GPU upload per frame is the whole point: doing every ready
        // page at once would just move the stutter here instead of
        // removing it.
        break;
    }
}

void MangaViewerLayout::SetCascadeScroll(const s32 y) {
    auto target_y = y;
    if (target_y < 0) {
        target_y = 0;
    }

    const auto logical_h = this->GetLogicalScreenHeight();
    const auto load_ahead = logical_h * MangaViewerLayout::CascadeLoadAheadScreens;
    while ((this->cascade_loaded_count < static_cast<u32>(this->page_count)) && ((this->cascade_total_height - target_y) < load_ahead)) {
        this->LoadCascadePage(this->cascade_loaded_count);
    }

    const auto max_y = this->cascade_total_height - logical_h;
    if (target_y > max_y) {
        target_y = (max_y < 0) ? 0 : max_y;
    }

    this->scroll_y = target_y;
    // Texture reloads first: Image::SetImage re-derives width/height from
    // the raw texture's pixel size, clobbering whatever fit-to-width size
    // PositionImage last set. Laying out after, unconditionally, corrects
    // that for every loaded page regardless of what changed above.
    this->UpdateCascadeTextures();
    this->UpdateCascadeLayout();
    this->UpdateCurrentPageFromCascadeScroll();
}

void MangaViewerLayout::SetCascadeScrollX(const s32 x) {
    auto clamped_x = x;
    if (clamped_x < 0) {
        clamped_x = 0;
    }
    else if (clamped_x > this->cascade_max_scroll_x) {
        clamped_x = this->cascade_max_scroll_x;
    }

    this->cascade_scroll_x = clamped_x;
    this->UpdateCascadeLayout();
}

void MangaViewerLayout::AdjustCascadeZoom(const s32 delta) {
    const auto logical_w = this->GetLogicalScreenWidth();
    const auto min_width = static_cast<s32>(logical_w * MangaViewerLayout::MinZoomFraction);
    const auto max_width = static_cast<s32>(logical_w * MangaViewerLayout::MaxZoomFraction);

    auto new_width = this->cascade_zoom_width + delta;
    if (new_width < min_width) {
        new_width = min_width;
    }
    else if (new_width > max_width) {
        new_width = max_width;
    }

    if (new_width == this->cascade_zoom_width) {
        return;
    }

    // Every already-measured page was fit to the OLD width preserving its
    // own aspect ratio, so rescaling by new/old keeps that ratio correct
    // without re-decoding anything. Pages not yet loaded simply get
    // measured at the new width whenever LoadCascadePage reaches them.
    //
    // Offsets are rebuilt as a running sum of the rescaled heights rather
    // than rescaled directly: rescaling both independently rounds each one
    // separately, and repeated zooming compounds that rounding drift into
    // visible gaps between pages. A running sum can never drift from the
    // heights it was built from.
    const auto scale = static_cast<double>(new_width) / static_cast<double>(this->cascade_zoom_width);
    s32 recomputed_total = 0;
    for (u32 i = 0; i < this->cascade_loaded_count; i++) {
        this->cascade_heights.at(i) = static_cast<s32>(this->cascade_heights.at(i) * scale);
        this->cascade_offsets.at(i) = recomputed_total;
        recomputed_total += this->cascade_heights.at(i);
    }
    this->cascade_total_height = recomputed_total;
    const auto new_scroll_y = static_cast<s32>(this->scroll_y * scale);

    this->cascade_zoom_width = new_width;
    this->UpdateCascadeHorizontalBounds();
    this->SetCascadeScroll(new_scroll_y);
}

void MangaViewerLayout::UpdateCascadeHorizontalBounds() {
    const auto logical_w = this->GetLogicalScreenWidth();
    this->cascade_max_scroll_x = this->cascade_zoom_width - logical_w;
    if (this->cascade_max_scroll_x < 0) {
        this->cascade_center_offset_x = (logical_w - this->cascade_zoom_width) / 2;
        this->cascade_max_scroll_x = 0;
    }
    else {
        this->cascade_center_offset_x = 0;
    }

    if (this->cascade_scroll_x > this->cascade_max_scroll_x) {
        this->cascade_scroll_x = this->cascade_max_scroll_x;
    }
}

void MangaViewerLayout::UpdateCascadeLayout() {
    const auto logical_x = this->cascade_center_offset_x - this->cascade_scroll_x;
    for (u32 i = 0; i < this->cascade_loaded_count; i++) {
        this->PositionImage(this->cascade_images.at(i), logical_x, this->cascade_offsets.at(i) - this->scroll_y, this->cascade_zoom_width, this->cascade_heights.at(i));
    }
}

void MangaViewerLayout::UpdateCascadeTextures() {
    const auto logical_h = this->GetLogicalScreenHeight();
    const auto keep_above = this->scroll_y - (logical_h * MangaViewerLayout::CascadeUnloadAboveScreens);
    const auto keep_below = this->scroll_y + (logical_h * MangaViewerLayout::CascadeLoadAheadScreens);
    // A screen further out than keep_above/keep_below: pages here aren't
    // due to reload yet, but requesting their decode now (instead of only
    // once they're actually within keep_above/keep_below) gives the
    // background thread a head start, the same way LoadCascadePage's own
    // forward prefetch does for pages reached for the very first time.
    const auto prefetch_above = keep_above - logical_h;
    const auto prefetch_below = keep_below + logical_h;

    for (u32 i = 0; i < this->cascade_loaded_count; i++) {
        auto &image = this->cascade_images.at(i);

        const auto page_top = this->cascade_offsets.at(i);
        const auto page_bottom = page_top + this->cascade_heights.at(i);
        const auto in_range = (page_bottom >= keep_above) && (page_top <= keep_below);

        if (in_range && !image->IsImageValid()) {
            this->ReloadCascadePageTexture(i);
        }
        else if (!in_range && image->IsImageValid()) {
            image->SetImage(nullptr);
        }
        else if (!in_range && !image->IsImageValid() && (page_bottom >= prefetch_above) && (page_top <= prefetch_below)) {
            this->RequestCascadeDecode(i);
        }
    }
}

void MangaViewerLayout::UpdateCurrentPageFromCascadeScroll() {
    const auto viewport_center = this->scroll_y + (this->GetLogicalScreenHeight() / 2);
    for (u32 i = 0; i < this->cascade_loaded_count; i++) {
        const auto top = this->cascade_offsets.at(i);
        const auto bottom = top + this->cascade_heights.at(i);
        if ((viewport_center >= top) && (viewport_center < bottom)) {
            this->current_page = i;
            break;
        }
    }
    this->SetPageIndicatorText(lang::Get("manga_viewer.page_indicator", {{"current", std::to_string(this->current_page + 1)}, {"total", std::to_string(this->page_count)}}));

    if (!this->progress_tracking_suspended) {
        manga::SaveProgress(this->manga_path, this->current_page, this->page_count);
    }
}
