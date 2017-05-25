#pragma once

#include "Input/InputDevice.h"

namespace DAVA
{
class InputSystem;

namespace Private
{
struct MainDispatcherEvent;
}

/**
    \ingroup input
    Represents mouse input device.
*/
class Mouse final : public InputDevice
{
    friend class DeviceManager; // For creation

public:
    // InputDevice overrides
    bool IsElementSupported(eInputElements elementId) const override;
    DigitalElementState GetDigitalElementState(eInputElements elementId) const override;
    AnalogElementState GetAnalogElementState(eInputElements elementId) const override;

    /**
        Return first button element which is pressed right now.
        If all buttons are released, return eInputElements::NONE.
    */
    eInputElements GetFirstPressedButton() const;

private:
    explicit Mouse(uint32 id);
    ~Mouse() override;

    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

    bool HandleEvent(const Private::MainDispatcherEvent& e);
    void HandleMouseClick(const Private::MainDispatcherEvent& e);
    void HandleMouseWheel(const Private::MainDispatcherEvent& e);
    void HandleMouseMove(const Private::MainDispatcherEvent& e);

    void OnEndFrame();

private:
    InputSystem* inputSystem = nullptr;

    Array<DigitalElementState, INPUT_ELEMENTS_MOUSE_BUTTON_COUNT> buttons;
    AnalogElementState mousePosition;
    AnalogElementState mouseWheelDelta;
};

} // namespace DAVA
