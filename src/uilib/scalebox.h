#ifndef _UILIB_SCALEBOX_H
#define _UILIB_SCALEBOX_H

#include <algorithm>
#include <cmath>
#include "container.h"

namespace Ui {

/// Container with a single child that is laid out normally when there is enough space for the child's min size,
/// and laid out at its min size and drawn scaled down to fit (keeping its aspect ratio) when there is not.
/// It reports a fraction of the child's min size as its own min size, see MIN_SCALE.
class ScaleBox : public Container {
public:
    static constexpr float MIN_SCALE = 0.25f;

    ScaleBox(const int x, const int y, const int w, const int h)
        : Container(x, y, w, h)
    {
        _hGrow = 1;
        _vGrow = 1;
    }

    void addChild(Widget* child) override
    {
        if (!child)
            return;
        Container::addChild(child);
        _hGrow = 1; // always fill available space, regardless of child
        _vGrow = 1;
        child->setPosition({child->getMargin().left, child->getMargin().top});
        measure();
        relayout();
    }

    void setSize(const Size size) override
    {
        if (size == _size)
            return;
        Container::setSize(size);
        relayout();
    }

    void render(Renderer renderer, const int offX, const int offY) override
    {
        if (_children.empty())
            return;
        const auto child = _children.front();
        if (child->getMinSize() != _childMinSize) {
            // child's min size changed since it was measured
            measure();
            relayout();
        }
        if (_scale >= 1.0f) {
            Container::render(renderer, offX, offY);
            return;
        }
        if (_scale <= 0.0f || !child->getVisible())
            return;
        if (_backgroundColor.a > 0) {
            const auto& c = _backgroundColor;
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_Rect r = {offX + _pos.left, offY + _pos.top, _size.width, _size.height};
            SDL_RenderFillRect(renderer, &r);
        }
        float oldScaleX, oldScaleY;
        SDL_RenderGetScale(renderer, &oldScaleX, &oldScaleY);
        SDL_RenderSetScale(renderer, oldScaleX * _scale, oldScaleY * _scale);
        // offsets are in unscaled coordinates, the child renders in scaled coordinates
        const int scaledOffX = static_cast<int>(std::lround((offX + _pos.left) / _scale));
        const int scaledOffY = static_cast<int>(std::lround((offY + _pos.top) / _scale));
        child->render(renderer, scaledOffX, scaledOffY);
        SDL_RenderSetScale(renderer, oldScaleX, oldScaleY);
    }

    /// Current scale, 1 if the child is drawn at its normal size.
    float getScale() const
    {
        return _scale;
    }

    /// Smallest size the child can be drawn unscaled at, including its margin.
    const Size& getContentSize() const
    {
        return _contentSize;
    }

protected:
    Position toChildSpace(const int x, const int y) const override
    {
        if (_scale >= 1.0f)
            return {x, y};
        if (_scale <= 0.0f)
            return {-1, -1};
        return {
            static_cast<int>(std::floor(static_cast<float>(x) / _scale)),
            static_cast<int>(std::floor(static_cast<float>(y) / _scale)),
        };
    }

    void measure()
    {
        const auto child = _children.front();
        const auto& m = child->getMargin();
        _childMinSize = child->getMinSize();
        _contentSize = {m.left + _childMinSize.width + m.right, m.top + _childMinSize.height + m.bottom};
        _minSize = {
            static_cast<int>(std::lround(static_cast<float>(_contentSize.width) * MIN_SCALE)),
            static_cast<int>(std::lround(static_cast<float>(_contentSize.height) * MIN_SCALE)),
        };
    }

    void relayout()
    {
        if (_children.empty())
            return;
        _scale = 1.0f;
        if (_size.width < 1 || _size.height < 1)
            return; // not sized yet
        if (_contentSize.width > _size.width)
            _scale = std::min(_scale, static_cast<float>(_size.width) / static_cast<float>(_contentSize.width));
        if (_contentSize.height > _size.height)
            _scale = std::min(_scale, static_cast<float>(_size.height) / static_cast<float>(_contentSize.height));
        // give the child all the (scaled) space, so growing children still fill the box
        const auto child = _children.front();
        const auto& m = child->getMargin();
        const int scaledWidth = static_cast<int>(std::lround(static_cast<float>(_size.width) / _scale));
        const int scaledHeight = static_cast<int>(std::lround(static_cast<float>(_size.height) / _scale));
        child->setSize({
            std::max(scaledWidth, _contentSize.width) - m.left - m.right,
            std::max(scaledHeight, _contentSize.height) - m.top - m.bottom,
        });
    }

    float _scale = 1.0f;
    Size _contentSize;
    Size _childMinSize;
};

} // namespace Ui

#endif // _UILIB_SCALEBOX_H
