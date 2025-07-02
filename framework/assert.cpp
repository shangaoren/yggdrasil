#include "assert.hpp"
#include "../YggdrasilConfig.hpp"

namespace kernel {
void Assertion::stop(){
  return Core::breakpoint();
  }
}