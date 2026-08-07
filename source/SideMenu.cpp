#include <SideMenu.hpp>

SideMenu::SideMenu(pu::ui::Layout *owner, const std::string &title) : owner(owner), open(false) {
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

    this->titleText = pu::ui::elm::TextBlock::New(content_x, panel_y + 28, title);
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

    this->footer = pu::ui::elm::TextBlock::New(content_x, panel_y + panel_h - 56, "A Seleccionar    B Cerrar");
    this->footer->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
    this->footer->SetColor(SideMenu::FooterTextColor);
    this->footer->SetVisible(false);
    this->owner->Add(this->footer);
}

pu::ui::elm::MenuItem::Ref SideMenu::AddItem(const std::string &name, ItemCallback on_selected) {
    auto item = pu::ui::elm::MenuItem::New(name);
    item->SetColor(SideMenu::ItemTextColor);
    // Menu distinguishes the A button from a touch tap release internally
    // (pu::ui::TouchPseudoKey), so the callback has to be registered for
    // both or a tap only ever focuses the item without activating it.
    item->AddOnKey(on_selected);
    item->AddOnKey(on_selected, pu::ui::TouchPseudoKey);
    this->menu->AddItem(item);
    this->menu->SetNumberOfItemsToShow(static_cast<s32>(this->menu->GetItems().size()));

    const auto index = this->menu->GetItems().size() - 1;
    const auto outline_y = this->menu_y + (static_cast<s32>(index) * SideMenu::ItemHeight) + SideMenu::OutlineMarginY;
    const auto outline_h = SideMenu::ItemHeight - (SideMenu::OutlineMarginY * 2);
    auto outline = RoundedOutlineRectangle::New(this->menu_item_x, outline_y, this->menu_w, outline_h, SideMenu::OutlineIdleColor, SideMenu::OutlineRadius, SideMenu::OutlineThickness);
    outline->SetVisible(this->open);
    this->itemOutlines.push_back(outline);
    this->owner->Add(outline);

    return item;
}

void SideMenu::SetItemName(pu::ui::elm::MenuItem::Ref &item, const std::string &name) {
    item->SetName(name);
    this->menu->ForceReloadItems();
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
    for (auto &outline : this->itemOutlines) {
        outline->SetVisible(open);
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
