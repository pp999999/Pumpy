#pragma once
#include "esphome/components/number/number.h"
#include "../pentair_if_ic.h"

namespace esphome {
namespace pentair_if_ic {

class SWGPercentNumber : public number::Number, public Parented<PentairIfIcComponent> {
 public:
  void control(float value) override {
    this->publish_state(value);
    this->parent_->set_swg_percent();
  }
};

}  // namespace pentair_if_ic
}  // namespace esphome
