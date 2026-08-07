#pragma once

#include <pu/Plutonium>
#include <chrono>

// A minimal loading indicator: a gray ring with a blue arc, covering 2/3 of
// the circumference, rotating on top of it.
//
// The rotation is driven by elapsed wall-clock time rather than a per-render
// step counter: this element is typically shown while something else blocks
// each frame for a while (e.g. decoding a cover image), so render calls land
// irregularly. Real time keeps it visibly moving at a steady pace instead of
// looking frozen between renders.
class LoadingSpinner : public pu::ui::elm::Element {
    public:
        LoadingSpinner(const s32 x, const s32 y, const s32 radius);
        PU_SMART_CTOR(LoadingSpinner)

        inline s32 GetX() override {
            return this->x;
        }

        inline s32 GetY() override {
            return this->y;
        }

        inline s32 GetWidth() override {
            return this->radius * 2;
        }

        inline s32 GetHeight() override {
            return this->radius * 2;
        }

        void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
        void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}

    private:
        static constexpr s32 SweepDegrees = 240;
        static constexpr s64 MsPerFullTurn = 1000;
        static constexpr s32 RingThickness = 6;
        static constexpr pu::ui::Color TrackColor = pu::ui::Color(210, 210, 210, 0xFF);
        // Same blue as MangaGrid's focus outline, for a consistent accent color.
        static constexpr pu::ui::Color SweepColor = pu::ui::Color(30, 100, 200, 0xFF);

        s32 x;
        s32 y;
        s32 radius;
        std::chrono::steady_clock::time_point start_time;
};
