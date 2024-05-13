#pragma once

#include "FST.hh"

namespace sta {

class StaState;

FST readFSTFile(const char *filename, StaState *sta);

} //namespace