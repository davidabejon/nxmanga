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
        enum class ViewMode {
            Vertical,
            Intermediate,
            Horizontal
        };

        static constexpr s32 ScrollSpeed = 25;
        static constexpr s32 ZoomSpeed = 10;
        static constexpr double MinZoomFraction = 0.3;
        static constexpr double MaxZoomFraction = 3.0;

        void LoadPage(const u32 index);
        void ApplyViewMode();
        void ApplyCurrentMode();
        void ApplyWidthMode(const s32 width);
        void ApplyHeightMode(const s32 height);
        void ApplyDimensions(const s32 width, const s32 height);
        void AdjustZoom(const s32 delta);
        void SetScroll(const s32 x, const s32 y);

        std::string manga_path;
        std::vector<std::string> page_files;
        u32 current_page;
        ViewMode mode;
        s32 tex_width;
        s32 tex_height;
        s32 target_size;
        s32 scroll_x;
        s32 scroll_y;
        s32 max_scroll_x;
        s32 max_scroll_y;
        s32 center_offset_x;
        s32 center_offset_y;
        OnBack on_back;
        pu::ui::elm::Image::Ref pageImage;
        pu::ui::elm::TextBlock::Ref pageIndicator;
};
