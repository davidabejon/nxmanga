#include <MangaListLayout.hpp>
#include <FsUtils.hpp>

MangaListLayout::MangaListLayout(const std::string &manga_root) : Layout::Layout(), manga_root(manga_root) {
    this->titleText = pu::ui::elm::TextBlock::New(75, 30, "nxmanga");
    this->titleText->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
    this->Add(this->titleText);

    this->menu = pu::ui::elm::Menu::New(75, 110, 1770, pu::ui::Color(230, 230, 230, 0xFF), pu::ui::Color(30, 100, 200, 0xFF), 110, 5);
    this->Add(this->menu);

    const auto manga_names = fs::ListDirectories(this->manga_root);
    for (const auto &name : manga_names) {
        auto item = pu::ui::elm::MenuItem::New(name);
        item->SetColor(pu::ui::Color(20, 20, 20, 0xFF));
        item->AddOnKey([this, name]() {
            if (this->on_selected) {
                this->on_selected(this->manga_root + "/" + name);
            }
        });
        this->menu->AddItem(item);
    }

    if (manga_names.empty()) {
        this->titleText->SetText("No se encontraron mangas en sdmc:/manga");
    }
}
