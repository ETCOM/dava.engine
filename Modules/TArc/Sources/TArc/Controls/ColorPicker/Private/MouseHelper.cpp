#include "TArc/Controls/ColorPicker/Private/MouseHelper.h"

#include <QMouseEvent>

namespace DAVA
{
MouseHelper::MouseHelper(QWidget* _w)
    : QObject(_w)
    , w(_w)
    , isHover(false)
    , isPressed(false)
    , clickDist(4)
    , dblClickDist(4)
{
    Q_ASSERT(w);
    w->installEventFilter(this);
}

bool MouseHelper::IsPressed() const
{
    return isPressed;
}

bool MouseHelper::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == w)
    {
        switch (e->type())
        {
        case QEvent::Enter:
            enterEvent(e);
            break;
        case QEvent::Leave:
            leaveEvent(e);
            break;
        case QEvent::MouseMove:
            mouseMoveEvent(static_cast<QMouseEvent*>(e));
            break;
        case QEvent::MouseButtonPress:
            mousePressEvent(static_cast<QMouseEvent*>(e));
            break;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(static_cast<QMouseEvent*>(e));
            break;
        case QEvent::Wheel:
            mouseWheelEvent(static_cast<QWheelEvent*>(e));
            break;

        default:
            break;
        }
    }

    return QObject::eventFilter(obj, e);
}

void MouseHelper::enterEvent(QEvent* event)
{
    Q_UNUSED(event);
    isHover = true;
    emit mouseEntered();
}

void MouseHelper::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    isHover = false;
    emit mouseLeaved();
}

void MouseHelper::mouseMoveEvent(QMouseEvent* event)
{
    pos = event->pos();
    emit mouseMove(pos);
}

void MouseHelper::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;

    pos = event->pos();
    clickPos = pos;
    isPressed = true;
    emit mousePress(pos);
}

void MouseHelper::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    if (!isPressed)
        return;

    pos = event->pos();
    isPressed = false;
    emit mouseRelease(pos);

    const QPoint dist = pos - clickPos;
    const int sqrDist = dist.x() * dist.x() + dist.y() * dist.y();
    if (sqrDist <= clickDist * clickDist)
    {
        emit clicked();
    }
}

void MouseHelper::mouseWheelEvent(QWheelEvent* event)
{
    emit mouseWheel(event->delta());
}
}
