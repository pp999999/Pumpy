#include "pentair_if_ic.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace pentair_if_ic {

static const char *TAG = "pentair_if_ic";

void PentairIfIcComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Pentair IntelliFlo + IntelliChlor...");
  this->read_all_chlorinator_info();
  ESP_LOGCONFIG(TAG, "IntelliChlor Version: %s", this->ic_version_.c_str());
  if (this->flow_control_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "Using Flow Control");
    this->flow_control_pin_->setup();
  }
  this->ic_last_command_timestamp_ = millis();
  this->ic_last_recv_timestamp_ = millis();
  this->ic_last_loop_timestamp_ = millis() - 31000;
  this->last_received_byte_millis_ = millis();
}

void PentairIfIcComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Pentair IntelliFlo + IntelliChlor RS485 Component");
  LOG_TEXT_SENSOR("  ", "IC_VersionTextSensor", this->ic_version_text_sensor_);
  LOG_SWITCH("  ", "TakeoverModeSwitch", this->takeover_mode_switch_);
  LOG_SWITCH("  ", "PumpRunSwitch", this->pump_run_switch_);
#ifdef USE_NUMBER
  LOG_NUMBER("  ", "SWGPercentNumber", this->swg_percent_number_);
#endif
  LOG_SENSOR("  ", "WaterTempSensor", this->water_temp_sensor_);
  LOG_SENSOR("  ", "SaltPPMSensor", this->salt_ppm_sensor_);
  LOG_SENSOR("  ", "IC_ErrorSensor", this->ic_error_sensor_);
  LOG_SENSOR("  ", "IC_StatusSensor", this->ic_status_sensor_);
  LOG_SENSOR("  ", "IF_PowerSensor", this->if_power_);
  LOG_SENSOR("  ", "IF_RPMSensor", this->if_rpm_);
  LOG_BINARY_SENSOR("  ", "IF_RunningBinarySensor", this->if_running_);
  LOG_TEXT_SENSOR("  ", "IF_ProgramTextSensor", this->if_program_);
  LOG_PIN("  Flow Control Pin: ", this->flow_control_pin_);
}

void PentairIfIcComponent::loop() {
  while (this->available() > 0) {
    uint8_t c;
    this->read_byte(&c);
    this->last_received_byte_millis_ = millis();
    ESP_LOGV(TAG, "Received byte: %02X, buffer size: %d", c, this->rx_buffer_.size());
    if (!this->rx_buffer_.empty()) {
      this->rx_buffer_.push_back(c);
      if (this->rx_buffer_[0] == 0xFF) {
        if (!this->validate_if_received_message_()) this->rx_buffer_.clear();
      } else if (this->rx_buffer_[0] == 0x10) {
        if (this->parse_ic_packet_()) this->rx_buffer_.clear();
      } else {
        ESP_LOGW(TAG, "Invalid packet start: %02X", this->rx_buffer_[0]);
        this->rx_buffer_.clear();
      }
    } else if (c == 0xFF || c == 0x10) {
      this->rx_buffer_.push_back(c);
    } else {
      ESP_LOGV(TAG, "Ignoring unexpected byte: %02X", c);
    }
  }

  auto since_last_cmd = millis() - this->ic_last_command_timestamp_;
  auto since_last_tx  = millis() - this->last_tx_millis_;
  auto since_last_rx  = millis() - this->last_received_byte_millis_;

  if (since_last_cmd > 100 && since_last_tx > 150 && since_last_rx > 100) {
    if (!this->tx_queue_.empty()) {
      auto packet   = this->tx_queue_.front();
      auto type     = std::get<0>(packet);
      auto retries  = std::get<1>(packet);
      auto attempts = std::get<2>(packet);
      auto data     = std::get<3>(packet);
      attempts++;
      if (type == PACKET_TYPE_IC) {
        ESP_LOGD(TAG, "IC Process Queue Retries:%i Attempt:%i", retries, attempts);
        if (attempts > retries) {
          ESP_LOGE(TAG, "IC No response %i > %i removing from send queue", retries, attempts);
          this->tx_queue_.pop();
        } else {
          std::get<2>(this->tx_queue_.front()) = attempts;
          if (this->flow_control_pin_ != nullptr) this->flow_control_pin_->digital_write(true);
          ESP_LOGI(TAG, "IC Sent: %s", format_hex_pretty(data).c_str());
          this->write_array(data);
          this->flush();
          if (this->flow_control_pin_ != nullptr) this->flow_control_pin_->digital_write(false);
          this->ic_last_command_timestamp_ = millis();
          this->last_tx_millis_ = millis();
        }
      } else if (type == PACKET_TYPE_IF) {
        this->flush();
        this->write_array(&data[0], data.size());
        ESP_LOGI(TAG, "IF Sent: %s", format_hex_pretty(data).c_str());
        this->last_received_byte_millis_ = millis();
        this->last_tx_millis_ = millis();
        this->tx_queue_.pop();
      }
    }
  }
}

void PentairIfIcComponent::update() {
  this->read_all_chlorinator_info();
  this->set_timeout(500, [this]() {
    this->requestPumpStatus();
    if (!this->pump_remote_control_active_)
      this->pumpToLocalControl();
  });
}

void PentairIfIcComponent::set_pump_run_state(bool state) {
  if (state) {
    ESP_LOGI(TAG, "PumpRunSwitch: taking remote control and running pump");
    this->pump_remote_control_active_ = true;
    this->pumpToRemoteControl();
    this->set_timeout(200, [this]() { this->run(); });
  } else {
    ESP_LOGI(TAG, "PumpRunSwitch: stopping pump and returning to local control");
    this->stop();
    this->set_timeout(200, [this]() {
      this->pump_remote_control_active_ = false;
      this->pumpToLocalControl();
    });
  }
}

void PentairIfIcComponent::read_all_chlorinator_info() {
  if (millis() - this->ic_last_loop_timestamp_ > 25000) {
    this->ic_last_loop_timestamp_ = millis();
    if (this->takeover_mode_switch_ != nullptr && this->takeover_mode_switch_->state) {
      this->ic_takeover_();
#ifdef USE_NUMBER
      if (this->swg_percent_number_ != nullptr)
        this->ic_set_percent_(this->swg_percent_number_->state);
#endif
    }
    this->get_ic_version_();
    this->get_ic_temp_();
    this->get_ic_more_();
  }
}

void PentairIfIcComponent::refresh_chlorinator() {
  ESP_LOGD(TAG, "Manual chlorinator refresh requested");
  this->ic_last_loop_timestamp_ = millis();
  if (this->takeover_mode_switch_ != nullptr && this->takeover_mode_switch_->state) {
    this->ic_takeover_();
#ifdef USE_NUMBER
    if (this->swg_percent_number_ != nullptr)
      this->ic_set_percent_(this->swg_percent_number_->state);
#endif
  }
  this->get_ic_version_();
  this->get_ic_temp_();
  this->get_ic_more_();
}

void PentairIfIcComponent::set_swg_percent() {
  if (this->takeover_mode_switch_ != nullptr && this->takeover_mode_switch_->state)
    this->read_all_chlorinator_info();
}
void PentairIfIcComponent::set_takeover_mode(bool enable) { this->read_all_chlorinator_info(); }
void PentairIfIcComponent::get_ic_more_() {}

void PentairIfIcComponent::get_ic_version_() {
  uint8_t cmd[3] = {0x50, 0x14, 0x00};
  this->send_ic_command_(cmd, 3, 1);
}
void PentairIfIcComponent::get_ic_temp_() {
  uint8_t cmd[3] = {0x50, 0x15, 0x00};
  this->send_ic_command_(cmd, 3, 3);
}
void PentairIfIcComponent::ic_takeover_() {
  uint8_t cmd[3] = {0x50, 0x00, 0x00};
  this->send_ic_command_(cmd, 3, 3);
}
void PentairIfIcComponent::ic_set_percent_(uint8_t percent) {
  this->ic_last_set_percent_ = percent;
  if (percent == 16) {
    uint8_t cmd[4] = {0x50, 0x11, percent, 0x00};
    this->send_ic_command_(cmd, 4, 3);
  } else {
    uint8_t cmd[3] = {0x50, 0x11, percent};
    this->send_ic_command_(cmd, 3, 3);
  }
}

void PentairIfIcComponent::send_ic_command_(const uint8_t *command, int command_len, uint8_t retries) {
  uint8_t crc = 0;
  std::vector<uint8_t> packet;
  packet.reserve(command_len + 5);
  packet.push_back(IC_CMD_FRAME_HEADER[0]); crc += IC_CMD_FRAME_HEADER[0];
  packet.push_back(IC_CMD_FRAME_HEADER[1]); crc += IC_CMD_FRAME_HEADER[1];
  if (command != nullptr)
    for (int i = 0; i < command_len; i++) { packet.push_back(command[i]); crc += command[i]; }
  packet.push_back(crc);
  packet.push_back(IC_CMD_FRAME_FOOTER[0]);
  packet.push_back(IC_CMD_FRAME_FOOTER[1]);
  this->tx_queue_.push(std::make_tuple(PACKET_TYPE_IC, retries, (uint8_t)0, packet));
}

bool PentairIfIcComponent::parse_ic_packet_() {
  size_t len = this->rx_buffer_.size();
  if (len < 2) return false;
  if (this->rx_buffer_[0] != 0x10) { ESP_LOGW(TAG, "IC Invalid header"); return true; }
  if (this->rx_buffer_[1] != 0x02) {
    if (len >= 64) { ESP_LOGW(TAG, "IC Buffer overflow"); return true; }
    return false;
  }
  if (len >= 4) {
    for (size_t i = 2; i < len - 1; i++) {
      if (this->rx_buffer_[i] == 0x10 && this->rx_buffer_[i + 1] == 0x03) {
        this->ic_last_recv_timestamp_ = millis();
        ESP_LOGI(TAG, "IC Package received: %s", format_hex_pretty(this->rx_buffer_).c_str());
        uint8_t *buf = &this->rx_buffer_[0];
        int pos = len - 1;
        if (pos >= 4 && buf[3] == 0x03) {
          this->ic_version_ = "";
          for (int j = 5; j <= pos - 3; j++) this->ic_version_ += buf[j];
          if (this->ic_version_text_sensor_ != nullptr) this->ic_version_text_sensor_->publish_state(this->ic_version_);
        } else if (pos >= 4 && buf[3] == 0x16) {
          if (this->water_temp_sensor_ != nullptr) this->water_temp_sensor_->publish_state(buf[4]);
        } else if (pos >= 4 && buf[3] == 0x12) {
          uint16_t saltPPM = buf[4] * 50;
          auto ef = buf[5];
          if (this->no_flow_binary_sensor_)      this->no_flow_binary_sensor_->publish_state(GETBIT8(ef, 0));
          if (this->low_salt_binary_sensor_)     this->low_salt_binary_sensor_->publish_state(GETBIT8(ef, 1));
          if (this->high_salt_binary_sensor_)    this->high_salt_binary_sensor_->publish_state(GETBIT8(ef, 2));
          if (this->clean_binary_sensor_)        this->clean_binary_sensor_->publish_state(GETBIT8(ef, 3));
          if (this->high_current_binary_sensor_) this->high_current_binary_sensor_->publish_state(GETBIT8(ef, 4));
          if (this->low_volts_binary_sensor_)    this->low_volts_binary_sensor_->publish_state(GETBIT8(ef, 5));
          if (this->low_temp_binary_sensor_)     this->low_temp_binary_sensor_->publish_state(GETBIT8(ef, 6));
          if (this->check_pcb_binary_sensor_)    this->check_pcb_binary_sensor_->publish_state(GETBIT8(ef, 7));
          if (this->salt_ppm_sensor_)    this->salt_ppm_sensor_->publish_state(saltPPM);
          if (this->ic_error_sensor_)    this->ic_error_sensor_->publish_state(ef);
          if (this->set_percent_sensor_) this->set_percent_sensor_->publish_state(this->ic_last_set_percent_);
        } else if (pos >= 4 && buf[3] == 0x01) {
          if (this->ic_status_sensor_) this->ic_status_sensor_->publish_state(buf[3]);
        }
        if (!this->tx_queue_.empty() && std::get<0>(this->tx_queue_.front()) == PACKET_TYPE_IC)
          this->tx_queue_.pop();
        return true;
      }
    }
  }
  if (len >= 64) { ESP_LOGW(TAG, "IC Clearing Buffer, size: %d", len); return true; }
  return false;
}

bool PentairIfIcComponent::validate_if_received_message_() {
  uint32_t at = this->rx_buffer_.size() - 1;
  uint8_t *data = &this->rx_buffer_[0];
  if (at == 0) return data[0] == 0xFF;
  if (at == 1) return data[1] == 0x00;
  if (at == 2) return data[2] == 0xFF;
  if (at == 3) return data[3] == 0xA5;
  if (at <= 8) return true;
  uint8_t packet_size = data[8];
  if (at < (uint32_t)(packet_size + 10)) return true;
  uint16_t checksum = 0;
  for (int j = 3; j < 3 + packet_size + 6; j++) checksum += data[j];
  uint16_t pcs = (data[3 + 6 + packet_size] << 8) + data[3 + 7 + packet_size];
  if (checksum != pcs) { ESP_LOGW(TAG, "IF CHECKSUM MISMATCH"); return false; }
  rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + 3);
  ESP_LOGI(TAG, "IF Package received: %s", format_hex_pretty(rx_buffer_).c_str());
  parse_if_packet_(rx_buffer_);
  return false;
}

void PentairIfIcComponent::parse_if_packet_(const std::vector<uint8_t> &data) {
  if (data[3] == 0x60 && data[4] == 0x07) {
    if (this->if_running_ != nullptr) {
      switch (data[6]) {
        case STOPPED:
          this->if_running_->publish_state(false);
          if (this->pump_run_switch_) this->pump_run_switch_->publish_state(false);
          break;
        case RUNNING:
          this->if_running_->publish_state(true);
          if (this->pump_run_switch_) this->pump_run_switch_->publish_state(true);
          break;
        default: ESP_LOGW(TAG, "IF Unknown running value %02x", data[6]); break;
      }
    }
    if (this->if_program_ != nullptr) {
      switch (data[7]) {
        case NO_PROG:    this->if_program_->publish_state(""); break;
        case LOCAL1:     this->if_program_->publish_state("Local 1"); break;
        case LOCAL2:     this->if_program_->publish_state("Local 2"); break;
        case LOCAL3:     this->if_program_->publish_state("Local 3"); break;
        case LOCAL4:     this->if_program_->publish_state("Local 4"); break;
        case EXT1:       this->if_program_->publish_state("External 1"); break;
        case EXT2:       this->if_program_->publish_state("External 2"); break;
        case EXT3:       this->if_program_->publish_state("External 3"); break;
        case EXT4:       this->if_program_->publish_state("External 4"); break;
        case TIMEOUT:    this->if_program_->publish_state("Time Out"); break;
        case PRIMING:    this->if_program_->publish_state("Priming"); break;
        case QUICKCLEAN: this->if_program_->publish_state("Quick Clean"); break;
        default: ESP_LOGW(TAG, "IF Unknown program value %02x", data[7]); break;
      }
    }
    if (this->if_power_)          this->if_power_->publish_state((data[9] * 256) + data[10]);
    if (this->if_rpm_)            this->if_rpm_->publish_state((data[11] * 256) + data[12]);
    if (this->if_flow_)           this->if_flow_->publish_state(data[13] * 0.227);
    if (this->if_pressure_)       this->if_pressure_->publish_state(data[14] / 14.504);
    if (this->if_time_remaining_) this->if_time_remaining_->publish_state(data[17] * 60 + data[18]);
    if (this->if_clock_)          this->if_clock_->publish_state(data[19] * 60 + data[20]);
  }
}

void PentairIfIcComponent::requestPumpStatus() {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x07, 0x00};
  queue_if_packet_(p, 6);
}
void PentairIfIcComponent::pumpToLocalControl() {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x04, 0x01, 0x00};
  queue_if_packet_(p, 7);
}
void PentairIfIcComponent::pumpToRemoteControl() {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x04, 0x01, 0xFF};
  queue_if_packet_(p, 7);
}
void PentairIfIcComponent::run() {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x06, 0x01, 0x0A};
  queue_if_packet_(p, 7);
}
void PentairIfIcComponent::stop() {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x06, 0x01, 0x04};
  queue_if_packet_(p, 7);
}
void PentairIfIcComponent::commandLocalProgram(int prog) {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x05, 0x01, 0};
  p[6] = prog + 1;
  queue_if_packet_(p, 7);
}
void PentairIfIcComponent::commandExternalProgram(int prog) {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x01, 0x04, 0x03, 0x21, 0x00, 0x00};
  p[9] = prog * 8;
  queue_if_packet_(p, 10);
}
void PentairIfIcComponent::saveValueForProgram(int prog, int value) {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x01, 0x04, 0x03, 0, 0, 0};
  p[7] = 0x26 + prog; p[8] = floor(value / 256); p[9] = value % 256;
  queue_if_packet_(p, 10);
}
void PentairIfIcComponent::commandRPM(int rpm) {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x01, 0x04, 0x02, 0xC4, 0, 0};
  p[8] = floor(rpm / 256); p[9] = rpm % 256;
  queue_if_packet_(p, 10);
}
void PentairIfIcComponent::commandFlow(int flow) {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x09, 0x04, 0x02, 0xC4, 0x00, 0};
  p[9] = flow;
  queue_if_packet_(p, 10);
}
void PentairIfIcComponent::setPumpClock(int hour, int minute) {
  uint8_t p[] = {0xA5, 0x00, 0x60, 0x10, 0x03, 0x02, 0, 0};
  p[6] = hour; p[7] = minute;
  queue_if_packet_(p, 8);
}

void PentairIfIcComponent::queue_if_packet_(uint8_t message[], int messageLength) {
  int checksum = 0;
  for (int j = 0; j < messageLength; j++) checksum += message[j];
  std::vector<uint8_t> packet = {0xFF, 0x00, 0xFF};
  packet.insert(packet.end(), message, message + messageLength);
  packet.push_back(checksum >> 8);
  packet.push_back(checksum & 0xFF);
  int sz = messageLength + 5;
  int pc = (packet[sz - 2] * 256) + packet[sz - 1];
  int db = 0;
  for (int i = 3; i < sz - 2; i++) db += packet[i];
  if (pc != db) { ESP_LOGW(TAG, "IF Asking to queue malformed packet"); return; }
  this->tx_queue_.push(std::make_tuple(PACKET_TYPE_IF, (uint8_t)0, (uint8_t)0, packet));
}

template<typename... Args>
std::string PentairIfIcComponent::string_format_(const std::string &format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  if (size_s <= 0) return {};
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

}  // namespace pentair_if_ic
}  // namespace esphome
