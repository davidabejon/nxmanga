#include <MainApplication.hpp>
#include <manga/MangaSource.hpp>

void MainApplication::OnLoad() {
    this->ShowMangaList(MainApplication::MangaRootPath, nullptr);

    this->SetOnInput([this](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_Plus) {
            this->Close();
        }
    });
}

void MainApplication::ShowMangaList(const std::string &path, MangaListLayout::OnBack on_back) {
    auto list_layout = MangaListLayout::New(path);
    list_layout->SetOnBack(on_back);

    auto go_back = [this, path, on_back]() {
        this->ShowMangaList(path, on_back);
    };

    list_layout->SetOnMangaSelected([this, go_back](const std::string &selected_path) {
        if (manga::IsLeafManga(selected_path)) {
            this->ShowMangaViewer(selected_path, go_back);
        }
        else {
            this->ShowMangaList(selected_path, go_back);
        }
    });

    this->LoadLayout(list_layout);
}

void MainApplication::ShowMangaViewer(const std::string &path, MangaViewerLayout::OnBack on_back) {
    auto viewer_layout = MangaViewerLayout::New(path);
    viewer_layout->SetOnBack(on_back);
    this->LoadLayout(viewer_layout);
}
