#pragma once

#include <pu/Plutonium>

class RoundedOutlineRectangle : public pu::ui::elm::Element {
    public:
        RoundedOutlineRectangle(const s32 x, const s32 y, const s32 width, const s32 height, const pu::ui::Color outline_clr, const s32 radius, const s32 thickness) : Element(), x(x), y(y), w(width), h(height), outline_clr(outline_clr), radius(radius), thickness(thickness) {}
        PU_SMART_CTOR(RoundedOutlineRectangle)

        inline s32 GetX() override {
            return this->x;
        }

        inline void SetX(const s32 x) {
            this->x = x;
        }

        inline s32 GetY() override {
            return this->y;
        }

        inline void SetY(const s32 y) {
            this->y = y;
        }

        inline s32 GetWidth() override {
            return this->w;
        }

        inline void SetWidth(const s32 width) {
            this->w = width;
        }

        inline s32 GetHeight() override {
            return this->h;
        }

        inline void SetHeight(const s32 height) {
            this->h = height;
        }

        inline pu::ui::Color GetOutlineColor() {
            return this->outline_clr;
        }

        inline void SetOutlineColor(const pu::ui::Color outline_clr) {
            this->outline_clr = outline_clr;
        }

        void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
        void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}

    private:
        s32 x;
        s32 y;
        s32 w;
        s32 h;
        pu::ui::Color outline_clr;
        s32 radius;
        s32 thickness;
};
