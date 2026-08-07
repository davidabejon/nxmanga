#include <RoundedRectangle.hpp>

void RoundedRectangle::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
    drawer->RenderRoundedRectangleFill(this->clr, x, y, this->w, this->h, this->radius);
}
