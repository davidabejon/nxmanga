#include <MainApplication.hpp>
#include <MangaListLayout.hpp>
#include <MangaViewerLayout.hpp>

void MainApplication::OnLoad() {
    this->ShowMangaList();

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_Plus) {
            this->Close();
        }
    });
}

void MainApplication::ShowMangaList() {
    auto list_layout = MangaListLayout::New(MainApplication::MangaRootPath);
    list_layout->SetOnMangaSelected([this](const std::string &manga_path) {
        auto viewer_layout = MangaViewerLayout::New(manga_path);
        viewer_layout->SetOnBack([this]() {
            this->ShowMangaList();
        });
        this->LoadLayout(viewer_layout);
    });
    this->LoadLayout(list_layout);
}
