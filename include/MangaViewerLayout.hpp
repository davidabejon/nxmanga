#pragma once

#include <pu/Plutonium>
#include <functional>
#include <string>
#include <vector>

class MangaViewerLayout : public pu::ui::Layout {
    public:
        using OnBack = std::function<void()>;

        MangaViewerLayout(const std::string &manga_path);
        PU_SMART_CTOR(MangaViewerLayout)

        inline void SetOnBack(OnBack on_back) {
            this->on_back = on_back;
        }

    private:
        static constexpr s32 ScrollSpeed = 25;

        void LoadPage(const u32 index);
        void SetScrollOffset(const s32 offset);

        std::string manga_path;
        std::vector<std::string> page_files;
        u32 current_page;
        s32 scroll_offset;
        s32 max_scroll_offset;
        OnBack on_back;
        pu::ui::elm::Image::Ref pageImage;
        pu::ui::elm::TextBlock::Ref pageIndicator;
};
