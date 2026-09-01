#ifndef WIRE_H
#define WIRE_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

class TwoWire {
 public:
  struct Transmission {
    uint8_t address;
    std::vector<uint8_t> data;
  };

  std::vector<Transmission> transmissions;
  std::vector<uint8_t> endTransmissionResults;
  uint8_t registers[256] = {};
  int requestFromResult = -1;

  void begin() {
  }

  void beginTransmission(uint8_t address) {
    _address = address;
    _outgoing.clear();
  }

  size_t write(uint8_t value) {
    _outgoing.push_back(value);
    return 1;
  }

  uint8_t endTransmission(bool stop = true) {
    (void)stop;
    transmissions.push_back({_address, _outgoing});
    uint8_t result = 0;
    if (!endTransmissionResults.empty()) {
      result = endTransmissionResults.front();
      endTransmissionResults.erase(endTransmissionResults.begin());
    }
    if (result != 0) {
      return result;
    }
    if (!_outgoing.empty()) {
      _registerPointer = _outgoing[0];
      for (size_t index = 1; index < _outgoing.size(); index++) {
        registers[_registerPointer++] = _outgoing[index];
      }
      if (_outgoing.size() == 1) {
        _registerPointer = _outgoing[0];
      }
    }
    return result;
  }

  uint8_t requestFrom(int address, int quantity) {
    (void)address;
    int result = requestFromResult < 0 ? quantity : requestFromResult;
    _remaining = result;
    return (uint8_t)result;
  }

  int read() {
    if (_remaining <= 0) {
      return -1;
    }
    _remaining--;
    return registers[_registerPointer++];
  }

 private:
  uint8_t _address = 0;
  uint8_t _registerPointer = 0;
  int _remaining = 0;
  std::vector<uint8_t> _outgoing;
};

extern TwoWire Wire;

#endif
