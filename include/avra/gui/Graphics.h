#pragma once

#include "avra/gui/Geometry.h"
#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <mutex>

#if defined(_WIN32)
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#endif

namespace avra::gui {

/**
 * @brief Studio & Hardware Faceplate Typography Font Families (8 built-in DirectWrite fonts).
 */
enum class FontFamily : int {
    StudioSans      = 0, // Segoe UI (Clean Modern Studio Sans)
    TechnicalMono   = 1, // Consolas (Precision Lab / Digital Rack Monospace)
    BroadcastDin    = 2, // Bahnschrift (German/British Console Engraved DIN)
    VintageConsole  = 3, // Trebuchet MS (Warm American Vintage Silkscreen)
    BoutiqueSerif   = 4, // Georgia (Classic Tube / Audiophile Serif)
    StencilBlock    = 5, // Franklin Gothic Medium / Wide Spaced Block (High-Legibility Industrial Stencil)
    GeometricClean  = 6, // Calibri (Modern Geometric Hardware Lettering)
    RetroTypewriter = 7  // Courier New (Retro Schematic / Typewriter Stencil)
};

inline const wchar_t* getFontFamilyWideName(FontFamily f) noexcept {
    switch (f) {
        case FontFamily::TechnicalMono:   return L"Consolas";
        case FontFamily::BroadcastDin:    return L"Bahnschrift";
        case FontFamily::VintageConsole:  return L"Trebuchet MS";
        case FontFamily::BoutiqueSerif:   return L"Georgia";
        case FontFamily::StencilBlock:    return L"Franklin Gothic Medium";
        case FontFamily::GeometricClean:  return L"Calibri";
        case FontFamily::RetroTypewriter: return L"Consolas";
        case FontFamily::StudioSans:
        default:                          return L"Segoe UI";
    }
}

/**
 * @brief Hardware-accelerated 2D graphics rendering context.
 * Wraps Direct2D on Windows for zero-CPU GPU-accelerated drawing with cached DirectWrite formats.
 */
class Graphics {
public:
#if defined(_WIN32)
    explicit Graphics(ID2D1RenderTarget* target, IDWriteFactory* dwrite, ID2D1SolidColorBrush* brush, Point<float> scale = {1.0f, 1.0f}) noexcept
        : target_(target), dwrite_(dwrite), brush_(brush), scale_(scale) {
        applyTransform();
    }
#else
    Graphics() noexcept = default;
#endif

    void clear(Color color) noexcept {
#if defined(_WIN32)
        if (target_) {
            target_->Clear(D2D1::ColorF(color.r, color.g, color.b, color.a));
        }
#endif
    }

    void fillRect(Rect<float> bounds, Color color) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_RECT_F r{ bounds.x, bounds.y, bounds.right(), bounds.bottom() };
        target_->FillRectangle(&r, brush_);
#endif
    }

    void drawRect(Rect<float> bounds, Color color, float strokeWidth = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_RECT_F r{ bounds.x, bounds.y, bounds.right(), bounds.bottom() };
        target_->DrawRectangle(&r, brush_, strokeWidth);
#endif
    }

    void fillRoundedRect(Rect<float> bounds, float radius, Color color) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_ROUNDED_RECT r{ { bounds.x, bounds.y, bounds.right(), bounds.bottom() }, radius, radius };
        target_->FillRoundedRectangle(&r, brush_);
#endif
    }

    void drawRoundedRect(Rect<float> bounds, float radius, Color color, float strokeWidth = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_ROUNDED_RECT r{ { bounds.x, bounds.y, bounds.right(), bounds.bottom() }, radius, radius };
        target_->DrawRoundedRectangle(&r, brush_, strokeWidth);
#endif
    }

    void fillLinearGradientRect(Rect<float> bounds, Color topColor, Color bottomColor, float radius = 0.0f) noexcept {
#if defined(_WIN32)
        if (!target_) return;
        D2D1_GRADIENT_STOP stops[2] = {
            { 0.0f, D2D1::ColorF(topColor.r, topColor.g, topColor.b, topColor.a) },
            { 1.0f, D2D1::ColorF(bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a) }
        };
        ID2D1GradientStopCollection* stopCollection = nullptr;
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 2, &stopCollection))) {
            ID2D1LinearGradientBrush* gradBrush = nullptr;
            if (SUCCEEDED(target_->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties({ bounds.x, bounds.y }, { bounds.x, bounds.bottom() }),
                stopCollection,
                &gradBrush))) {
                if (radius > 0.0f) {
                    D2D1_ROUNDED_RECT r{ { bounds.x, bounds.y, bounds.right(), bounds.bottom() }, radius, radius };
                    target_->FillRoundedRectangle(&r, gradBrush);
                } else {
                    D2D1_RECT_F r{ bounds.x, bounds.y, bounds.right(), bounds.bottom() };
                    target_->FillRectangle(&r, gradBrush);
                }
                gradBrush->Release();
            }
            stopCollection->Release();
        }
#endif
    }

    void fillRadialGradientCircle(Point<float> center, float radius, Color innerColor, Color outerColor, Point<float> originOffset = {0.0f, 0.0f}) noexcept {
#if defined(_WIN32)
        if (!target_) return;
        D2D1_GRADIENT_STOP stops[2] = {
            { 0.0f, D2D1::ColorF(innerColor.r, innerColor.g, innerColor.b, innerColor.a) },
            { 1.0f, D2D1::ColorF(outerColor.r, outerColor.g, outerColor.b, outerColor.a) }
        };
        ID2D1GradientStopCollection* stopCollection = nullptr;
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 2, &stopCollection))) {
            ID2D1RadialGradientBrush* radBrush = nullptr;
            if (SUCCEEDED(target_->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(
                    { center.x, center.y },
                    { originOffset.x, originOffset.y },
                    radius,
                    radius),
                stopCollection,
                &radBrush))) {
                D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
                target_->FillEllipse(&el, radBrush);
                radBrush->Release();
            }
            stopCollection->Release();
        }
#endif
    }

    void fillRadialGradientCircle3Stops(
        Point<float> center,
        float radius,
        Color c0,
        float midStop,
        Color c1,
        Color c2,
        Point<float> originOffset = {0.0f, 0.0f}) noexcept {
#if defined(_WIN32)
        if (!target_) return;
        D2D1_GRADIENT_STOP stops[3] = {
            { 0.0f,    D2D1::ColorF(c0.r, c0.g, c0.b, c0.a) },
            { midStop, D2D1::ColorF(c1.r, c1.g, c1.b, c1.a) },
            { 1.0f,    D2D1::ColorF(c2.r, c2.g, c2.b, c2.a) }
        };
        ID2D1GradientStopCollection* stopCollection = nullptr;
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 3, &stopCollection))) {
            ID2D1RadialGradientBrush* radBrush = nullptr;
            if (SUCCEEDED(target_->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(
                    { center.x, center.y },
                    { originOffset.x, originOffset.y },
                    radius,
                    radius),
                stopCollection,
                &radBrush))) {
                D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
                target_->FillEllipse(&el, radBrush);
                radBrush->Release();
            }
            stopCollection->Release();
        }
#endif
    }

    void fillLinearGradientCircle3Stops(
        Point<float> center,
        float radius,
        Point<float> startPt,
        Point<float> endPt,
        Color c0,
        float midStop,
        Color c1,
        Color c2) noexcept {
#if defined(_WIN32)
        if (!target_) return;
        D2D1_GRADIENT_STOP stops[3] = {
            { 0.0f,    D2D1::ColorF(c0.r, c0.g, c0.b, c0.a) },
            { midStop, D2D1::ColorF(c1.r, c1.g, c1.b, c1.a) },
            { 1.0f,    D2D1::ColorF(c2.r, c2.g, c2.b, c2.a) }
        };
        ID2D1GradientStopCollection* stopCollection = nullptr;
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 3, &stopCollection))) {
            ID2D1LinearGradientBrush* gradBrush = nullptr;
            if (SUCCEEDED(target_->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties({ startPt.x, startPt.y }, { endPt.x, endPt.y }),
                stopCollection,
                &gradBrush))) {
                D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
                target_->FillEllipse(&el, gradBrush);
                gradBrush->Release();
            }
            stopCollection->Release();
        }
#endif
    }

    void fillLinearGradientCircle6Stops(
        Point<float> center,
        float radius,
        Point<float> startPt,
        Point<float> endPt,
        Color c0,
        Color c1,
        Color c2,
        Color c3,
        Color c4,
        Color c5) noexcept {
#if defined(_WIN32)
        if (!target_) return;
        D2D1_GRADIENT_STOP stops[6] = {
            { 0.00f, D2D1::ColorF(c0.r, c0.g, c0.b, c0.a) },
            { 0.20f, D2D1::ColorF(c1.r, c1.g, c1.b, c1.a) },
            { 0.40f, D2D1::ColorF(c2.r, c2.g, c2.b, c2.a) },
            { 0.60f, D2D1::ColorF(c3.r, c3.g, c3.b, c3.a) },
            { 0.80f, D2D1::ColorF(c4.r, c4.g, c4.b, c4.a) },
            { 1.00f, D2D1::ColorF(c5.r, c5.g, c5.b, c5.a) }
        };
        ID2D1GradientStopCollection* stopCollection = nullptr;
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 6, &stopCollection))) {
            ID2D1LinearGradientBrush* gradBrush = nullptr;
            if (SUCCEEDED(target_->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties({ startPt.x, startPt.y }, { endPt.x, endPt.y }),
                stopCollection,
                &gradBrush))) {
                D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
                target_->FillEllipse(&el, gradBrush);
                gradBrush->Release();
            }
            stopCollection->Release();
        }
#endif
    }

    void drawLinearGradientCircle(
        Point<float> center,
        float radius,
        Point<float> startPt,
        Point<float> endPt,
        Color c0,
        Color c1,
        float strokeWidth = 1.5f) noexcept {
#if defined(_WIN32)
        if (!target_) return;
        D2D1_GRADIENT_STOP stops[2] = {
            { 0.0f, D2D1::ColorF(c0.r, c0.g, c0.b, c0.a) },
            { 1.0f, D2D1::ColorF(c1.r, c1.g, c1.b, c1.a) }
        };
        ID2D1GradientStopCollection* stopCollection = nullptr;
        if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 2, &stopCollection))) {
            ID2D1LinearGradientBrush* gradBrush = nullptr;
            if (SUCCEEDED(target_->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties({ startPt.x, startPt.y }, { endPt.x, endPt.y }),
                stopCollection,
                &gradBrush))) {
                D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
                target_->DrawEllipse(&el, gradBrush, strokeWidth);
                gradBrush->Release();
            }
            stopCollection->Release();
        }
#endif
    }

    void fillTaperedFingerRidgeSolid(
        Point<float> center,
        float yNose,
        float yTail,
        float noseRadius,
        float tailRadius,
        Color fillCol) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        ID2D1Factory* factory = nullptr;
        target_->GetFactory(&factory);
        if (!factory) return;

        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(factory->CreatePathGeometry(&path))) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink))) {
                const float ny = center.y + yNose + noseRadius;
                const float ty = center.y + yTail - tailRadius;
                sink->BeginFigure({ center.x - tailRadius, ty }, D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine({ center.x - noseRadius, ny });
                sink->AddArc(D2D1::ArcSegment(
                    { center.x + noseRadius, ny },
                    { noseRadius, noseRadius },
                    0.0f,
                    D2D1_SWEEP_DIRECTION_CLOCKWISE,
                    D2D1_ARC_SIZE_SMALL));
                sink->AddLine({ center.x + tailRadius, ty });
                sink->AddArc(D2D1::ArcSegment(
                    { center.x - tailRadius, ty },
                    { tailRadius, tailRadius },
                    0.0f,
                    D2D1_SWEEP_DIRECTION_CLOCKWISE,
                    D2D1_ARC_SIZE_SMALL));
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                sink->Release();

                brush_->SetColor(D2D1::ColorF(fillCol.r, fillCol.g, fillCol.b, fillCol.a));
                target_->FillGeometry(path, brush_);
            }
            path->Release();
        }
        factory->Release();
#endif
    }

    void fillAndStrokeTaperedFingerRidge(
        Point<float> center,
        float yNose,
        float yTail,
        float noseRadius,
        float tailRadius,
        Color litCol,
        Color shadowCol,
        Color rimHighlightCol,
        float rimStrokeWidth = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        ID2D1Factory* factory = nullptr;
        target_->GetFactory(&factory);
        if (!factory) return;

        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(factory->CreatePathGeometry(&path))) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink))) {
                const float ny = center.y + yNose + noseRadius;
                const float ty = center.y + yTail - tailRadius;
                sink->BeginFigure({ center.x - tailRadius, ty }, D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine({ center.x - noseRadius, ny });
                sink->AddArc(D2D1::ArcSegment(
                    { center.x + noseRadius, ny },
                    { noseRadius, noseRadius },
                    0.0f,
                    D2D1_SWEEP_DIRECTION_CLOCKWISE,
                    D2D1_ARC_SIZE_SMALL));
                sink->AddLine({ center.x + tailRadius, ty });
                sink->AddArc(D2D1::ArcSegment(
                    { center.x - tailRadius, ty },
                    { tailRadius, tailRadius },
                    0.0f,
                    D2D1_SWEEP_DIRECTION_CLOCKWISE,
                    D2D1_ARC_SIZE_SMALL));
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                sink->Release();

                D2D1_GRADIENT_STOP stops[2] = {
                    { 0.0f, D2D1::ColorF(litCol.r, litCol.g, litCol.b, litCol.a) },
                    { 1.0f, D2D1::ColorF(shadowCol.r, shadowCol.g, shadowCol.b, shadowCol.a) }
                };
                ID2D1GradientStopCollection* stopCollection = nullptr;
                if (SUCCEEDED(target_->CreateGradientStopCollection(stops, 2, &stopCollection))) {
                    ID2D1LinearGradientBrush* gradBrush = nullptr;
                    if (SUCCEEDED(target_->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(
                            { center.x - tailRadius, center.y },
                            { center.x + tailRadius, center.y }),
                        stopCollection,
                        &gradBrush))) {
                        target_->FillGeometry(path, gradBrush);
                        gradBrush->Release();
                    }
                    stopCollection->Release();
                }

                brush_->SetColor(D2D1::ColorF(rimHighlightCol.r, rimHighlightCol.g, rimHighlightCol.b, rimHighlightCol.a));
                target_->DrawGeometry(path, brush_, rimStrokeWidth);
            }
            path->Release();
        }
        factory->Release();
#endif
    }

    void fillCircle(Point<float> center, float radius, Color color) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
        target_->FillEllipse(&el, brush_);
#endif
    }

    void drawCircle(Point<float> center, float radius, Color color, float strokeWidth = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_ELLIPSE el{ { center.x, center.y }, radius, radius };
        target_->DrawEllipse(&el, brush_, strokeWidth);
#endif
    }

    void fillEllipse(Point<float> center, float rx, float ry, Color color) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_ELLIPSE el{ { center.x, center.y }, rx, ry };
        target_->FillEllipse(&el, brush_);
#endif
    }

    void drawEllipse(Point<float> center, float rx, float ry, Color color, float strokeWidth = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        D2D1_ELLIPSE el{ { center.x, center.y }, rx, ry };
        target_->DrawEllipse(&el, brush_, strokeWidth);
#endif
    }

    void drawLine(Point<float> p1, Point<float> p2, Color color, float strokeWidth = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
        target_->DrawLine({ p1.x, p1.y }, { p2.x, p2.y }, brush_, strokeWidth);
#endif
    }

    void drawPolyline(std::span<const Point<float>> points, Color color, float strokeWidth = 1.5f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_ || points.size() < 2) return;
        ID2D1Factory* factory = nullptr;
        target_->GetFactory(&factory);
        if (!factory) return;

        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(factory->CreatePathGeometry(&path))) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink))) {
                sink->BeginFigure({ points[0].x, points[0].y }, D2D1_FIGURE_BEGIN_HOLLOW);
                for (size_t i = 1; i < points.size(); ++i) {
                    sink->AddLine({ points[i].x, points[i].y });
                }
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->Close();
                sink->Release();

                brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
                target_->DrawGeometry(path, brush_, strokeWidth);
            }
            path->Release();
        }
        factory->Release();
#endif
    }

    void fillPolygon(std::span<const Point<float>> points, Color color) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_ || points.size() < 3) return;
        ID2D1Factory* factory = nullptr;
        target_->GetFactory(&factory);
        if (!factory) return;

        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(factory->CreatePathGeometry(&path))) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink))) {
                sink->BeginFigure({ points[0].x, points[0].y }, D2D1_FIGURE_BEGIN_FILLED);
                for (size_t i = 1; i < points.size(); ++i) {
                    sink->AddLine({ points[i].x, points[i].y });
                }
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                sink->Release();

                brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
                target_->FillGeometry(path, brush_);
            }
            path->Release();
        }
        factory->Release();
#endif
    }

    /**
     * @brief Draw an arc (essential for modern rotary knob tracks and indicators).
     */
    void drawArc(Point<float> center, float radius, float startAngleRadians, float endAngleRadians, Color color, float strokeWidth = 2.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !brush_) return;
        const float sweep = endAngleRadians - startAngleRadians;
        if (std::abs(sweep) < 0.002f) return;

        ID2D1Factory* factory = nullptr;
        target_->GetFactory(&factory);
        if (!factory) return;

        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(factory->CreatePathGeometry(&path))) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink))) {
                const float startX = center.x + radius * std::sin(startAngleRadians);
                const float startY = center.y - radius * std::cos(startAngleRadians);
                const float endX = center.x + radius * std::sin(endAngleRadians);
                const float endY = center.y - radius * std::cos(endAngleRadians);

                sink->BeginFigure({ startX, startY }, D2D1_FIGURE_BEGIN_HOLLOW);

                const D2D1_ARC_SIZE arcSize = (std::abs(sweep) > kPi) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
                const D2D1_SWEEP_DIRECTION dir = (sweep >= 0) ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

                sink->AddArc(D2D1::ArcSegment(
                    { endX, endY },
                    { radius, radius },
                    0.0f,
                    dir,
                    arcSize
                ));

                sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->Close();
                sink->Release();

                brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
                target_->DrawGeometry(path, brush_, strokeWidth);
            }
            path->Release();
        }
        factory->Release();
#endif
    }

    void setScale(Point<float> scale) noexcept {
        scale_ = scale;
        applyTransform();
    }
    [[nodiscard]] Point<float> getScale() const noexcept { return scale_; }

    void pushOffset(Point<float> offset) noexcept {
        originStack_.push_back(origin_);
        origin_ = { origin_.x + offset.x, origin_.y + offset.y };
        applyTransform();
    }

    void popOffset() noexcept {
        if (!originStack_.empty()) {
            origin_ = originStack_.back();
            originStack_.pop_back();
            applyTransform();
        }
    }

    void pushRotation(float angleDegrees, Point<float> localCenter) noexcept {
        rotationStack_.push_back({ rotationDeg_, rotationCenter_ });
        rotationDeg_    = angleDegrees;
        rotationCenter_ = localCenter;
        applyTransform();
    }

    void popRotation() noexcept {
        if (!rotationStack_.empty()) {
            const auto prev = rotationStack_.back();
            rotationStack_.pop_back();
            rotationDeg_    = prev.deg;
            rotationCenter_ = prev.center;
        } else {
            rotationDeg_ = 0.0f;
        }
        applyTransform();
    }

    void pushClipRect(Rect<float> clipBounds) noexcept {
#if defined(_WIN32)
        if (target_) {
            D2D1_RECT_F r{ clipBounds.x, clipBounds.y, clipBounds.right(), clipBounds.bottom() };
            target_->PushAxisAlignedClip(&r, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }
#else
        (void)clipBounds;
#endif
    }

    void popClipRect() noexcept {
#if defined(_WIN32)
        if (target_) {
            target_->PopAxisAlignedClip();
        }
#endif
    }

    void setFontFamily(FontFamily family) noexcept { fontFamily_ = family; }
    [[nodiscard]] FontFamily getFontFamily() const noexcept { return fontFamily_; }

    void setFontScale(float scale) noexcept { fontScale_ = (std::max)(0.65f, (std::min)(scale, 1.65f)); }
    [[nodiscard]] float getFontScale() const noexcept { return fontScale_; }

    void setSkinTextColor(bool enabled, Color col = Color::fromHex(0xE8F1F2)) noexcept {
        useSkinTextColor_ = enabled;
        skinTextColor_    = col;
    }

    [[nodiscard]] Color resolveSilkscreenColor(Color defaultCol, float alphaScale = 1.0f) const noexcept {
        if (useSkinTextColor_) {
            return skinTextColor_.withAlpha(skinTextColor_.a * alphaScale);
        }
        return defaultCol;
    }

    void setSkinTextPlateColor(bool enabled, Color col = Color::fromHex(0x0E2A2C, 0.58f)) noexcept {
        useSkinTextPlateColor_ = enabled;
        skinTextPlateColor_    = col;
    }

    [[nodiscard]] bool hasSkinTextPlateColor() const noexcept { return useSkinTextPlateColor_; }

    [[nodiscard]] Color resolveTextPlateColor(Color defaultCol, float alphaOverride = -1.0f) const noexcept {
        if (useSkinTextPlateColor_) {
            if (skinTextPlateColor_.a <= 0.01f) {
                return Color(skinTextPlateColor_.r, skinTextPlateColor_.g, skinTextPlateColor_.b, 0.0f);
            }
            const float a = (alphaOverride >= 0.0f) ? (alphaOverride * (skinTextPlateColor_.a / 0.62f)) : skinTextPlateColor_.a;
            return skinTextPlateColor_.withAlpha((std::max)(0.0f, (std::min)(a, 0.92f)));
        }
        return (alphaOverride >= 0.0f) ? defaultCol.withAlpha(alphaOverride) : defaultCol;
    }

    void resetSkinTypography() noexcept {
        fontFamily_            = FontFamily::StudioSans;
        fontScale_             = 1.0f;
        useSkinTextColor_      = false;
        useSkinTextPlateColor_ = false;
    }

    void drawText(std::string_view text, Rect<float> bounds, Color color, float fontSize = 13.0f, bool isCentered = true, bool isBold = false) noexcept {
#if defined(_WIN32)
        if (!target_ || !dwrite_ || !brush_ || text.empty() || bounds.width <= 2.0f) return;

        const float scaledFontSize = fontSize * fontScale_;

        // Auto-fit font size when a line of text would exceed bounds.width so centered/leading text never clips
        size_t maxLineChars = 0;
        size_t curLineChars = 0;
        for (char c : text) {
            if (c == '\n') {
                maxLineChars = (std::max)(maxLineChars, curLineChars);
                curLineChars = 0;
            } else {
                ++curLineChars;
            }
        }
        maxLineChars = (std::max)(maxLineChars, curLineChars);

        float effectiveFontSize = scaledFontSize;
        if (maxLineChars > 2 && bounds.width > 14.0f) {
            const float avgCharW = scaledFontSize * (isBold ? 0.65f : 0.58f);
            const float estWidth = static_cast<float>(maxLineChars) * avgCharW;
            const float availW   = (std::max)(10.0f, bounds.width - 3.0f);
            if (estWidth > availW) {
                effectiveFontSize = (std::max)(5.4f, scaledFontSize * (availW / estWidth));
            }
        }

        IDWriteTextFormat* format = getOrCreateTextFormat(effectiveFontSize, isCentered, isBold, fontFamily_);
        if (format) {
            brush_->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));

            int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
            if (wideLen > 0) {
                std::wstring wideText(static_cast<size_t>(wideLen), 0);
                MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wideText.data(), wideLen);
                const float extraPadY = (std::max)(0.0f, (effectiveFontSize * 1.45f - bounds.height) * 0.5f);
                D2D1_RECT_F r{ bounds.x, bounds.y - extraPadY, bounds.right(), bounds.bottom() + extraPadY };
                target_->DrawText(wideText.c_str(), static_cast<UINT32>(wideText.size()), format, &r, brush_, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
#endif
    }

    /**
     * @brief Measures the rendered horizontal width of `text` using DirectWrite text layout metrics.
     */
    [[nodiscard]] float measureTextWidth(std::string_view text, float fontSize = 13.0f, bool isBold = false) const noexcept {
        if (text.empty()) return 0.0f;
        const float scaledFontSize = fontSize * fontScale_;
#if defined(_WIN32)
        uint64_t hash = 14695981039346656037ULL;
        for (unsigned char c : text) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        hash ^= (static_cast<uint64_t>(static_cast<uint32_t>(scaledFontSize * 10.0f + 0.5f)) << 16)
              | (static_cast<uint64_t>(fontFamily_) << 4)
              | (isBold ? 1ULL : 0ULL);
        hash *= 1099511628211ULL;

        static std::unordered_map<uint64_t, float> s_widthCache;
        auto it = s_widthCache.find(hash);
        if (it != s_widthCache.end()) {
            return it->second;
        }

        if (dwrite_) {
            IDWriteTextFormat* format = const_cast<Graphics*>(this)->getOrCreateTextFormat(scaledFontSize, false, isBold, fontFamily_);
            if (format) {
                int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
                if (wideLen > 0) {
                    std::wstring wideText(static_cast<size_t>(wideLen), 0);
                    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wideText.data(), wideLen);
                    IDWriteTextLayout* layout = nullptr;
                    HRESULT hr = dwrite_->CreateTextLayout(
                        wideText.c_str(),
                        static_cast<UINT32>(wideText.size()),
                        format,
                        4000.0f,
                        200.0f,
                        &layout);
                    if (SUCCEEDED(hr) && layout) {
                        DWRITE_TEXT_METRICS metrics{};
                        const HRESULT mHr = layout->GetMetrics(&metrics);
                        layout->Release();
                        if (SUCCEEDED(mHr)) {
                            if (s_widthCache.size() > 4096) s_widthCache.clear();
                            s_widthCache[hash] = metrics.widthIncludingTrailingWhitespace;
                            return metrics.widthIncludingTrailingWhitespace;
                        }
                    }
                }
            }
        }
#endif
        return static_cast<float>(text.size()) * scaledFontSize * (isBold ? 0.65f : 0.58f);
    }

    /**
     * @brief Measures both the rendered horizontal width (`x`) and vertical height (`y`) of `text`
     * using DirectWrite `IDWriteTextLayout::GetMetrics()`. Safe to call from UI event handlers without an active `Graphics` target.
     */
    [[nodiscard]] static Point<float> measureTextMetrics(
        std::string_view text,
        float fontSize = 13.0f,
        bool isBold = true,
        FontFamily family = FontFamily::StudioSans) noexcept {
        if (text.empty()) return { 0.0f, 0.0f };
#if defined(_WIN32)
        uint64_t hash = 14695981039346656037ULL;
        for (unsigned char c : text) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        hash ^= (static_cast<uint64_t>(static_cast<uint32_t>(fontSize * 10.0f + 0.5f)) << 16)
              | (static_cast<uint64_t>(family) << 4)
              | (isBold ? 1ULL : 0ULL);
        hash *= 1099511628211ULL;

        static std::unordered_map<uint64_t, Point<float>> s_metricsCache;
        auto it = s_metricsCache.find(hash);
        if (it != s_metricsCache.end()) {
            return it->second;
        }

        static IDWriteFactory* s_sharedDWrite = nullptr;
        if (!s_sharedDWrite) {
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&s_sharedDWrite));
        }
        if (s_sharedDWrite) {
            const DWRITE_FONT_WEIGHT weight = (family == FontFamily::StencilBlock)
                ? DWRITE_FONT_WEIGHT_BOLD
                : (isBold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);
            IDWriteTextFormat* format = nullptr;
            if (SUCCEEDED(s_sharedDWrite->CreateTextFormat(
                    getFontFamilyWideName(family),
                    nullptr,
                    weight,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    fontSize,
                    L"en-us",
                    &format)) && format) {
                format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                const int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
                if (wideLen > 0) {
                    std::wstring wideText(static_cast<size_t>(wideLen), 0);
                    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wideText.data(), wideLen);
                    IDWriteTextLayout* layout = nullptr;
                    const HRESULT hr = s_sharedDWrite->CreateTextLayout(
                        wideText.c_str(),
                        static_cast<UINT32>(wideText.size()),
                        format,
                        4000.0f,
                        400.0f,
                        &layout);
                    if (SUCCEEDED(hr) && layout) {
                        DWRITE_TEXT_METRICS metrics{};
                        const HRESULT mHr = layout->GetMetrics(&metrics);
                        layout->Release();
                        format->Release();
                        if (SUCCEEDED(mHr)) {
                            const Point<float> res{ metrics.widthIncludingTrailingWhitespace, metrics.height };
                            if (s_metricsCache.size() > 4096) s_metricsCache.clear();
                            s_metricsCache[hash] = res;
                            return res;
                        }
                    }
                }
                format->Release();
            }
        }
#endif
        return {
            static_cast<float>(text.size()) * fontSize * (isBold ? 0.65f : 0.58f),
            fontSize * 1.35f
        };
    }

    /**
     * @brief Releases all cached Direct2D bitmaps associated with `target` before `target` is destroyed.
     * Prevents ABA dangling pointer crashes when a subsequent plugin window allocates an ID2D1HwndRenderTarget
     * at the same address, and supports multiple simultaneous plugin windows without cache thrashing.
     */
#if defined(_WIN32)
    static void releaseRenderTargetCaches(ID2D1RenderTarget* target) noexcept {
        if (!target) return;
        std::lock_guard<std::mutex> lock(getBitmapCacheMutex());
        auto& fileMap = getFileBitmapCacheMap();
        if (auto it = fileMap.find(target); it != fileMap.end()) {
            for (auto& kv : it->second) {
                if (kv.second) kv.second->Release();
            }
            fileMap.erase(it);
        }
        auto& memMap = getMemBitmapCacheMap();
        if (auto it = memMap.find(target); it != memMap.end()) {
            for (auto& kv : it->second) {
                if (kv.second) kv.second->Release();
            }
            memMap.erase(it);
        }
        auto& procMap = getProcBitmapCacheMap();
        if (auto it = procMap.find(target); it != procMap.end()) {
            for (auto& kv : it->second) {
                if (kv.second) kv.second->Release();
            }
            procMap.erase(it);
        }
    }
#endif

    /**
     * @brief Loads and caches a custom image file (.png, .jpg, .jpeg, .bmp, .gif, .ico) via WIC
     * and draws it inside `bounds` (preserving aspect ratio) with optional opacity.
     */
    bool drawImageFromFile(std::string_view filePath, Rect<float> bounds, float opacity = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || filePath.empty() || bounds.width <= 1.0f || bounds.height <= 1.0f) return false;

        const std::string key(filePath);
        ID2D1Bitmap* bmp = nullptr;
        {
            std::lock_guard<std::mutex> lock(getBitmapCacheMutex());
            auto& targetCache = getFileBitmapCacheMap()[target_];
            auto it = targetCache.find(key);
            if (it != targetCache.end()) {
                bmp = it->second;
            } else {
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                static IWICImagingFactory* s_wicFactory = nullptr;
                if (!s_wicFactory) {
                    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_wicFactory));
                }
                if (s_wicFactory) {
                    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, filePath.data(), static_cast<int>(filePath.size()), nullptr, 0);
                    if (wideLen > 0) {
                        std::wstring widePath(static_cast<size_t>(wideLen), 0);
                        MultiByteToWideChar(CP_UTF8, 0, filePath.data(), static_cast<int>(filePath.size()), widePath.data(), wideLen);

                        IWICBitmapDecoder* decoder = nullptr;
                        if (SUCCEEDED(s_wicFactory->CreateDecoderFromFilename(
                                widePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)) && decoder) {
                            IWICBitmapFrameDecode* frame = nullptr;
                            if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
                                UINT srcW = 0, srcH = 0;
                                frame->GetSize(&srcW, &srcH);
                                IWICBitmapSource* wicSource = frame;
                                IWICBitmapScaler* scaler = nullptr;
                                if ((static_cast<float>(srcW) > bounds.width * 2.0f || static_cast<float>(srcH) > bounds.height * 2.0f) &&
                                    SUCCEEDED(s_wicFactory->CreateBitmapScaler(&scaler)) && scaler) {
                                    if (SUCCEEDED(scaler->Initialize(frame, 256, 256, WICBitmapInterpolationModeHighQualityCubic)) ||
                                        SUCCEEDED(scaler->Initialize(frame, 256, 256, WICBitmapInterpolationModeFant))) {
                                        wicSource = scaler;
                                    }
                                }
                                IWICFormatConverter* converter = nullptr;
                                if (SUCCEEDED(s_wicFactory->CreateFormatConverter(&converter)) && converter) {
                                    if (SUCCEEDED(converter->Initialize(
                                            wicSource,
                                            GUID_WICPixelFormat32bppPBGRA,
                                            WICBitmapDitherTypeNone,
                                            nullptr,
                                            0.0,
                                            WICBitmapPaletteTypeMedianCut))) {
                                        target_->CreateBitmapFromWicBitmap(converter, nullptr, &bmp);
                                    }
                                    converter->Release();
                                }
                                if (scaler) scaler->Release();
                                frame->Release();
                            }
                            decoder->Release();
                        }
                    }
                }
                targetCache[key] = bmp;
            }
        }

        if (!bmp) {
            auto& reg = getEmbeddedImageRegistry();
            auto embIt = reg.find(key);
            if (embIt == reg.end()) {
                const auto slashPos = key.find_last_of("/\\");
                if (slashPos != std::string::npos) {
                    embIt = reg.find(key.substr(slashPos + 1));
                }
            }
            if (embIt != reg.end() && embIt->second.first && embIt->second.second > 0) {
                return drawImageFromMemory(embIt->second.first, embIt->second.second, bounds, opacity);
            }
            return false;
        }

        const D2D1_SIZE_F bmpSize = bmp->GetSize();
        if (bmpSize.width <= 0.0f || bmpSize.height <= 0.0f) return false;

        const float scale = (std::min)(bounds.width / bmpSize.width, bounds.height / bmpSize.height);
        const float drawW = bmpSize.width * scale;
        const float drawH = bmpSize.height * scale;
        const float drawX = bounds.x + (bounds.width - drawW) * 0.5f;
        const float drawY = bounds.y + (bounds.height - drawH) * 0.5f;
        const D2D1_RECT_F destRect{ drawX, drawY, drawX + drawW, drawY + drawH };
        target_->DrawBitmap(bmp, &destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
        return true;
#else
        (void)filePath; (void)bounds; (void)opacity;
        return false;
#endif
    }

    /**
     * @brief Registers a compiled-in `constexpr uint8_t[]` asset buffer (`EmbeddedAssets.h`) so single-file
     * `.vst3` and `.exe` builds can render icons and custom sticker decals with zero external files.
     */
    static void registerEmbeddedImage(std::string_view virtualPath, const uint8_t* data, size_t sizeBytes) {
        if (virtualPath.empty() || !data || sizeBytes == 0) return;
        std::string key(virtualPath);
        for (char& ch : key) {
            if (ch == '\\') ch = '/';
        }
        auto& reg = getEmbeddedImageRegistry();
        reg[key] = { data, sizeBytes };
        const auto slashPos = key.find_last_of('/');
        if (slashPos != std::string::npos) {
            reg[key.substr(slashPos + 1)] = { data, sizeBytes };
        }
    }

    /**
     * @brief Decodes and draws an embedded PNG/JPG byte array (`constexpr uint8_t[]`) directly from memory,
     * caching the resulting Direct2D GPU bitmap by buffer pointer.
     */
    bool drawImageFromMemory(const uint8_t* data, size_t sizeBytes, Rect<float> bounds, float opacity = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || !data || sizeBytes == 0 || bounds.width <= 1.0f || bounds.height <= 1.0f) return false;

        const uintptr_t key = reinterpret_cast<uintptr_t>(data);
        ID2D1Bitmap* bmp = nullptr;
        {
            std::lock_guard<std::mutex> lock(getBitmapCacheMutex());
            auto& targetCache = getMemBitmapCacheMap()[target_];
            auto it = targetCache.find(key);
            if (it != targetCache.end()) {
                bmp = it->second;
            } else {
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                static IWICImagingFactory* s_wicFactory = nullptr;
                if (!s_wicFactory) {
                    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_wicFactory));
                }
                if (s_wicFactory) {
                    IWICStream* stream = nullptr;
                    if (SUCCEEDED(s_wicFactory->CreateStream(&stream)) && stream) {
                        if (SUCCEEDED(stream->InitializeFromMemory(
                                const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data)),
                                static_cast<DWORD>(sizeBytes)))) {
                            IWICBitmapDecoder* decoder = nullptr;
                            if (SUCCEEDED(s_wicFactory->CreateDecoderFromStream(
                                    stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder)) && decoder) {
                                IWICBitmapFrameDecode* frame = nullptr;
                                if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
                                    IWICFormatConverter* converter = nullptr;
                                    if (SUCCEEDED(s_wicFactory->CreateFormatConverter(&converter)) && converter) {
                                        if (SUCCEEDED(converter->Initialize(
                                                frame,
                                                GUID_WICPixelFormat32bppPBGRA,
                                                WICBitmapDitherTypeNone,
                                                nullptr,
                                                0.0,
                                                WICBitmapPaletteTypeMedianCut))) {
                                            target_->CreateBitmapFromWicBitmap(converter, nullptr, &bmp);
                                        }
                                        converter->Release();
                                    }
                                    frame->Release();
                                }
                                decoder->Release();
                            }
                        }
                        stream->Release();
                    }
                }
                targetCache[key] = bmp;
            }
        }

        if (!bmp) return false;

        const D2D1_SIZE_F bmpSize = bmp->GetSize();
        if (bmpSize.width <= 0.0f || bmpSize.height <= 0.0f) return false;

        const float scale = (std::min)(bounds.width / bmpSize.width, bounds.height / bmpSize.height);
        const float drawW = bmpSize.width * scale;
        const float drawH = bmpSize.height * scale;
        const float drawX = bounds.x + (bounds.width - drawW) * 0.5f;
        const float drawY = bounds.y + (bounds.height - drawH) * 0.5f;
        const D2D1_RECT_F destRect{ drawX, drawY, drawX + drawW, drawY + drawH };
        target_->DrawBitmap(bmp, &destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
        return true;
#else
        (void)data; (void)sizeBytes; (void)bounds; (void)opacity;
        return false;
#endif
    }

    /**
     * @brief Decodes and draws a full-resolution theme backdrop or sprite atlas sub-region (`srcNormRect` in [0..1])
     * stretched cleanly to `destBounds` with zero downscaling, supporting both compiled-in embedded buffers and disk files.
     */
    bool drawThemeBackdrop(
        std::string_view assetKey,
        Rect<float> destBounds,
        float opacity = 1.0f,
        Rect<float> srcNormRect = { 0.0f, 0.0f, 1.0f, 1.0f }) noexcept {
#if defined(_WIN32)
        if (!target_ || assetKey.empty() || destBounds.width <= 1.0f || destBounds.height <= 1.0f) return false;

        std::string key(assetKey);
        for (char& ch : key) {
            if (ch == '\\') ch = '/';
        }

        ID2D1Bitmap* bmp = nullptr;

        // 1. Check Embedded Asset Registry first (works in single-file .vst3 and .exe builds)
        auto& reg = getEmbeddedImageRegistry();
        auto embIt = reg.find(key);
        if (embIt == reg.end()) {
            const auto slashPos = key.find_last_of('/');
            if (slashPos != std::string::npos) {
                embIt = reg.find(key.substr(slashPos + 1));
            }
        }

        if (embIt != reg.end() && embIt->second.first && embIt->second.second > 0) {
            const uint8_t* data = embIt->second.first;
            const size_t sizeBytes = embIt->second.second;
            const uintptr_t memKey = reinterpret_cast<uintptr_t>(data);
            std::lock_guard<std::mutex> lock(getBitmapCacheMutex());
            auto& targetCache = getMemBitmapCacheMap()[target_];
            auto it = targetCache.find(memKey);
            if (it != targetCache.end()) {
                bmp = it->second;
            } else {
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                static IWICImagingFactory* s_wicFactory = nullptr;
                if (!s_wicFactory) {
                    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_wicFactory));
                }
                if (s_wicFactory) {
                    IWICStream* stream = nullptr;
                    if (SUCCEEDED(s_wicFactory->CreateStream(&stream)) && stream) {
                        if (SUCCEEDED(stream->InitializeFromMemory(
                                const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data)),
                                static_cast<DWORD>(sizeBytes)))) {
                            IWICBitmapDecoder* decoder = nullptr;
                            if (SUCCEEDED(s_wicFactory->CreateDecoderFromStream(
                                    stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder)) && decoder) {
                                IWICBitmapFrameDecode* frame = nullptr;
                                if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
                                    IWICFormatConverter* converter = nullptr;
                                    if (SUCCEEDED(s_wicFactory->CreateFormatConverter(&converter)) && converter) {
                                        if (SUCCEEDED(converter->Initialize(
                                                frame,
                                                GUID_WICPixelFormat32bppPBGRA,
                                                WICBitmapDitherTypeNone,
                                                nullptr,
                                                0.0,
                                                WICBitmapPaletteTypeMedianCut))) {
                                            target_->CreateBitmapFromWicBitmap(converter, nullptr, &bmp);
                                        }
                                        converter->Release();
                                    }
                                    frame->Release();
                                }
                                decoder->Release();
                            }
                        }
                        stream->Release();
                    }
                }
                targetCache[memKey] = bmp;
            }
        }

        // 2. Fallback to full-resolution file load if not found in embedded registry
        if (!bmp) {
            const std::string cacheKey = "fullres:" + key;
            std::lock_guard<std::mutex> lock(getBitmapCacheMutex());
            auto& targetCache = getFileBitmapCacheMap()[target_];
            auto it = targetCache.find(cacheKey);
            if (it != targetCache.end()) {
                bmp = it->second;
            } else {
                std::string resolvedPath = key;
                std::error_code ec;
                if (!std::filesystem::exists(resolvedPath, ec)) {
                    const std::string alt2 = "assets/themes/" + key;
                    if (std::filesystem::exists(alt2, ec)) {
                        resolvedPath = alt2;
                    } else {
                        char exeBuf[MAX_PATH]{};
                        if (GetModuleFileNameA(nullptr, exeBuf, MAX_PATH) > 0) {
                            std::filesystem::path cur = std::filesystem::path(exeBuf).parent_path();
                            for (int d = 0; d < 4; ++d) {
                                auto cand = cur / "assets" / "themes" / key;
                                if (std::filesystem::exists(cand, ec)) {
                                    resolvedPath = cand.string();
                                    break;
                                }
                                if (!cur.has_parent_path() || cur.parent_path() == cur) break;
                                cur = cur.parent_path();
                            }
                        }
                    }
                }
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                static IWICImagingFactory* s_wicFactory = nullptr;
                if (!s_wicFactory) {
                    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_wicFactory));
                }
                if (s_wicFactory) {
                    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, resolvedPath.data(), static_cast<int>(resolvedPath.size()), nullptr, 0);
                    if (wideLen > 0) {
                        std::wstring widePath(static_cast<size_t>(wideLen), 0);
                        MultiByteToWideChar(CP_UTF8, 0, resolvedPath.data(), static_cast<int>(resolvedPath.size()), widePath.data(), wideLen);
                        IWICBitmapDecoder* decoder = nullptr;
                        if (SUCCEEDED(s_wicFactory->CreateDecoderFromFilename(
                                widePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)) && decoder) {
                            IWICBitmapFrameDecode* frame = nullptr;
                            if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
                                IWICFormatConverter* converter = nullptr;
                                if (SUCCEEDED(s_wicFactory->CreateFormatConverter(&converter)) && converter) {
                                    if (SUCCEEDED(converter->Initialize(
                                            frame,
                                            GUID_WICPixelFormat32bppPBGRA,
                                            WICBitmapDitherTypeNone,
                                            nullptr,
                                            0.0,
                                            WICBitmapPaletteTypeMedianCut))) {
                                        target_->CreateBitmapFromWicBitmap(converter, nullptr, &bmp);
                                    }
                                    converter->Release();
                                }
                                frame->Release();
                            }
                            decoder->Release();
                        }
                    }
                }
                targetCache[cacheKey] = bmp;
            }
        }

        if (!bmp) return false;

        const D2D1_SIZE_F bmpSize = bmp->GetSize();
        if (bmpSize.width <= 0.0f || bmpSize.height <= 0.0f) return false;

        const D2D1_RECT_F destRect{ destBounds.x, destBounds.y, destBounds.right(), destBounds.bottom() };
        D2D1_RECT_F srcRect{
            srcNormRect.x * bmpSize.width,
            srcNormRect.y * bmpSize.height,
            srcNormRect.right() * bmpSize.width,
            srcNormRect.bottom() * bmpSize.height
        };
        if (srcNormRect.x <= 0.001f && srcNormRect.y <= 0.001f &&
            srcNormRect.width >= 0.999f && srcNormRect.height >= 0.999f) {
            const float destAspect = destBounds.width / std::max(1.0f, destBounds.height);
            const float bmpAspect  = bmpSize.width / std::max(1.0f, bmpSize.height);
            if (destAspect > bmpAspect * 1.02f) {
                const float cropH = bmpSize.width / destAspect;
                const float padY  = (bmpSize.height - cropH) * 0.5f;
                srcRect.top    = padY;
                srcRect.bottom = bmpSize.height - padY;
            } else if (bmpAspect > destAspect * 1.02f) {
                const float cropW = bmpSize.height * destAspect;
                const float padX  = (bmpSize.width - cropW) * 0.5f;
                srcRect.left  = padX;
                srcRect.right = bmpSize.width - padX;
            }
        }
        target_->DrawBitmap(bmp, &destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &srcRect);
        return true;
#else
        (void)assetKey; (void)destBounds; (void)opacity; (void)srcNormRect;
        return false;
#endif
    }

    /**
     * @brief Synthesizes and caches a procedural 32-bit BGRA surface bitmap (`width x height`) on the GPU
     * and draws it into `bounds` in a single `DrawBitmap` call. Only regenerates when `cacheKey` or `target_` changes.
     */
    bool drawCachedProceduralBitmap(
        uint64_t cacheKey,
        uint32_t width,
        uint32_t height,
        Rect<float> bounds,
        const std::function<void(uint32_t* pixels, uint32_t w, uint32_t h)>& synthesizeFn,
        float opacity = 1.0f) noexcept {
#if defined(_WIN32)
        if (!target_ || width == 0 || height == 0 || bounds.width <= 1.0f || bounds.height <= 1.0f || !synthesizeFn) {
            return false;
        }

        ID2D1Bitmap* bmp = nullptr;
        {
            std::lock_guard<std::mutex> lock(getBitmapCacheMutex());
            auto& targetCache = getProcBitmapCacheMap()[target_];
            if (targetCache.size() > 24) {
                for (auto& kv : targetCache) {
                    if (kv.second) kv.second->Release();
                }
                targetCache.clear();
            }

            auto it = targetCache.find(cacheKey);
            if (it != targetCache.end()) {
                bmp = it->second;
            } else {
                std::vector<uint32_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height), 0xFF000000u);
                synthesizeFn(pixels.data(), width, height);

                const D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                    96.0f,
                    96.0f);
                target_->CreateBitmap(
                    D2D1::SizeU(width, height),
                    pixels.data(),
                    width * 4u,
                    &bmpProps,
                    &bmp);
                targetCache[cacheKey] = bmp;
            }
        }

        if (!bmp) return false;

        const D2D1_RECT_F destRect{ bounds.x, bounds.y, bounds.right(), bounds.bottom() };
        target_->DrawBitmap(bmp, &destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
        return true;
#else
        (void)cacheKey; (void)width; (void)height; (void)bounds; (void)synthesizeFn; (void)opacity;
        return false;
#endif
    }

    /**
     * @brief Renders an arbitrary Direct2D callback into an offscreen WIC bitmap and saves it as a .png file.
     */
    static bool exportRenderCallbackToPng(const std::filesystem::path& filePath,
                                          uint32_t width,
                                          uint32_t height,
                                          const std::function<void(Graphics&)>& renderFn) noexcept {
#if defined(_WIN32)
        if (width == 0 || height == 0 || !renderFn) return false;
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        IWICImagingFactory* wicFactory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory))) || !wicFactory) {
            return false;
        }

        IWICBitmap* wicBitmap = nullptr;
        if (FAILED(wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap)) || !wicBitmap) {
            wicFactory->Release();
            return false;
        }

        ID2D1Factory* d2dFactory = nullptr;
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory)) || !d2dFactory) {
            wicBitmap->Release();
            wicFactory->Release();
            return false;
        }

        static IDWriteFactory* s_dwriteFactory = nullptr;
        if (!s_dwriteFactory) {
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&s_dwriteFactory));
        }

        const D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f, 96.0f);

        ID2D1RenderTarget* wicTarget = nullptr;
        if (FAILED(d2dFactory->CreateWicBitmapRenderTarget(wicBitmap, rtProps, &wicTarget)) || !wicTarget) {
            d2dFactory->Release();
            wicBitmap->Release();
            wicFactory->Release();
            return false;
        }

        ID2D1SolidColorBrush* solidBrush = nullptr;
        wicTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &solidBrush);

        wicTarget->BeginDraw();
        wicTarget->Clear(D2D1::ColorF(0.06f, 0.08f, 0.09f, 1.0f));
        if (solidBrush) {
            Graphics offscreenG(wicTarget, s_dwriteFactory, solidBrush, { 1.0f, 1.0f });
            renderFn(offscreenG);
        }
        const HRESULT drawHr = wicTarget->EndDraw();

        if (solidBrush) solidBrush->Release();
        releaseRenderTargetCaches(wicTarget);
        wicTarget->Release();
        d2dFactory->Release();

        if (FAILED(drawHr)) {
            wicBitmap->Release();
            wicFactory->Release();
            return false;
        }

        std::error_code ec;
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path(), ec);
        }

        bool savedOk = false;
        IWICStream* stream = nullptr;
        if (SUCCEEDED(wicFactory->CreateStream(&stream)) && stream) {
            if (SUCCEEDED(stream->InitializeFromFilename(filePath.wstring().c_str(), GENERIC_WRITE))) {
                IWICBitmapEncoder* encoder = nullptr;
                if (SUCCEEDED(wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) && encoder) {
                    if (SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache))) {
                        IWICBitmapFrameEncode* frameEncode = nullptr;
                        if (SUCCEEDED(encoder->CreateNewFrame(&frameEncode, nullptr)) && frameEncode) {
                            if (SUCCEEDED(frameEncode->Initialize(nullptr))) {
                                frameEncode->SetSize(width, height);
                                WICPixelFormatGUID formatGuid = GUID_WICPixelFormat32bppBGRA;
                                frameEncode->SetPixelFormat(&formatGuid);
                                if (SUCCEEDED(frameEncode->WriteSource(wicBitmap, nullptr))) {
                                    if (SUCCEEDED(frameEncode->Commit()) && SUCCEEDED(encoder->Commit())) {
                                        savedOk = true;
                                    }
                                }
                            }
                            frameEncode->Release();
                        }
                    }
                    encoder->Release();
                }
            }
            stream->Release();
        }

        wicBitmap->Release();
        wicFactory->Release();
        return savedOk;
#else
        (void)filePath; (void)width; (void)height; (void)renderFn;
        return false;
#endif
    }

private:
#if defined(_WIN32)
    IDWriteTextFormat* getOrCreateTextFormat(float fontSize, bool isCentered, bool isBold, FontFamily family) noexcept {
        if (!dwrite_) return nullptr;
        const uint32_t sizeTenths = static_cast<uint32_t>(fontSize * 10.0f + 0.5f);
        const uint32_t famBits    = static_cast<uint32_t>(family) & 0xFu;
        const uint32_t key        = (sizeTenths << 6) | (famBits << 2) | (isCentered ? 2u : 0u) | (isBold ? 1u : 0u);

        static std::unordered_map<uint32_t, IDWriteTextFormat*> s_formatCache;
        auto it = s_formatCache.find(key);
        if (it != s_formatCache.end()) {
            return it->second;
        }

        const DWRITE_FONT_WEIGHT weight = (family == FontFamily::StencilBlock)
            ? DWRITE_FONT_WEIGHT_BOLD
            : (isBold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);

        IDWriteTextFormat* format = nullptr;
        HRESULT hr = dwrite_->CreateTextFormat(
            getFontFamilyWideName(family),
            nullptr,
            weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"en-us",
            &format
        );

        if (SUCCEEDED(hr) && format) {
            format->SetTextAlignment(isCentered ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            s_formatCache[key] = format;
            return format;
        }
        return nullptr;
    }

    ID2D1RenderTarget* target_{nullptr};
    IDWriteFactory* dwrite_{nullptr};
    ID2D1SolidColorBrush* brush_{nullptr};
#endif

    void applyTransform() noexcept {
#if defined(_WIN32)
        if (target_) {
            D2D1::Matrix3x2F mat = D2D1::Matrix3x2F::Identity();
            if (std::abs(rotationDeg_) > 0.01f) {
                mat = D2D1::Matrix3x2F::Rotation(rotationDeg_, D2D1::Point2F(rotationCenter_.x, rotationCenter_.y));
            }
            mat = mat *
                  D2D1::Matrix3x2F::Translation(origin_.x, origin_.y) *
                  D2D1::Matrix3x2F::Scale(scale_.x, scale_.y);
            target_->SetTransform(mat);
        }
#endif
    }

    static std::unordered_map<std::string, std::pair<const uint8_t*, size_t>>& getEmbeddedImageRegistry() noexcept {
        static std::unordered_map<std::string, std::pair<const uint8_t*, size_t>> s_registry;
        return s_registry;
    }

#if defined(_WIN32)
    static std::mutex& getBitmapCacheMutex() noexcept {
        static std::mutex s_mutex;
        return s_mutex;
    }

    static std::unordered_map<ID2D1RenderTarget*, std::unordered_map<std::string, ID2D1Bitmap*>>& getFileBitmapCacheMap() noexcept {
        static std::unordered_map<ID2D1RenderTarget*, std::unordered_map<std::string, ID2D1Bitmap*>> s_map;
        return s_map;
    }

    static std::unordered_map<ID2D1RenderTarget*, std::unordered_map<uintptr_t, ID2D1Bitmap*>>& getMemBitmapCacheMap() noexcept {
        static std::unordered_map<ID2D1RenderTarget*, std::unordered_map<uintptr_t, ID2D1Bitmap*>> s_map;
        return s_map;
    }

    static std::unordered_map<ID2D1RenderTarget*, std::unordered_map<uint64_t, ID2D1Bitmap*>>& getProcBitmapCacheMap() noexcept {
        static std::unordered_map<ID2D1RenderTarget*, std::unordered_map<uint64_t, ID2D1Bitmap*>> s_map;
        return s_map;
    }
#endif

    struct RotationState {
        float        deg{0.0f};
        Point<float> center{0.0f, 0.0f};
    };

    Point<float> origin_{0.0f, 0.0f};
    std::vector<Point<float>> originStack_;
    Point<float> scale_{1.0f, 1.0f};
    float        rotationDeg_{0.0f};
    Point<float> rotationCenter_{0.0f, 0.0f};
    std::vector<RotationState> rotationStack_;
    FontFamily fontFamily_{FontFamily::StudioSans};
    float      fontScale_{1.0f};
    bool       useSkinTextColor_{false};
    Color      skinTextColor_{Color::fromHex(0xE8F1F2)};
    bool       useSkinTextPlateColor_{false};
    Color      skinTextPlateColor_{Color::fromHex(0x0E2A2C, 0.58f)};
};

} // namespace avra::gui
