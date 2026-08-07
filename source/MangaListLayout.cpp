#include <MangaListLayout.hpp>
#include <FsUtils.hpp>
#include <manga/MangaSource.hpp>

MangaListLayout::MangaListLayout(const std::string &manga_root) : Layout::Layout(), manga_root(manga_root) {
    this->titleText = pu::ui::elm::TextBlock::New(75, 30, "nxmanga");
    this->titleText->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
    this->Add(this->titleText);

    this->menu = pu::ui::elm::Menu::New(75, 110, 1770, pu::ui::Color(230, 230, 230, 0xFF), pu::ui::Color(30, 100, 200, 0xFF), 110, 5);
    this->Add(this->menu);

    const auto manga_names = manga::ListMangaEntries(this->manga_root);
    for (const auto &name : manga_names) {
        const auto full_path = this->manga_root + "/" + name;

        auto display_name = name;
        if (!fs::IsDirectory(full_path)) {
            const auto dot_pos = display_name.find_last_of('.');
            if (dot_pos != std::string::npos) {
                display_name = display_name.substr(0, dot_pos);
            }
        }

        auto item = pu::ui::elm::MenuItem::New(display_name);
        item->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
        item->AddOnKey([this, full_path]() {
            if (this->on_selected) {
                this->on_selected(full_path);
            }
        });
        this->menu->AddItem(item);
    }

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
