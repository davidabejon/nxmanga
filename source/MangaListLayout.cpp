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

MangaListLayout::MangaListLayout(const std::string &manga_root) : Layout::Layout(), manga_root(manga_root), pending_index(0) {
    this->titleText = pu::ui::elm::TextBlock::New(75, 30, "nxmanga");
    this->titleText->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
    this->Add(this->titleText);

    const auto grid_height = static_cast<s32>(pu::ui::render::ScreenHeight) - 110 - 40;
    this->grid = MangaGrid::New(75, 110, 1770, grid_height, MangaListLayout::GridColumns);
    this->grid->SetVisible(false);
    this->Add(this->grid);

    const auto screen_w = static_cast<s32>(pu::ui::render::ScreenWidth);
    const auto screen_h = static_cast<s32>(pu::ui::render::ScreenHeight);
    constexpr s32 SpinnerRadius = 60;

    this->spinner = LoadingSpinner::New((screen_w / 2) - SpinnerRadius, (screen_h / 2) - SpinnerRadius - 30, SpinnerRadius);
    this->Add(this->spinner);

    this->loadingText = pu::ui::elm::TextBlock::New(0, (screen_h / 2) + SpinnerRadius - 10, "Cargando...");
    this->loadingText->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
    this->loadingText->SetX((screen_w - this->loadingText->GetWidth()) / 2);
    this->Add(this->loadingText);

    for (const auto &name : manga::ListMangaEntries(this->manga_root)) {
        const auto full_path = this->manga_root + "/" + name;

        auto display_name = name;
        if (!fs::IsDirectory(full_path)) {
            const auto dot_pos = display_name.find_last_of('.');
            if (dot_pos != std::string::npos) {
                display_name = display_name.substr(0, dot_pos);
            }
        }

        this->pending_paths.push_back(full_path);
        this->pending_names.push_back(display_name);
    }

    this->grid->SetOnItemSelected([this](const size_t index) {
        if (this->on_selected) {
            this->on_selected(this->pending_paths.at(index));
        }
    });

    if (this->pending_paths.empty()) {
        this->titleText->SetText("No se encontraron mangas en " + this->manga_root);
        this->spinner->SetVisible(false);
        this->loadingText->SetVisible(false);
    }
    else {
        this->AddRenderCallback([this]() {
            this->LoadNextPendingCover();
        });
    }

    this->sideMenu = SideMenu::New(this, "Opciones");

    this->orientationItem = this->sideMenu->AddItem(this->GetOrientationLabel(), [this]() {
        const auto orientation = (settings::GetReadingOrientation() == settings::ReadingOrientation::Vertical) ? settings::ReadingOrientation::Horizontal : settings::ReadingOrientation::Vertical;
        settings::SetReadingOrientation(orientation);
        this->sideMenu->SetItemName(this->orientationItem, this->GetOrientationLabel());
    });

    this->sideMenu->AddItem("Cerrar menu", [this]() {
        this->sideMenu->SetOpen(false);
    });

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        const auto menu_consumed = this->sideMenu->HandleInput(keys_down, keys_up, keys_held, touch_pos);
        this->grid->SetInputEnabled(!menu_consumed);
        if (menu_consumed) {
            return;
        }

        if (keys_down & HidNpadButton_B) {
            if (this->on_back) {
                this->on_back();
            }
        }
    });
}

std::string MangaListLayout::GetOrientationLabel() const {
    return (settings::GetReadingOrientation() == settings::ReadingOrientation::Vertical) ? "Vista: Vertical" : "Vista: Horizontal";
}

void MangaListLayout::LoadNextPendingCover() {
    if (this->pending_index >= this->pending_paths.size()) {
        return;
    }

    this->grid->AddItem(this->pending_names.at(this->pending_index), LoadCoverThumbnail(this->pending_paths.at(this->pending_index)));
    this->pending_index++;

    if (this->pending_index >= this->pending_paths.size()) {
        this->spinner->SetVisible(false);
        this->loadingText->SetVisible(false);
        this->grid->SetVisible(true);
    }
}
