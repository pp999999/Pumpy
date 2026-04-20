#pragma once
#include "esphome/components/switch/switch.h"
#include "../pentair_if_ic.h"

namespace esphome {
namespace pentair_if_ic {

class TakeoverModeSwitch : public switch_::Switch, public Parented<PentairIfIcComponent> {
 public:
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->set_takeover_mode(state);
  }
};

}  // namespace pentair_if_ic
}  // namespace esphome
