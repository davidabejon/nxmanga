#include <SideMenu.hpp>
#include <Lang.hpp>
#include <Settings.hpp>

SideMenu::SideMenu(pu::ui::Layout *owner) : owner(owner), open(false) {
    const auto screen_w = static_cast<s32>(pu::ui::render::ScreenWidth);
    const auto screen_h = static_cast<s32>(pu::ui::render::ScreenHeight);

    const auto panel_w = SideMenu::PanelWidth;
    const auto panel_h = screen_h - (SideMenu::PanelMargin * 2);
    const auto panel_x = screen_w - panel_w - SideMenu::PanelMargin;
    const auto panel_y = SideMenu::PanelMargin;
    const auto content_x = panel_x + SideMenu::PanelInset;

    this->bg = RoundedRectangle::New(panel_x, panel_y, panel_w, panel_h, SideMenu::PanelColor, SideMenu::PanelBorderRadius);
    this->bg->SetVisible(false);
    this->owner->Add(this->bg);

    this->titleText = pu::ui::elm::TextBlock::New(content_x, panel_y + 28, lang::Get("common.side_menu_title"));
    this->titleText->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
    this->titleText->SetColor(SideMenu::ItemTextColor);
    this->titleText->SetVisible(false);
    this->owner->Add(this->titleText);

    const auto divider_y = this->titleText->GetY() + this->titleText->GetHeight() + 20;
    this->divider = pu::ui::elm::Rectangle::New(content_x, divider_y, panel_w - (SideMenu::PanelInset * 2), 3, SideMenu::AccentColor);
    this->divider->SetVisible(false);
    this->owner->Add(this->divider);

    this->menu_y = divider_y + 24;
    this->menu_w = panel_w - (SideMenu::ItemsInset * 2);
    this->menu_item_x = panel_x + SideMenu::ItemsInset;

    this->menu = pu::ui::elm::Menu::New(this->menu_item_x, this->menu_y, this->menu_w, SideMenu::PanelColor, SideMenu::PanelColor, SideMenu::ItemHeight, 0);
    this->menu->SetVisible(false);
    this->menu->SetOnSelectionChanged([this]() {
        this->UpdateItemOutlines();
    });
    this->owner->Add(this->menu);

    this->footer = pu::ui::elm::TextBlock::New(content_x, panel_y + panel_h - 56, lang::Get("common.side_menu_footer"));
    this->footer->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->footer->SetColor(SideMenu::FooterTextColor);
    this->footer->SetVisible(false);
    this->owner->Add(this->footer);

    // Only worth showing if there's actually something to switch to.
    if (lang::GetAvailableLanguages().size() >= 2) {
        this->AddItem([this]() {
            return this->GetLanguageItemLabel();
        }, [this]() {
            this->CycleLanguage();
        });
    }

    this->builtinItemCount = this->menu->GetItems().size();
}

pu::ui::elm::MenuItem::Ref SideMenu::AddItem(LabelProvider get_label, ItemCallback on_selected) {
    auto item = pu::ui::elm::MenuItem::New(get_label());
    item->SetColor(SideMenu::ItemTextColor);
    // Menu distinguishes the A button from a touch tap release internally
    // (pu::ui::TouchPseudoKey), so the callback has to be registered for
    // both or a tap only ever focuses the item without activating it.
    item->AddOnKey(on_selected);
    item->AddOnKey(on_selected, pu::ui::TouchPseudoKey);
    this->menu->AddItem(item);
    this->menu->SetNumberOfItemsToShow(static_cast<s32>(this->menu->GetItems().size()));
    // Menu::OnRender only lazily (re)loads name textures when its cache is
    // completely empty, which is only true the very first time it's ever
    // rendered. Past that point (e.g. after ClearItems() partially refills
    // it with just the builtin items), it's on AddItem to keep the cache in
    // sync with the item list itself, or the render loop indexes into it
    // out of bounds for every item added afterwards.
    this->menu->ForceReloadItems();
    this->itemLabelProviders.push_back(get_label);

    const auto index = this->menu->GetItems().size() - 1;
    if (index < this->itemOutlines.size()) {
        // Reusing a pool slot left over from before the most recent
        // ClearItems(): same index means the same position, so it's already
        // correctly placed, just currently hidden.
        this->itemOutlines.at(index)->SetVisible(this->open);
    }
    else {
        const auto outline_y = this->menu_y + (static_cast<s32>(index) * SideMenu::ItemHeight) + SideMenu::OutlineMarginY;
        const auto outline_h = SideMenu::ItemHeight - (SideMenu::OutlineMarginY * 2);
        auto outline = RoundedOutlineRectangle::New(this->menu_item_x, outline_y, this->menu_w, outline_h, SideMenu::OutlineIdleColor, SideMenu::OutlineRadius, SideMenu::OutlineThickness);
        outline->SetVisible(this->open);
        this->itemOutlines.push_back(outline);
        this->owner->Add(outline);
    }

    return item;
}

void SideMenu::ClearItems() {
    // Menu::ClearItems() (unlike just resizing GetItems()'s vector) also
    // resets its internal selected/scroll indices, which would otherwise be
    // left stale and potentially out of bounds once fewer items are added
    // back than there were before. The builtin items (e.g. the language
    // picker) are saved first and added straight back, since Menu::ClearItems()
    // would otherwise wipe those too.
    auto &menu_items = this->menu->GetItems();
    const std::vector<pu::ui::elm::MenuItem::Ref> builtin_items(menu_items.begin(), menu_items.begin() + this->builtinItemCount);
    const std::vector<LabelProvider> builtin_label_providers(this->itemLabelProviders.begin(), this->itemLabelProviders.begin() + this->builtinItemCount);

    this->menu->ClearItems();
    for (auto item : builtin_items) {
        this->menu->AddItem(item);
    }
    this->itemLabelProviders = builtin_label_providers;

    // Outlines can't be removed from the owner Layout, only hidden; SetOpen
    // already only shows outlines up to the current item count, but hiding
    // them here too keeps their state consistent even if queried meanwhile.
    for (size_t i = this->builtinItemCount; i < this->itemOutlines.size(); i++) {
        this->itemOutlines.at(i)->SetVisible(false);
    }

    this->menu->SetNumberOfItemsToShow(static_cast<s32>(this->menu->GetItems().size()));
    this->menu->ForceReloadItems();
}

void SideMenu::RefreshLabels() {
    this->titleText->SetText(lang::Get("common.side_menu_title"));
    this->footer->SetText(lang::Get("common.side_menu_footer"));

    auto &menu_items = this->menu->GetItems();
    for (size_t i = 0; i < menu_items.size(); i++) {
        menu_items.at(i)->SetName(this->itemLabelProviders.at(i)());
    }
    this->menu->ForceReloadItems();
}

std::string SideMenu::GetLanguageItemLabel() const {
    return lang::Get("language_name");
}

void SideMenu::CycleLanguage() {
    const auto languages = lang::GetAvailableLanguages();
    if (languages.empty()) {
        return;
    }

    const auto current = lang::GetLanguage();
    size_t next_index = 0;
    for (size_t i = 0; i < languages.size(); i++) {
        if (languages.at(i) == current) {
            next_index = (i + 1) % languages.size();
            break;
        }
    }

    lang::SetLanguage(languages.at(next_index));
    settings::SetLanguage(languages.at(next_index));
    this->RefreshLabels();
}

void SideMenu::UpdateItemOutlines() {
    const auto selected = this->menu->GetSelectedIndex();
    for (size_t i = 0; i < this->itemOutlines.size(); i++) {
        const auto is_selected = (static_cast<s32>(i) == selected);
        this->itemOutlines.at(i)->SetOutlineColor(is_selected ? SideMenu::OutlineFocusColor : SideMenu::OutlineIdleColor);
    }
}

void SideMenu::SetOpen(const bool open) {
    this->open = open;
    this->bg->SetVisible(open);
    this->titleText->SetVisible(open);
    this->divider->SetVisible(open);
    this->menu->SetVisible(open);
    this->footer->SetVisible(open);

    // Only up to the current item count: the pool can hold more outlines
    // than that, left over (hidden) from before the last ClearItems().
    const auto item_count = this->menu->GetItems().size();
    for (size_t i = 0; i < this->itemOutlines.size(); i++) {
        this->itemOutlines.at(i)->SetVisible(open && (i < item_count));
    }

    if (open) {
        this->UpdateItemOutlines();
    }
}

bool SideMenu::HandleInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
    if (keys_down & HidNpadButton_X) {
        this->SetOpen(!this->open);
        return true;
    }

    if (this->open) {
        if (keys_down & HidNpadButton_B) {
            this->SetOpen(false);
        }
        return true;
    }

    return false;
}
