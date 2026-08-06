#include <MangaViewerLayout.hpp>
#include <FsUtils.hpp>

MangaViewerLayout::MangaViewerLayout(const std::string &manga_path) : Layout::Layout(), manga_path(manga_path), page_files(fs::ListImageFiles(manga_path)), current_page(0), scroll_offset(0), max_scroll_offset(0) {
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
        else if (keys_down & HidNpadButton_B) {
            if (this->on_back) {
                this->on_back();
            }
        }

        if (keys_held & HidNpadButton_StickLUp) {
            this->SetScrollOffset(this->scroll_offset - MangaViewerLayout::ScrollSpeed);
        }
        else if (keys_held & HidNpadButton_StickLDown) {
            this->SetScrollOffset(this->scroll_offset + MangaViewerLayout::ScrollSpeed);
        }
    });
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
    if (this->pageImage != nullptr) {
        this->pageImage->SetY(-this->scroll_offset);
    }
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

    const auto tex_w = pu::ui::render::GetTextureWidth(tex);
    const auto tex_h = pu::ui::render::GetTextureHeight(tex);
    const auto scaled_height = static_cast<s32>((static_cast<double>(tex_h) * pu::ui::render::ScreenWidth) / tex_w);

    auto tex_handle = pu::sdl2::TextureHandle::New(tex);
    if (this->pageImage == nullptr) {
        this->pageImage = pu::ui::elm::Image::New(0, 0, tex_handle);
        this->Add(this->pageImage);
    }
    else {
        this->pageImage->SetImage(tex_handle);
    }
    this->pageImage->SetWidth(pu::ui::render::ScreenWidth);
    this->pageImage->SetHeight(scaled_height);

    this->max_scroll_offset = scaled_height - static_cast<s32>(pu::ui::render::ScreenHeight);
    if (this->max_scroll_offset < 0) {
        this->max_scroll_offset = 0;
    }

    this->current_page = index;
    this->SetScrollOffset(0);
    this->pageIndicator->SetText(std::to_string(index + 1) + " / " + std::to_string(this->page_files.size()));
}
