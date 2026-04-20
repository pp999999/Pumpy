#pragma once
#include "esphome/components/switch/switch.h"
#include "../pentair_if_ic.h"

namespace esphome {
namespace pentair_if_ic {

// PumpRunSwitch: exposes run/stop control for the IntelliFlo pump.
// ON  -> takes remote control of the pump, then sends run command.
// OFF -> sends stop command, then returns pump to local control.
// State is kept in sync with actual pump running status via parse_if_packet_.
class PumpRunSwitch : public switch_::Switch, public Parented<PentairIfIcComponent> {
 public:
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->set_pump_run_state(state);
  }
};

}  // namespace pentair_if_ic
}  // namespace esphome
