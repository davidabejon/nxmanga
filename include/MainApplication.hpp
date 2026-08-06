#pragma once

#include <pu/Plutonium>

class MainLayout : public pu::ui::Layout {
    private:
        pu::ui::elm::TextBlock::Ref helloText;

    public:
        MainLayout();
        PU_SMART_CTOR(MainLayout)
};

class MainApplication : public pu::ui::Application {
    private:
        MainLayout::Ref layout;

    public:
        using Application::Application;
        PU_SMART_CTOR(MainApplication)

        void OnLoad() override;
};
