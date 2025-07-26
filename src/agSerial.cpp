/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifdef ARDUINO
#ifndef ESP8266

#include "agSerial.h"
#include "agLogger.h"

#define MAX_RETRY_IICSERIAL_UART_INIT 3

AgSerial::AgSerial(TwoWire &wire) : _wire(wire) {}

AgSerial::~AgSerial() {
  if (iicSerial_ != nullptr) {
    delete iicSerial_;
    iicSerial_ = nullptr;
  }
}

void AgSerial::init(int iicResetIO) {
  if (iicSerial_ != nullptr) {
    // already initialized
    AG_LOGI(TAG, "IICSerial Already initialized");
    return;
  }

  // init iic serial
  _iicResetIO = static_cast<gpio_num_t>(iicResetIO);
  gpio_reset_pin(_iicResetIO); // IIC-UART reset
  gpio_set_direction(_iicResetIO, GPIO_MODE_OUTPUT);
  gpio_set_level(_iicResetIO, 1);
  iicSerial_ = new DFRobot_IICSerial(_wire, SUBUART_CHANNEL_1, 1, 1);
  AG_LOGI(TAG, "IICSerial initialized");
}

bool AgSerial::open(int baud) {
  if (_atLineOpened) {
    AG_LOGI(TAG, "IICSerial already opened");
    return true;
  }

  int counter = 0;
  do {
    if (iicSerial_->begin(baud) == 0) {
      _atLineOpened = true;
      break;
    }

    AG_LOGW(TAG, "IICSerial failed open serial line, retry..");
    counter++;
    delay(500);
  } while (counter < MAX_RETRY_IICSERIAL_UART_INIT);

  if (!_atLineOpened) {
    AG_LOGE(TAG, "IICSerial failed open serial line, give up..");
    return false;
  }

  AG_LOGI(TAG, "IICSerial success open serial line");
  return true;
}

void AgSerial::close() {
  if (!_atLineOpened) {
    AG_LOGI(TAG, "IICSerial line already closed");
    return;
  }

  iicSerial_->flush();
  iicSerial_->end();
  _atLineOpened = false;

  // TODO: prepare sleep?
}

void AgSerial::setDebug(bool enable) { _debug = enable; }

bool AgSerial::available() { return (iicSerial_->available() > 0); }

void AgSerial::print(const char *str) {
  if (_debug) {
    Serial.print(str); // TODO: Change to idf compatiblee
  }
  iicSerial_->print(str);
}

uint8_t AgSerial::read() {
  if (_debug) {
    char b = iicSerial_->read();
    Serial.write(b); // TODO: Change to idf compatiblee
    return b;
  }

  return iicSerial_->read();
}

#endif // ESP8266
#endif // ARDUINO
