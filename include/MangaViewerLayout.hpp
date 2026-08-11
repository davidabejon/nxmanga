#pragma once

#include <pu/Plutonium>
#include <RoundedRectangle.hpp>
#include <SideMenu.hpp>
#include <manga/MangaSource.hpp>
#include <Settings.hpp>
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
        // How many logical screen heights of cascade pages to keep decoded
        // ahead of/behind the viewport. Bounds memory use to a handful of
        // pages regardless of chapter length.
        static constexpr s32 CascadeLoadAheadScreens = 2;
        static constexpr s32 CascadeUnloadAboveScreens = 1;
        // How many pages EnterCascadeMode's catch-up decodes per frame while
        // seeking to a deep saved page, so resuming far into a long chapter
        // doesn't decode hundreds of pages synchronously in one frame.
        static constexpr s32 CascadeCatchupPagesPerFrame = 4;

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
        // Positions/sizes/rotates an Image on the real (never-rotated)
        // screen from logical (orientation-agnostic) coordinates/size,
        // shared by both the single-page and cascade rendering paths.
        void PositionImage(const pu::ui::elm::Image::Ref &image, const s32 logical_x, const s32 logical_y, const s32 width, const s32 height) const;
        // Positions/sizes/rotates pageImage from the logical
        // width/height/scroll/center-offset state below.
        void UpdateImageTransform();
        s32 GetLogicalScreenWidth() const;
        s32 GetLogicalScreenHeight() const;
        // Converts a real touch point into the logical coordinate space, so
        // drag and tap handling stay orientation-agnostic.
        s32 ToLogicalTouchX(const s32 real_x, const s32 real_y) const;
        s32 ToLogicalTouchY(const s32 real_x, const s32 real_y) const;

        void SetCascadeMode(const bool enabled);
        std::string GetCascadeModeLabel() const;
        void EnterCascadeMode();
        void LeaveCascadeMode();
        // Loads a few more cascade pages towards current_page, called from a
        // render callback until it catches up, instead of EnterCascadeMode
        // decoding potentially hundreds of pages synchronously in one frame.
        void AdvanceCascadeCatchup();
        // Tears down and rebuilds all cascade state from scratch, needed
        // after an orientation change since every page's fit-to-width
        // height depends on the (now different) logical screen width.
        void ResetCascadeMode();
        void LoadCascadePage(const u32 index);
        void ReloadCascadePageTexture(const u32 index);
        void SetCascadeScroll(const s32 y);
        void SetCascadeScrollX(const s32 x);
        // Changes the width every cascade page is fit to and rescales every
        // already-measured page's cached height/offset by the same ratio,
        // so already-loaded pages stay correctly proportioned without
        // re-decoding them.
        void AdjustCascadeZoom(const s32 delta);
        // Recomputes cascade_max_scroll_x/cascade_center_offset_x for the
        // current cascade_zoom_width vs the logical screen width.
        void UpdateCascadeHorizontalBounds();
        void UpdateCascadeLayout();
        // Frees textures for pages that scrolled far out of view and
        // reloads ones that scrolled back into range, keeping memory use
        // bounded regardless of chapter length.
        void UpdateCascadeTextures();
        void UpdateCurrentPageFromCascadeScroll();

        std::string manga_path;
        // True once this manga was already fully read when it was opened, so
        // re-reading it this session (without necessarily reaching the last
        // page again) never overwrites its saved completed progress.
        bool progress_tracking_suspended;
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
        bool cascade_mode;
        // True while AdvanceCascadeCatchup still has pages to load before
        // reaching current_page; set by EnterCascadeMode, cleared once it
        // catches up (or if the user leaves cascade mode first).
        bool cascade_catching_up;
        // Parallel, page_count-sized vectors: null/0 until LoadCascadePage
        // reaches that index. Pages load contiguously from 0, so indices
        // below cascade_loaded_count always have a valid height/offset,
        // even if their texture has since been unloaded to save memory.
        std::vector<pu::ui::elm::Image::Ref> cascade_images;
        std::vector<s32> cascade_heights;
        std::vector<s32> cascade_offsets;
        u32 cascade_loaded_count;
        s32 cascade_total_height;
        // Width every cascade page is currently fit to; GetLogicalScreenWidth()
        // when unzoomed, wider/narrower once the user pinches/sticks zoom.
        s32 cascade_zoom_width;
        s32 cascade_scroll_x;
        s32 cascade_max_scroll_x;
        s32 cascade_center_offset_x;
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
};
