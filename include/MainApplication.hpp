#pragma once

#include <pu/Plutonium>

class MainApplication : public pu::ui::Application {
    public:
        using Application::Application;
        PU_SMART_CTOR(MainApplication)

        void OnLoad() override;

    private:
        static constexpr const char *MangaRootPath = "sdmc:/manga";

        void ShowMangaList();
};
