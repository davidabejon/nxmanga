#include <LoadingSpinner.hpp>

LoadingSpinner::LoadingSpinner(const s32 x, const s32 y, const s32 radius) : Element(), x(x), y(y), radius(radius), start_time(std::chrono::steady_clock::now()) {}

void LoadingSpinner::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
    const auto center_x = x + this->radius;
    const auto center_y = y + this->radius;

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->start_time).count();
    const auto start_angle = static_cast<s32>((elapsed_ms * 360 / LoadingSpinner::MsPerFullTurn) % 360);
    const auto end_angle = start_angle + LoadingSpinner::SweepDegrees;

    const auto renderer = pu::ui::render::GetMainRenderer();
    for (s32 i = 0; i < LoadingSpinner::RingThickness; i++) {
        const auto ring_radius = this->radius - i;
        drawer->RenderCircle(LoadingSpinner::TrackColor, center_x, center_y, ring_radius);
        arcRGBA(renderer, static_cast<Sint16>(center_x), static_cast<Sint16>(center_y), static_cast<Sint16>(ring_radius), static_cast<Sint16>(start_angle), static_cast<Sint16>(end_angle), LoadingSpinner::SweepColor.r, LoadingSpinner::SweepColor.g, LoadingSpinner::SweepColor.b, LoadingSpinner::SweepColor.a);
    }
}
