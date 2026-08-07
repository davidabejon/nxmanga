#include <MangaListLayout.hpp>
#include <FsUtils.hpp>
#include <manga/MangaSource.hpp>

namespace {

    pu::sdl2::TextureHandle::Ref LoadCoverThumbnail(const std::string &path) {
        const auto cover = manga::GetCoverImage(path);
        if (cover.empty()) {
            return nullptr;
        }

        auto tex = pu::ui::render::LoadImageFromBuffer(cover.data(), cover.size());
        if (tex == nullptr) {
            return nullptr;
        }

        return pu::sdl2::TextureHandle::New(tex);
    }

}

MangaListLayout::MangaListLayout(const std::string &manga_root) : Layout::Layout(), manga_root(manga_root) {
    this->titleText = pu::ui::elm::TextBlock::New(75, 30, "nxmanga");
    this->titleText->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
    this->Add(this->titleText);

    const auto grid_height = static_cast<s32>(pu::ui::render::ScreenHeight) - 110 - 40;
    this->grid = MangaGrid::New(75, 110, 1770, grid_height, MangaListLayout::GridColumns);
    this->Add(this->grid);

    const auto manga_names = manga::ListMangaEntries(this->manga_root);
    std::vector<std::string> full_paths;
    for (const auto &name : manga_names) {
        const auto full_path = this->manga_root + "/" + name;
        full_paths.push_back(full_path);

        auto display_name = name;
        if (!fs::IsDirectory(full_path)) {
            const auto dot_pos = display_name.find_last_of('.');
            if (dot_pos != std::string::npos) {
                display_name = display_name.substr(0, dot_pos);
            }
        }

        this->grid->AddItem(display_name, LoadCoverThumbnail(full_path));
    }

    this->grid->SetOnItemSelected([this, full_paths](const size_t index) {
        if (this->on_selected) {
            this->on_selected(full_paths.at(index));
        }
    });

    if (manga_names.empty()) {
        this->titleText->SetText("No se encontraron mangas en " + this->manga_root);
    }

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_B) {
            if (this->on_back) {
                this->on_back();
            }
        }
    });
}
