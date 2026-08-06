#include <MangaViewerLayout.hpp>
#include <FsUtils.hpp>

MangaViewerLayout::MangaViewerLayout(const std::string &manga_path) : Layout::Layout(), manga_path(manga_path), page_files(fs::ListImageFiles(manga_path)), current_page(0), mode(ViewMode::Vertical), tex_width(0), tex_height(0), scroll_offset(0), max_scroll_offset(0), center_offset(0) {
    this->SetBackgroundColor(pu::ui::Color(0, 0, 0, 0xFF));

    this->pageIndicator = pu::ui::elm::TextBlock::New(1700, 20, "");
    this->pageIndicator->SetColor(pu::ui::Color(255, 255, 255, 0xFF));
    this->Add(this->pageIndicator);

    if (!this->page_files.empty()) {
        this->LoadPage(0);
    }
    else {
        this->pageIndicator->SetText("Sin imagenes");
    }

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

        if (this->mode == ViewMode::Horizontal) {
            if (keys_held & HidNpadButton_StickLLeft) {
                this->SetScrollOffset(this->scroll_offset - MangaViewerLayout::ScrollSpeed);
            }
            else if (keys_held & HidNpadButton_StickLRight) {
                this->SetScrollOffset(this->scroll_offset + MangaViewerLayout::ScrollSpeed);
            }
        }
        else {
            if (keys_held & HidNpadButton_StickLUp) {
                this->SetScrollOffset(this->scroll_offset - MangaViewerLayout::ScrollSpeed);
            }
            else if (keys_held & HidNpadButton_StickLDown) {
                this->SetScrollOffset(this->scroll_offset + MangaViewerLayout::ScrollSpeed);
            }
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
    this->ApplyViewMode();
    this->pageIndicator->SetText(std::to_string(index + 1) + " / " + std::to_string(this->page_files.size()));
}

void MangaViewerLayout::ApplyViewMode() {
    if ((this->pageImage == nullptr) || (this->tex_width <= 0) || (this->tex_height <= 0)) {
        return;
    }

    switch (this->mode) {
        case ViewMode::Vertical: {
            this->ApplyWidthMode(static_cast<s32>(pu::ui::render::ScreenWidth));
            break;
        }
        case ViewMode::Intermediate: {
            const auto full_width = static_cast<s32>(pu::ui::render::ScreenWidth);
            const auto fit_height_width = static_cast<s32>((static_cast<double>(this->tex_width) * pu::ui::render::ScreenHeight) / this->tex_height);
            this->ApplyWidthMode((full_width + fit_height_width) / 2);
            break;
        }
        case ViewMode::Horizontal: {
            this->ApplyHeightMode(static_cast<s32>(pu::ui::render::ScreenHeight));
            break;
        }
    }

    this->SetScrollOffset(0);
}

void MangaViewerLayout::ApplyWidthMode(const s32 width) {
    const auto height = static_cast<s32>((static_cast<double>(this->tex_height) * width) / this->tex_width);

    this->pageImage->SetWidth(width);
    this->pageImage->SetHeight(height);
    this->pageImage->SetX((static_cast<s32>(pu::ui::render::ScreenWidth) - width) / 2);

    this->max_scroll_offset = height - static_cast<s32>(pu::ui::render::ScreenHeight);
    if (this->max_scroll_offset < 0) {
        this->center_offset = (static_cast<s32>(pu::ui::render::ScreenHeight) - height) / 2;
        this->max_scroll_offset = 0;
    }
    else {
        this->center_offset = 0;
    }
}

void MangaViewerLayout::ApplyHeightMode(const s32 height) {
    const auto width = static_cast<s32>((static_cast<double>(this->tex_width) * height) / this->tex_height);

    this->pageImage->SetWidth(width);
    this->pageImage->SetHeight(height);
    this->pageImage->SetY((static_cast<s32>(pu::ui::render::ScreenHeight) - height) / 2);

    this->max_scroll_offset = width - static_cast<s32>(pu::ui::render::ScreenWidth);
    if (this->max_scroll_offset < 0) {
        this->center_offset = (static_cast<s32>(pu::ui::render::ScreenWidth) - width) / 2;
        this->max_scroll_offset = 0;
    }
    else {
        this->center_offset = 0;
    }
}

void MangaViewerLayout::SetScrollOffset(const s32 offset) {
    auto clamped = offset;
    if (clamped < 0) {
        clamped = 0;
    }
    else if (clamped > this->max_scroll_offset) {
        clamped = this->max_scroll_offset;
    }

    this->scroll_offset = clamped;
    if (this->pageImage == nullptr) {
        return;
    }

    if (this->mode == ViewMode::Horizontal) {
        this->pageImage->SetX(this->center_offset - this->scroll_offset);
    }
    else {
        this->pageImage->SetY(this->center_offset - this->scroll_offset);
    }
}
