#include <RoundedOutlineRectangle.hpp>

void RoundedOutlineRectangle::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
    for (s32 i = 0; i < this->thickness; i++) {
        const auto line_w = this->w - (i * 2);
        const auto line_h = this->h - (i * 2);
        if ((line_w <= 0) || (line_h <= 0)) {
            break;
        }

        auto line_radius = this->radius - i;
        if (line_radius < 0) {
            line_radius = 0;
        }

        drawer->RenderRoundedRectangle(this->outline_clr, x + i, y + i, line_w, line_h, line_radius);
    }
}
