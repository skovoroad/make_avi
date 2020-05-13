#pragma once
#include <exception>

namespace BuildAvi {
  class AviException : public std::exception {
    const char * reason_;
  public:
    explicit AviException(const char *reason)
      : reason_(reason) 
    {}

    const char* what() const noexcept override {
      return reason_;
    }
  };
}
