#ifndef STEPPER_CUTTER_ESP_USB_HOST_KEYBORD_H
#define STEPPER_CUTTER_ESP_USB_HOST_KEYBORD_H

#include <EspUsbHost.h>

class EspUsbHostKeybord : public EspUsbHost {
public:
  virtual void onKey(usb_transfer_t *transfer) {}

  void onReceive(const usb_transfer_t *transfer) override {
    onKey(const_cast<usb_transfer_t *>(transfer));
  }
};

#endif
