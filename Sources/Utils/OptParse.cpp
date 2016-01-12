//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/OptParse.h"

#include <algorithm>
#include <cstdlib>

namespace ds2 {

void OptParse::addOption(OptionType type, std::string const &name,
                         char shortName, std::string const &help, bool hidden) {
  DS2ASSERT(_options.find(name) == _options.end());
  DS2ASSERT(findShortOpt(shortName) == _options.end());
  _options[name] = {shortName, type, {false, "", {}}, help, hidden};
}

int OptParse::parse(int argc, char **argv, std::string &host, int &port,
                    bool slave) {
#define CHECK(COND)                                                            \
  do {                                                                         \
    if (!(COND)) {                                                             \
      usageDie();                                                              \
    }                                                                          \
  } while (0)

  int idx;

  host.clear();
  // Skip argv[0] which contains the program name,
  // and argv[1] which contains the run mode
  idx = 2;

  while (idx < argc) {
    if (argv[idx][0] == '-' && argv[idx][1] == '-') {
      // Long option.
      auto it = _options.find(argv[idx] + 2);
      CHECK(it != _options.end());

      switch (it->second.type) {
      case boolOption:
        it->second.values.boolValue = true;
        break;
      case stringOption:
        CHECK(idx + 1 < argc);
        it->second.values.stringValue = argv[++idx];
        break;
      case vectorOption:
        CHECK(idx + 1 < argc);
        it->second.values.vectorValue.push_back(argv[++idx]);
        break;
      }
    } else if (argv[idx][0] == '-') {
      // Short option.
      CHECK(argv[idx][1] != '\0');
      for (int optidx = 1; optidx > 0 && argv[idx][optidx] != '\0'; ++optidx) {
        auto it = findShortOpt(argv[idx][optidx]);
        CHECK(it != _options.end());
        switch (it->second.type) {
        case boolOption:
          it->second.values.boolValue = true;
          break;
        case stringOption:
          if (argv[idx][optidx + 1] != '\0') {
            it->second.values.stringValue = argv[idx] + optidx + 1;
          } else {
            CHECK(idx + 1 < argc);
            it->second.values.stringValue = argv[++idx];
            optidx = -1;
          }
          break;
        case vectorOption:
          if (argv[idx][optidx + 1] != '\0') {
            it->second.values.vectorValue.push_back(argv[idx] + optidx + 1);
          } else {
            CHECK(idx + 1 < argc);
            it->second.values.vectorValue.push_back(argv[++idx]);
            optidx = -1;
          }
          break;
        }
      }
    } else if (host.empty()) {
      std::string addrString(argv[idx]);
      auto splitPos = addrString.find(":");
      CHECK(splitPos != std::string::npos);
      if (!splitPos)
        host = "localhost";
      else
        host = addrString.substr(0, splitPos).c_str();

      port = atoi(addrString.substr(splitPos + 1).c_str());
    } else {
      // End of options.
      break;
    }

    ++idx;
  }

  CHECK(!host.empty() || slave);

  return idx;
#undef CHECK
}

bool OptParse::getBool(std::string const &name) {
  return get(name, boolOption).values.boolValue;
}

std::string const &OptParse::getString(std::string const &name) {
  return get(name, stringOption).values.stringValue;
}

std::vector<std::string> const &OptParse::getVector(std::string const &name) {
  return get(name, vectorOption).values.vectorValue;
}

void OptParse::usageDie(std::string const &message) {
  auto outStream = stderr;

  if (!message.empty()) {
    fprintf(outStream, "error: %s\n", message.c_str());
  }

  fprintf(outStream, "usage: %s [%s] [%s] [%s] [%s]\n", "ds2", "RUN_MODE",
          "OPTIONS", "[HOST]:PORT", "PROGRAM [ARGUMENTS...]");

  size_t help_align = 0;

  std::for_each(_options.begin(), _options.end(),
                [&help_align](OptionCollection::value_type const &arg) {
                  if (arg.first.size() > help_align) {
                    help_align = arg.first.size();
                  }
                });

  help_align += 2;

  for (auto const &e : _options) {
    if (e.second.hidden)
      continue;

    fprintf(outStream, "  -%c, --%s", e.second.shortName, e.first.c_str());
    fprintf(outStream, " %s", (e.second.type == stringOption ? "ARG" : "   "));
    for (size_t i = 0; i < help_align - e.first.size(); ++i) {
      fprintf(outStream, " ");
    }
    fprintf(outStream, "%s\n", e.second.help.c_str());
  }

  exit(EXIT_FAILURE);
}

OptParse::OptionStorage const &OptParse::get(std::string const &name,
                                             OptionType type) {
  DS2ASSERT(_options.find(name) != _options.end());
  DS2ASSERT(_options[name].type == type);
  return _options[name];
}

std::map<std::string, OptParse::OptionStorage>::iterator
OptParse::findShortOpt(char shortOption) {
  return std::find_if(
      _options.begin(), _options.end(),
      [shortOption](std::pair<std::string, OptionStorage> const &arg) {
        return arg.second.shortName == shortOption;
      });
}
}
