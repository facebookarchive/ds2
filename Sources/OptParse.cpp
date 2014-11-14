//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Log.h"
#include "DebugServer2/OptParse.h"

#include <algorithm>
#include <cstdlib>

namespace ds2 {

void OptParse::addOption(OptionType type, std::string const &name,
                         char shortName, std::string const &help, bool hidden) {
  DS2ASSERT(_options.find(name) == _options.end());
  DS2ASSERT(findShortOpt(shortName) == _options.end());
  _options[name] = {shortName, type, "", help, hidden};
}

int OptParse::parse(int argc, char **argv) {
#define CHECK(COND)                                                            \
  do {                                                                         \
    if (!(COND)) {                                                             \
      usageDie();                                                              \
    }                                                                          \
  } while (0)
  int idx;

  // Skip argv[0] which contains the program name.
  idx = 1;

  while (idx < argc) {
    if (argv[idx][0] == '-' && argv[idx][1] == '-') {
      // Long option.
      auto it = _options.find(argv[idx] + 2);
      CHECK(it != _options.end());

      if (it->second.type == boolOption) {
        it->second.value = "true";
      } else {
        CHECK(idx + 1 < argc);
        it->second.value = argv[++idx];
      }
    } else if (argv[idx][0] == '-') {
      // Short option.
      CHECK(argv[idx][1] != '\0');
      for (int optidx = 1; argv[idx][optidx] != '\0'; ++optidx) {
        auto it = findShortOpt(argv[idx][optidx]);
        CHECK(it != _options.end());
        if (it->second.type == boolOption) {
          it->second.value = "true";
        } else {
          if (argv[idx][optidx + 1] != '\0') {
            it->second.value = argv[idx] + optidx + 1;
          } else {
            CHECK(idx + 1 < argc);
            it->second.value = argv[++idx];
          }
          break;
        }
      }
    } else {
      // End of options.
      break;
    }

    ++idx;
  }

  return idx;
#undef CHECK
}

bool OptParse::getBool(std::string const &name) {
  return !get(name, boolOption).empty();
}

std::string const &OptParse::getString(std::string const &name) {
  return get(name, stringOption);
}

void OptParse::usageDie(std::string const &message) {
  if (!message.empty()) {
    fprintf(stderr, "error: %s\n", message.c_str());
  }

  fprintf(stderr, "usage: %s [OPTIONS] [PROGRAM [ARGUMENTS...]]\n", "ds2");

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

    fprintf(stderr, "  -%c, --%s", e.second.shortName, e.first.c_str());
    fprintf(stderr, " %s", (e.second.type == stringOption ? "ARG" : "   "));
    for (size_t i = 0; i < help_align - e.first.size(); ++i) {
      fprintf(stderr, " ");
    }
    fprintf(stderr, "%s\n", e.second.help.c_str());
  }

  exit(EXIT_FAILURE);
}

std::string const &OptParse::get(std::string const &name, OptionType type) {
  DS2ASSERT(_options.find(name) != _options.end());
  DS2ASSERT(_options[name].type == type);
  return _options[name].value;
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
