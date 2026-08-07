#include <MangaViewerLayout.hpp>
#include <FsUtils.hpp>

MangaViewerLayout::MangaViewerLayout(const std::string &manga_path) : Layout::Layout(), manga_path(manga_path), page_files(fs::ListImageFiles(manga_path)), current_page(0), mode(ViewMode::Vertical), tex_width(0), tex_height(0), target_size(0), scroll_x(0), scroll_y(0), max_scroll_x(0), max_scroll_y(0), center_offset_x(0), center_offset_y(0) {
    this->SetBackgroundColor(pu::ui::Color(0, 0, 0, 0xFF));

    this->pageIndicator = pu::ui::elm::TextBlock::New(1700, 20, "");
    this->pageIndicator->SetColor(pu::ui::Color(255, 255, 255, 0xFF));
    this->pageIndicatorBg = pu::ui::elm::Rectangle::New(0, 0, 0, 0, pu::ui::Color(0, 0, 0, 160), MangaViewerLayout::PageIndicatorBorderRadius);

    if (!this->page_files.empty()) {
        this->LoadPage(0);
    }
    else {
        this->SetPageIndicatorText("Sin imagenes");
    }

    this->Add(this->pageIndicatorBg);
    this->Add(this->pageIndicator);

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_R) {
            if ((this->current_page + 1) < this->page_files.size()) {
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
    if (index >= this->page_files.size()) {
        return;
    }

    const auto path = this->manga_path + "/" + this->page_files.at(index);
    auto tex = pu::ui::render::LoadImageFromFile(path);
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
    this->SetPageIndicatorText(std::to_string(index + 1) + " / " + std::to_string(this->page_files.size()));
}

void MangaViewerLayout::SetPageIndicatorText(const std::string &text) {
    this->pageIndicator->SetText(text);

    const auto padding = MangaViewerLayout::PageIndicatorPadding;
    this->pageIndicatorBg->SetX(this->pageIndicator->GetX() - padding);
    this->pageIndicatorBg->SetY(this->pageIndicator->GetY() - padding);
    this->pageIndicatorBg->SetWidth(this->pageIndicator->GetWidth() + (padding * 2));
    this->pageIndicatorBg->SetHeight(this->pageIndicator->GetHeight() + (padding * 2));
}

void MangaViewerLayout::ApplyViewMode() {
    if ((this->pageImage == nullptr) || (this->tex_width <= 0) || (this->tex_height <= 0)) {
        return;
    }

    switch (this->mode) {
        case ViewMode::Vertical: {
            this->target_size = static_cast<s32>(pu::ui::render::ScreenWidth);
            break;
        }
        case ViewMode::Intermediate: {
            const auto full_width = static_cast<s32>(pu::ui::render::ScreenWidth);
            const auto fit_height_width = static_cast<s32>((static_cast<double>(this->tex_width) * pu::ui::render::ScreenHeight) / this->tex_height);
            this->target_size = (full_width + fit_height_width) / 2;
            break;
        }
        case ViewMode::Horizontal: {
            this->target_size = static_cast<s32>(pu::ui::render::ScreenHeight);
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
    this->pageImage->SetWidth(width);
    this->pageImage->SetHeight(height);

    this->max_scroll_x = width - static_cast<s32>(pu::ui::render::ScreenWidth);
    if (this->max_scroll_x < 0) {
        this->center_offset_x = (static_cast<s32>(pu::ui::render::ScreenWidth) - width) / 2;
        this->max_scroll_x = 0;
    }
    else {
        this->center_offset_x = 0;
    }

    this->max_scroll_y = height - static_cast<s32>(pu::ui::render::ScreenHeight);
    if (this->max_scroll_y < 0) {
        this->center_offset_y = (static_cast<s32>(pu::ui::render::ScreenHeight) - height) / 2;
        this->max_scroll_y = 0;
    }
    else {
        this->center_offset_y = 0;
    }
}

void MangaViewerLayout::AdjustZoom(const s32 delta) {
    if ((this->pageImage == nullptr) || (this->tex_width <= 0) || (this->tex_height <= 0)) {
        return;
    }

    const auto is_height_based = (this->mode == ViewMode::Horizontal);
    const auto screen_size = is_height_based ? static_cast<s32>(pu::ui::render::ScreenHeight) : static_cast<s32>(pu::ui::render::ScreenWidth);

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
    if (this->pageImage == nullptr) {
        return;
    }

    this->pageImage->SetX(this->center_offset_x - this->scroll_x);
    this->pageImage->SetY(this->center_offset_y - this->scroll_y);
}
