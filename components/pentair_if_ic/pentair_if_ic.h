#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"
#include <queue>

namespace esphome {
namespace pentair_if_ic {

static const uint8_t IC_CMD_FRAME_HEADER[2] = {0x10, 0x02};
static const uint8_t IC_CMD_FRAME_FOOTER[2] = {0x10, 0x03};

#define GETBIT8(a, b) ((a) & ((uint8_t) 1 << (b)))

enum running : uint8_t { STOPPED = 0x04, RUNNING = 0x0A };
enum program : uint8_t {
  NO_PROG = 0x00, LOCAL1 = 0x01, LOCAL2 = 0x02, LOCAL3 = 0x03, LOCAL4 = 0x04,
  EXT1 = 0x09, EXT2 = 0x0A, EXT3 = 0x0B, EXT4 = 0x0C,
  TIMEOUT = 0x0E, PRIMING = 0x11, QUICKCLEAN = 0x0D, UNKNOWN = 0xFF,
};

class PentairIfIcComponent : public PollingComponent, public uart::UARTDevice {
  SUB_TEXT_SENSOR(ic_version)
  SUB_TEXT_SENSOR(ic_debug)
  SUB_SWITCH(takeover_mode)
  SUB_SWITCH(pump_run)
#ifdef USE_NUMBER
  SUB_NUMBER(swg_percent)
#endif
  SUB_SENSOR(salt_ppm)
  SUB_SENSOR(water_temp)
  SUB_SENSOR(ic_status)
  SUB_SENSOR(ic_error)
  SUB_SENSOR(set_percent)
  SUB_BINARY_SENSOR(no_flow)
  SUB_BINARY_SENSOR(low_salt)
  SUB_BINARY_SENSOR(high_salt)
  SUB_BINARY_SENSOR(clean)
  SUB_BINARY_SENSOR(high_current)
  SUB_BINARY_SENSOR(low_volts)
  SUB_BINARY_SENSOR(low_temp)
  SUB_BINARY_SENSOR(check_pcb)

 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

  void read_all_chlorinator_info();
  void read_all_info() { read_all_chlorinator_info(); }
  void refresh_chlorinator();
  void set_swg_percent();
  void set_takeover_mode(bool enable);

  void requestPumpStatus();
  void run();
  void stop();
  void commandLocalProgram(int prog);
  void commandExternalProgram(int prog);
  void saveValueForProgram(int prog, int value);
  void commandRPM(int rpm);
  void commandFlow(int flow);
  void pumpToLocalControl();
  void pumpToRemoteControl();
  void setPumpClock(int hour, int minute);

  // Called by PumpRunSwitch
  void set_pump_run_state(bool state);
  bool pump_remote_control_active_{false};

  void set_flow_control_pin(GPIOPin *flow_control_pin) { this->flow_control_pin_ = flow_control_pin; }

  void set_if_power(sensor::Sensor *s)                   { if_power_ = s; }
  void set_if_rpm(sensor::Sensor *s)                     { if_rpm_ = s; }
  void set_if_flow(sensor::Sensor *s)                    { if_flow_ = s; }
  void set_if_pressure(sensor::Sensor *s)                { if_pressure_ = s; }
  void set_if_time_remaining(sensor::Sensor *s)          { if_time_remaining_ = s; }
  void set_if_clock(sensor::Sensor *s)                   { if_clock_ = s; }
  void set_if_running(binary_sensor::BinarySensor *s)    { if_running_ = s; }
  void set_if_program(text_sensor::TextSensor *s)        { if_program_ = s; }

 protected:
  GPIOPin *flow_control_pin_{nullptr};
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_received_byte_millis_ = 0;
  uint32_t last_tx_millis_ = 0;

  void get_ic_version_();
  void get_ic_temp_();
  void get_ic_more_();
  void ic_takeover_();
  void ic_set_percent_(uint8_t percent);
  void send_ic_command_(const uint8_t *command, int command_len, uint8_t retries);
  bool parse_ic_packet_();

  enum PacketType : uint8_t { PACKET_TYPE_IF = 0, PACKET_TYPE_IC = 1 };
  std::queue<std::tuple<PacketType, uint8_t, uint8_t, std::vector<uint8_t>>> tx_queue_;

  uint32_t ic_last_command_timestamp_;
  uint32_t ic_last_recv_timestamp_;
  uint32_t ic_last_loop_timestamp_;
  uint8_t ic_last_set_percent_ = 0;
  bool ic_run_again_;
  std::string ic_version_;

  void parse_if_packet_(const std::vector<uint8_t> &data);
  bool validate_if_received_message_();
  void queue_if_packet_(uint8_t message[], int messageLength);

  sensor::Sensor *if_power_{nullptr};
  sensor::Sensor *if_rpm_{nullptr};
  sensor::Sensor *if_flow_{nullptr};
  sensor::Sensor *if_pressure_{nullptr};
  sensor::Sensor *if_time_remaining_{nullptr};
  sensor::Sensor *if_clock_{nullptr};
  binary_sensor::BinarySensor *if_running_{nullptr};
  text_sensor::TextSensor *if_program_{nullptr};

  template<typename... Args>
  std::string string_format_(const std::string &format, Args... args);
};

}  // namespace pentair_if_ic
}  // namespace esphome
