#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/helpers.h"
#include "esphome/core/gpio.h"
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#include <queue>
#include <tuple>
#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace esphome {
namespace pentair_if_ic {

static const uint8_t PACKET_TYPE_IC = 0;
static const uint8_t PACKET_TYPE_IF = 1;
static const uint8_t IC_CMD_FRAME_HEADER[] = {0x10, 0x02};
static const uint8_t IC_CMD_FRAME_FOOTER[] = {0x10, 0x03};
static const uint8_t STOPPED    = 0x04;
static const uint8_t RUNNING    = 0x0A;
static const uint8_t NO_PROG    = 0x00;
static const uint8_t LOCAL1     = 0x01;
static const uint8_t LOCAL2     = 0x02;
static const uint8_t LOCAL3     = 0x03;
static const uint8_t LOCAL4     = 0x04;
static const uint8_t EXT1       = 0x09;
static const uint8_t EXT2       = 0x11;
static const uint8_t EXT3       = 0x19;
static const uint8_t EXT4       = 0x21;
static const uint8_t TIMEOUT    = 0x08;
static const uint8_t PRIMING    = 0x06;
static const uint8_t QUICKCLEAN = 0x0C;
#define GETBIT8(val, bit) (((val) >> (bit)) & 0x01)

using TxEntry = std::tuple<uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>;

class PentairIfIcComponent : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void set_if_power(sensor::Sensor *s)          { if_power_           = s; }
  void set_if_rpm(sensor::Sensor *s)            { if_rpm_             = s; }
  void set_if_flow(sensor::Sensor *s)           { if_flow_            = s; }
  void set_if_pressure(sensor::Sensor *s)       { if_pressure_        = s; }
  void set_if_time_remaining(sensor::Sensor *s) { if_time_remaining_  = s; }
  void set_if_clock(sensor::Sensor *s)          { if_clock_           = s; }
  void set_salt_ppm_sensor(sensor::Sensor *s)   { salt_ppm_sensor_    = s; }
  void set_water_temp_sensor(sensor::Sensor *s) { water_temp_sensor_  = s; }
  void set_ic_status_sensor(sensor::Sensor *s)  { ic_status_sensor_   = s; }
  void set_ic_error_sensor(sensor::Sensor *s)   { ic_error_sensor_    = s; }
  void set_set_percent_sensor(sensor::Sensor *s){ set_percent_sensor_ = s; }

  void set_if_running(binary_sensor::BinarySensor *s)           { if_running_             = s; }
  void set_no_flow_binary_sensor(binary_sensor::BinarySensor *s)      { no_flow_binary_sensor_      = s; }
  void set_low_salt_binary_sensor(binary_sensor::BinarySensor *s)     { low_salt_binary_sensor_     = s; }
  void set_high_salt_binary_sensor(binary_sensor::BinarySensor *s)    { high_salt_binary_sensor_    = s; }
  void set_clean_binary_sensor(binary_sensor::BinarySensor *s)        { clean_binary_sensor_        = s; }
  void set_high_current_binary_sensor(binary_sensor::BinarySensor *s) { high_current_binary_sensor_ = s; }
  void set_low_volts_binary_sensor(binary_sensor::BinarySensor *s)    { low_volts_binary_sensor_    = s; }
  void set_low_temp_binary_sensor(binary_sensor::BinarySensor *s)     { low_temp_binary_sensor_     = s; }
  void set_check_pcb_binary_sensor(binary_sensor::BinarySensor *s)    { check_pcb_binary_sensor_    = s; }

  void set_if_program(text_sensor::TextSensor *s)            { if_program_             = s; }
  void set_ic_version_text_sensor(text_sensor::TextSensor *s){ ic_version_text_sensor_ = s; }
  void set_ic_debug_text_sensor(text_sensor::TextSensor *s)  { ic_debug_text_sensor_   = s; }

  void set_takeover_mode_switch(switch_::Switch *s) { takeover_mode_switch_ = s; }
  void set_pump_run_switch(switch_::Switch *s)      { pump_run_switch_      = s; }

#ifdef USE_NUMBER
  void set_swg_percent_number(number::Number *n)    { swg_percent_number_ = n; }
#endif

  void set_flow_control_pin(GPIOPin *pin) { flow_control_pin_ = pin; }

  void set_pump_run_state(bool state);
  void set_takeover_mode(bool enable);
  void set_swg_percent();
  void refresh_chlorinator();
  void read_all_chlorinator_info();
  void requestPumpStatus();
  void pumpToLocalControl();
  void pumpToRemoteControl();
  void run();
  void stop();
  void commandLocalProgram(int prog);
  void commandExternalProgram(int prog);
  void saveValueForProgram(int prog, int value);
  void commandRPM(int rpm);
  void commandFlow(int flow);
  void setPumpClock(int hour, int minute);

 protected:
  sensor::Sensor *if_power_           {nullptr};
  sensor::Sensor *if_rpm_             {nullptr};
  sensor::Sensor *if_flow_            {nullptr};
  sensor::Sensor *if_pressure_        {nullptr};
  sensor::Sensor *if_time_remaining_  {nullptr};
  sensor::Sensor *if_clock_           {nullptr};
  sensor::Sensor *salt_ppm_sensor_    {nullptr};
  sensor::Sensor *water_temp_sensor_  {nullptr};
  sensor::Sensor *ic_status_sensor_   {nullptr};
  sensor::Sensor *ic_error_sensor_    {nullptr};
  sensor::Sensor *set_percent_sensor_ {nullptr};

  binary_sensor::BinarySensor *if_running_             {nullptr};
  binary_sensor::BinarySensor *no_flow_binary_sensor_      {nullptr};
  binary_sensor::BinarySensor *low_salt_binary_sensor_     {nullptr};
  binary_sensor::BinarySensor *high_salt_binary_sensor_    {nullptr};
  binary_sensor::BinarySensor *clean_binary_sensor_        {nullptr};
  binary_sensor::BinarySensor *high_current_binary_sensor_ {nullptr};
  binary_sensor::BinarySensor *low_volts_binary_sensor_    {nullptr};
  binary_sensor::BinarySensor *low_temp_binary_sensor_     {nullptr};
  binary_sensor::BinarySensor *check_pcb_binary_sensor_    {nullptr};

  text_sensor::TextSensor *if_program_             {nullptr};
  text_sensor::TextSensor *ic_version_text_sensor_ {nullptr};
  text_sensor::TextSensor *ic_debug_text_sensor_   {nullptr};

  switch_::Switch *takeover_mode_switch_ {nullptr};
  switch_::Switch *pump_run_switch_      {nullptr};

#ifdef USE_NUMBER
  number::Number *swg_percent_number_ {nullptr};
#endif

  GPIOPin *flow_control_pin_ {nullptr};

  std::string ic_version_;
  bool pump_remote_control_active_ {false};
  uint8_t ic_last_set_percent_     {0};
  uint32_t ic_last_command_timestamp_ {0};
  uint32_t ic_last_recv_timestamp_    {0};
  uint32_t ic_last_loop_timestamp_    {0};
  uint32_t last_received_byte_millis_ {0};
  uint32_t last_tx_millis_            {0};

  std::vector<uint8_t> rx_buffer_;
  std::queue<TxEntry>  tx_queue_;

  void get_ic_version_();
  void get_ic_temp_();
  void get_ic_more_();
  void ic_takeover_();
  void ic_set_percent_(uint8_t percent);
  void send_ic_command_(const uint8_t *command, int command_len, uint8_t retries);
  bool parse_ic_packet_();
  bool validate_if_received_message_();
  void parse_if_packet_(const std::vector<uint8_t> &data);
  void queue_if_packet_(uint8_t message[], int messageLength);
  template<typename... Args>
  std::string string_format_(const std::string &format, Args... args);
};

}  // namespace pentair_if_ic
}  // namespace esphome
