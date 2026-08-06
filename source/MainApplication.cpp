#include <MainApplication.hpp>

MainLayout::MainLayout() : Layout::Layout() {
    this->helloText = pu::ui::elm::TextBlock::New(300, 300, "Hola Mundo!");
    this->Add(this->helloText);
}

void MainApplication::OnLoad() {
    this->layout = MainLayout::New();
    this->LoadLayout(this->layout);

    this->SetOnInput([&](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_Plus) {
            this->Close();
        }
    });
}
