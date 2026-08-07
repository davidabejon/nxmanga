#pragma once

#include <pu/Plutonium>
#include <MangaListLayout.hpp>
#include <MangaViewerLayout.hpp>

class MainApplication : public pu::ui::Application {
    public:
        using Application::Application;
        PU_SMART_CTOR(MainApplication)

        void OnLoad() override;

    private:
        static constexpr const char *MangaRootPath = "sdmc:/manga";

        // Shows the entries under path (mangas/chapters, or further series
        // folders). on_back is invoked when the user backs out of this level;
        // pass nullptr at the library root, where there is nowhere to go back to.
        void ShowMangaList(const std::string &path, MangaListLayout::OnBack on_back);
        void ShowMangaViewer(const std::string &path, MangaViewerLayout::OnBack on_back);
};
