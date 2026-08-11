#include <MangaListLayout.hpp>
#include <FsUtils.hpp>
#include <manga/MangaSource.hpp>
#include <manga/ReadingProgress.hpp>
#include <Lang.hpp>

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
    this->titleText = pu::ui::elm::TextBlock::New(75, 30, lang::Get("app_title"));
    this->titleText->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
    this->Add(this->titleText);

    const auto screen_w = static_cast<s32>(pu::ui::render::ScreenWidth);
    const auto screen_h = static_cast<s32>(pu::ui::render::ScreenHeight);

    // Hints at the X-button menu without needing it already open. A
    // translucent backing rectangle keeps the text readable regardless of
    // what's rendered behind it (cover art, scrolled content, etc.). Built
    // before the grid below so the grid's height can leave exactly enough
    // room under it, instead of guessing a margin that happens to fit.
    constexpr s32 SettingsHintPadding = 10;
    constexpr s32 SettingsHintBottomGap = 10;
    constexpr s32 SettingsHintBorderRadius = 10;

    this->settingsHintBg = RoundedRectangle::New(0, 0, 0, 0, pu::ui::Color(60, 60, 60, 170), SettingsHintBorderRadius);
    this->Add(this->settingsHintBg);

    this->settingsHint = pu::ui::elm::TextBlock::New(75 + SettingsHintPadding, 0, lang::Get("manga_list.settings_hint"));
    this->settingsHint->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->settingsHint->SetColor(pu::ui::Color(255, 255, 255, 0xFF));
    this->settingsHint->SetY(screen_h - SettingsHintBottomGap - SettingsHintPadding - this->settingsHint->GetHeight());
    this->Add(this->settingsHint);

    this->settingsHintBg->SetX(this->settingsHint->GetX() - SettingsHintPadding);
    this->settingsHintBg->SetY(this->settingsHint->GetY() - SettingsHintPadding);
    this->settingsHintBg->SetWidth(this->settingsHint->GetWidth() + (SettingsHintPadding * 2));
    this->settingsHintBg->SetHeight(this->settingsHint->GetHeight() + (SettingsHintPadding * 2));

    constexpr s32 GridBottomGap = 8;
    const auto grid_height = this->settingsHintBg->GetY() - GridBottomGap - 110;
    this->grid = MangaGrid::New(75, 110, 1770, grid_height, MangaListLayout::GridColumns);
    this->grid->SetVisible(false);
    this->Add(this->grid);

    constexpr s32 SpinnerRadius = 60;

    this->spinner = LoadingSpinner::New((screen_w / 2) - SpinnerRadius, (screen_h / 2) - SpinnerRadius - 30, SpinnerRadius);
    this->Add(this->spinner);

    this->loadingText = pu::ui::elm::TextBlock::New(0, (screen_h / 2) + SpinnerRadius - 10, lang::Get("common.loading"));
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
        this->titleText->SetText(lang::Get("manga_list.no_mangas_found", {{"path", this->manga_root}}));
        this->spinner->SetVisible(false);
        this->loadingText->SetVisible(false);
    }
    else {
        this->AddRenderCallback([this]() {
            this->LoadNextPendingCover();
        });
    }

    this->sideMenu = SideMenu::New(this);

    this->sideMenu->AddItem([this]() {
        return this->GetOrientationLabel();
    }, [this]() {
        const auto orientation = (settings::GetReadingOrientation() == settings::ReadingOrientation::Vertical) ? settings::ReadingOrientation::Horizontal : settings::ReadingOrientation::Vertical;
        settings::SetReadingOrientation(orientation);
        this->sideMenu->RefreshLabels();
    });

    this->sideMenu->AddItem([this]() {
        return this->GetCascadeModeLabel();
    }, [this]() {
        settings::SetCascadeMode(!settings::GetCascadeMode());
        this->sideMenu->RefreshLabels();
    });

    this->sideMenu->AddItem([]() {
        return lang::Get("common.side_menu_close");
    }, [this]() {
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
    return (settings::GetReadingOrientation() == settings::ReadingOrientation::Vertical) ? lang::Get("common.orientation_vertical") : lang::Get("common.orientation_horizontal");
}

std::string MangaListLayout::GetCascadeModeLabel() const {
    return settings::GetCascadeMode() ? lang::Get("common.cascade_on") : lang::Get("common.cascade_off");
}

void MangaListLayout::LoadNextPendingCover() {
    if (this->pending_index >= this->pending_paths.size()) {
        return;
    }

    const auto &path = this->pending_paths.at(this->pending_index);
    const auto completed = manga::IsFullyRead(path);

    // Only a leaf manga/chapter has a single page counter to show progress
    // for; a series folder aggregates several chapters, each with its own.
    bool in_progress = false;
    manga::ReadingProgress progress;
    if (!completed && manga::IsLeafManga(path)) {
        progress = manga::GetProgress(path);
        in_progress = progress.page_count > 0;
    }

    this->grid->AddItem(this->pending_names.at(this->pending_index), LoadCoverThumbnail(path), completed, in_progress, progress.current_page, progress.page_count);
    this->pending_index++;

    if (this->pending_index >= this->pending_paths.size()) {
        this->spinner->SetVisible(false);
        this->loadingText->SetVisible(false);
        this->grid->SetVisible(true);
    }
}
