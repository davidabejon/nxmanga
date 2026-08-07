#pragma once

#include <pu/Plutonium>
#include <RoundedRectangle.hpp>
#include <SideMenu.hpp>
#include <manga/MangaSource.hpp>
#include <Settings.hpp>
#include <functional>
#include <string>

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

        // Reading orientation, independent of ViewMode's fit-to-width/height
        // modes above. Horizontal is the console held normally, exactly as
        // it works today. Vertical rotates the page 90 degrees so it fills
        // the screen like an e-book, held with the console turned on its
        // side. Persisted via Settings, so it carries over between mangas.
        using ReadingOrientation = settings::ReadingOrientation;

        static constexpr s32 ScrollSpeed = 25;
        static constexpr s32 ZoomSpeed = 10;
        static constexpr double MinZoomFraction = 0.3;
        static constexpr double MaxZoomFraction = 3.0;
        static constexpr s32 PageIndicatorPadding = 10;
        static constexpr s32 PageIndicatorBorderRadius = 14;
        // Maximum finger movement, in pixels, still considered a tap rather
        // than the start of a drag.
        static constexpr s32 TapMoveTolerance = 12;
        // How much target_size changes per pixel of change in the distance
        // between the two pinching fingers.
        static constexpr double PinchZoomSensitivity = 1.0;
        // Clockwise rotation, in degrees, applied to the page image in
        // ReadingOrientation::Vertical.
        static constexpr float PortraitRotationAngle = 90.0f;

        void LoadPage(const u32 index);
        void ApplyViewMode();
        void ApplyCurrentMode();
        void ApplyWidthMode(const s32 width);
        void ApplyHeightMode(const s32 height);
        void ApplyDimensions(const s32 width, const s32 height);
        void AdjustZoom(const s32 delta);
        void SetScroll(const s32 x, const s32 y);
        void SetPageIndicatorText(const std::string &text);
        void ToggleOrientation();
        std::string GetOrientationLabel() const;
        // Positions/sizes/rotates pageImage on the real (never-rotated)
        // screen from the logical width/height/scroll/center-offset state
        // below, which are always expressed as if orientation were
        // Horizontal.
        void UpdateImageTransform();
        s32 GetLogicalScreenWidth() const;
        s32 GetLogicalScreenHeight() const;
        // Converts a real touch point into the logical coordinate space, so
        // drag and tap handling stay orientation-agnostic.
        s32 ToLogicalTouchX(const s32 real_x, const s32 real_y) const;
        s32 ToLogicalTouchY(const s32 real_x, const s32 real_y) const;

        manga::MangaSourcePtr source;
        size_t page_count;
        u32 current_page;
        ViewMode mode;
        ReadingOrientation orientation;
        s32 tex_width;
        s32 tex_height;
        s32 target_size;
        // Logical (orientation-agnostic) size of the currently displayed
        // image, i.e. as if orientation were Horizontal.
        s32 image_width;
        s32 image_height;
        s32 scroll_x;
        s32 scroll_y;
        s32 max_scroll_x;
        s32 max_scroll_y;
        s32 center_offset_x;
        s32 center_offset_y;
        bool touch_active;
        bool touch_moved;
        s32 touch_start_x;
        s32 touch_start_y;
        s32 touch_last_x;
        s32 touch_last_y;
        bool pinch_active;
        double pinch_last_distance;
        // True from the moment a second finger is seen until the whole
        // gesture fully releases. Lets the tail end of a pinch (one finger
        // lifted a frame before the other) avoid being read as a fresh tap.
        bool touch_had_multitouch;
        OnBack on_back;
        pu::ui::elm::Image::Ref pageImage;
        RoundedRectangle::Ref pageIndicatorBg;
        pu::ui::elm::TextBlock::Ref pageIndicator;
        SideMenu::Ref sideMenu;
        pu::ui::elm::MenuItem::Ref orientationItem;
};
