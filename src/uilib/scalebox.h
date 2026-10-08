#ifndef _UILIB_SCALEBOX_H
#define _UILIB_SCALEBOX_H

#include <algorithm>
#include <cmath>
#include "container.h"

namespace Ui {

/// Container with a single child that is scaled down to fit when there is not enough space.
/// Unlike other containers, the child's min size does not propagate up, so the ScaleBox can be any size.
class ScaleBox : public Container {
public:
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
        if (child->getMinSize() != _childMinSize)
            relayout(); // child's min size changed since last layout
        if (_scale >= 1.0f) {
            Container::render(renderer, offX, offY);
            return;
        }
        float oldScaleX, oldScaleY;
        SDL_RenderGetScale(renderer, &oldScaleX, &oldScaleY);
        SDL_RenderSetScale(renderer, oldScaleX * _scale, oldScaleY * _scale);
        // offsets are in unscaled coordinates, children render in scaled coordinates
        const int scaledOffX = static_cast<int>(std::lround((offX + _pos.left) / _scale));
        const int scaledOffY = static_cast<int>(std::lround((offY + _pos.top) / _scale));
        if (child->getVisible())
            child->render(renderer, scaledOffX, scaledOffY);
        SDL_RenderSetScale(renderer, oldScaleX, oldScaleY);
    }

    float getScale() const
    {
        return _scale;
    }

protected:
    Position toChildSpace(const int x, const int y) const override
    {
        if (_scale >= 1.0f)
            return {x, y};
        return {
            static_cast<int>(std::floor(static_cast<float>(x) / _scale)),
            static_cast<int>(std::floor(static_cast<float>(y) / _scale)),
        };
    }

    void relayout()
    {
        if (_children.empty())
            return;
        const auto child = _children.front();
        _childMinSize = child->getMinSize();
        if (_size.width < 1 || _size.height < 1)
            return; // not sized yet
        const int needWidth = child->getLeft() + _childMinSize.width + child->getMargin().right;
        const int needHeight = child->getTop() + _childMinSize.height + child->getMargin().bottom;
        _scale = 1.0f;
        if (needWidth > _size.width)
            _scale = std::min(_scale, static_cast<float>(_size.width) / static_cast<float>(needWidth));
        if (needHeight > _size.height)
            _scale = std::min(_scale, static_cast<float>(_size.height) / static_cast<float>(needHeight));
        // give the child all the (scaled) space, so growing children still fill the box
        const int scaledWidth = static_cast<int>(std::lround(static_cast<float>(_size.width) / _scale));
        const int scaledHeight = static_cast<int>(std::lround(static_cast<float>(_size.height) / _scale));
        child->setSize({
            std::max(scaledWidth, needWidth) - child->getLeft() - child->getMargin().right,
            std::max(scaledHeight, needHeight) - child->getTop() - child->getMargin().bottom,
        });
    }

    float _scale = 1.0f;
    Size _childMinSize;
};

} // namespace Ui

#endif // _UILIB_SCALEBOX_H
