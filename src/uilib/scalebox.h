#ifndef _UILIB_SCALEBOX_H
#define _UILIB_SCALEBOX_H

#include <algorithm>
#include <cmath>
#include "container.h"

namespace Ui {

/// Container with a single child that is laid out at its natural size once, then drawn scaled to fit,
/// keeping its aspect ratio and centered, the same way a map image is drawn.
/// The child's min size does not propagate up, so the ScaleBox can be any size.
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
        _childMinSize = child->getMinSize();
        const auto natural = getNaturalSize(child);
        const auto& m = child->getMargin();
        child->setPosition({m.left, m.top});
        child->setSize(natural);
        _contentSize = {m.left + natural.width + m.right, m.top + natural.height + m.bottom};
        if (_size.width < 1 && _size.height < 1)
            Container::setSize(_contentSize); // start at 100% until the parent decides
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
        if (_backgroundColor.a > 0) {
            const auto& c = _backgroundColor;
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_Rect r = {offX + _pos.left, offY + _pos.top, _size.width, _size.height};
            SDL_RenderFillRect(renderer, &r);
        }
        if (_children.empty())
            return;
        const auto child = _children.front();
        if (!child->getVisible())
            return;
        if (child->getMinSize() != _childMinSize) {
            // content grew since it was measured (e.g. an image finished loading)
            _childMinSize = child->getMinSize();
            const auto natural = getNaturalSize(child);
            if (natural != child->getSize()) {
                const auto& m = child->getMargin();
                child->setSize(natural);
                _contentSize = {m.left + natural.width + m.right, m.top + natural.height + m.bottom};
            }
            relayout();
        }
        if (_scale <= 0.0f)
            return;
        float oldScaleX, oldScaleY;
        SDL_RenderGetScale(renderer, &oldScaleX, &oldScaleY);
        SDL_RenderSetScale(renderer, oldScaleX * _scale, oldScaleY * _scale);
        // offsets are in unscaled coordinates, the child renders in scaled coordinates
        const int scaledOffX = static_cast<int>(std::lround((offX + _pos.left + _offset.left) / _scale));
        const int scaledOffY = static_cast<int>(std::lround((offY + _pos.top + _offset.top) / _scale));
        child->render(renderer, scaledOffX, scaledOffY);
        SDL_RenderSetScale(renderer, oldScaleX, oldScaleY);
    }

    float getScale() const
    {
        return _scale;
    }

    /// Size a widget wants to be drawn at: the extent of its content, which can be bigger than its min size.
    static Size getNaturalSize(const Widget* widget)
    {
        Size res = widget->getMinSize();
        if (const auto container = dynamic_cast<const Container*>(widget)) {
            for (const auto child: container->getChildren()) {
                const auto natural = getNaturalSize(child);
                res.width = std::max(res.width, child->getLeft() + natural.width + child->getMargin().right);
                res.height = std::max(res.height, child->getTop() + natural.height + child->getMargin().bottom);
            }
        } else {
            // leaf widgets, e.g. an item drawn at item_size while its min size is its image size
            res.width = std::max(res.width, widget->getWidth());
            res.height = std::max(res.height, widget->getHeight());
        }
        return res;
    }

protected:
    Position toChildSpace(const int x, const int y) const override
    {
        if (_scale <= 0.0f)
            return {-1, -1};
        return {
            static_cast<int>(std::floor(static_cast<float>(x - _offset.left) / _scale)),
            static_cast<int>(std::floor(static_cast<float>(y - _offset.top) / _scale)),
        };
    }

    void relayout()
    {
        _scale = 0.0f;
        _offset = {0, 0};
        if (_contentSize.width < 1 || _contentSize.height < 1 || _size.width < 1 || _size.height < 1)
            return;
        _scale = std::min(static_cast<float>(_size.width) / static_cast<float>(_contentSize.width),
                          static_cast<float>(_size.height) / static_cast<float>(_contentSize.height));
        _offset = {
            static_cast<int>(std::lround((_size.width - _contentSize.width * _scale) / 2)),
            static_cast<int>(std::lround((_size.height - _contentSize.height * _scale) / 2)),
        };
    }

    float _scale = 0.0f;
    Position _offset;
    Size _contentSize;
    Size _childMinSize;
};

} // namespace Ui

#endif // _UILIB_SCALEBOX_H
