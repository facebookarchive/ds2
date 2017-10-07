//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Utils/OptParse.h"
#include "DebugServer2/Utils/Log.h"
#include "DebugServer2/Utils/String.h"

#include <algorithm>
#include <cstdlib>

namespace ds2 {

void OptParse::addOption(OptionType type, std::string const &name,
                         char shortName, std::string const &help, bool hidden) {
  DS2ASSERT(_options.find(name) == _options.end());
  DS2ASSERT(findShortOpt(shortName) == _options.end());
  _options[name] = {shortName, type, {false, "", {}}, help, hidden};
}

int OptParse::parse(int argc, char **argv) {
#define CHECK(COND, ...)                                                       \
  do {                                                                         \
    if (!(COND)) {                                                             \
      usageDie(__VA_ARGS__);                                                   \
    }                                                                          \
  } while (0)

  // Skip argv[0] which contains the program name,
  // and argv[1] which contains the run mode
  int idx = 2;

  while (idx < argc) {
    if (argv[idx][0] == '-' && argv[idx][1] == '-') {
      std::string argStr(argv[idx]);
      // program name may be preceeded by "--"
      if (argStr == "--") {
        ++idx;
        break;
      }

      argStr = argStr.substr(2);

      std::string argVal;
      auto split = argStr.find("=");
      if (split != std::string::npos) {
        argVal = argStr.substr(split + 1);
        argStr = argStr.substr(0, split);
      }

      // Long option.
      auto it = _options.find(argStr);
      CHECK(it != _options.end(), "unrecognized option `%s'", argv[idx]);

      if (it->second.type != boolOption && argVal.empty()) {
        CHECK(idx + 1 < argc, "option `%s' expects an argument", argv[idx]);
        argVal = std::string(argv[++idx]);
      }

      switch (it->second.type) {
      case boolOption:
        it->second.values.boolValue = true;
        break;
      case stringOption:
        it->second.values.stringValue = argVal;
        break;
      case vectorOption:
        it->second.values.vectorValue.push_back(argVal);
        break;
      }
    } else if (argv[idx][0] == '-') {
      // Short option.
      CHECK(argv[idx][1] != '\0', "unrecognized option `-'");
      for (int optidx = 1; optidx > 0 && argv[idx][optidx] != '\0'; ++optidx) {
        auto it = findShortOpt(argv[idx][optidx]);
        CHECK(it != _options.end(), "unrecognized option `%s'", argv[idx]);
        switch (it->second.type) {
        case boolOption:
          it->second.values.boolValue = true;
          break;
        case stringOption:
          if (argv[idx][optidx + 1] != '\0') {
            it->second.values.stringValue = argv[idx] + optidx + 1;
          } else {
            CHECK(idx + 1 < argc, "option `%s' expects an argument", argv[idx]);
            it->second.values.stringValue = argv[++idx];
            optidx = -1;
          }
          break;
        case vectorOption:
          if (argv[idx][optidx + 1] != '\0') {
            it->second.values.vectorValue.push_back(argv[idx] + optidx + 1);
          } else {
            CHECK(idx + 1 < argc, "option `%s' expects an argument", argv[idx]);
            it->second.values.vectorValue.push_back(argv[++idx]);
            optidx = -1;
          }
          break;
        }
      }
    } else {
      // We already have our [host]:port, this can only be the path to the
      // binary to run.
      if (!_port.empty())
        break;

      std::string addrString(argv[idx]);
      auto splitPos = addrString.rfind(":");

      // Argument is not an option (--long or -s) and not a [host]:port. We're
      // done with parsing and this is the path to the binary to run.
      if (splitPos == std::string::npos)
        break;

      if (splitPos > 0) {
        // IPv6 addresses can be of the form '[a:b:c:d::1]:12345', so we need
        // to strip the square brackets around the host part.
        if (addrString[0] == '[' && addrString[splitPos - 1] == ']') {
          _host = addrString.substr(1, splitPos - 2);
        } else {
          _host = addrString.substr(0, splitPos);
        }
      }

      _port = addrString.substr(splitPos + 1);
    }

    ++idx;
  }

  return idx;
#undef CHECK
}

bool OptParse::getBool(std::string const &name) const {
  return get(name, boolOption).values.boolValue;
}

std::string const &OptParse::getString(std::string const &name) const {
  return get(name, stringOption).values.stringValue;
}

std::vector<std::string> const &
OptParse::getVector(std::string const &name) const {
  return get(name, vectorOption).values.vectorValue;
}

std::string const &OptParse::getHost() const { return _host; }

std::string const &OptParse::getPort() const { return _port; }

static void VPrint(char const *format, va_list ap) {
  va_list ap_copy;

  va_copy(ap_copy, ap);
  std::vector<char> line(ds2::Utils::VSNPrintf(nullptr, 0, format, ap_copy) +
                         1);
  va_end(ap_copy);

  va_copy(ap_copy, ap);
  ds2::Utils::VSNPrintf(line.data(), line.size(), format, ap_copy);
  va_end(ap_copy);

  fputs(line.data(), stderr);
#if defined(OS_WIN32)
  OutputDebugStringA(line.data());
#endif
}

static void Print(char const *format, ...) {
  va_list ap;

  va_start(ap, format);
  VPrint(format, ap);
  va_end(ap);
}

void OptParse::usageDie(char const *format, ...) {
  if (format != nullptr) {
    va_list ap;

    Print("error: ");
    va_start(ap, format);
    VPrint(format, ap);
    va_end(ap);
    Print("\n");
  }

  Print("usage: ds2 RUN_MODE [OPTIONS] %s%s\n", "[HOST]:PORT ",
        "[-- PROGRAM [ARGUMENTS...]]");

  static std::map<OptionType, std::string> const argTypePlaceholder = {
      {stringOption, "ARG"}, {vectorOption, "ARG..."}, {boolOption, ""},
  };

  size_t help_align = 0;

  std::for_each(_options.begin(), _options.end(),
                [&help_align](OptionCollection::value_type const &arg) {
                  size_t argLength =
                      arg.first.size() + 1 +
                      argTypePlaceholder.at(arg.second.type).size();
                  if (argLength > help_align) {
                    help_align = argLength;
                  }
                });

  help_align += 2;

  for (auto const &e : _options) {
    if (e.second.hidden) {
      continue;
    }

    Print("  -%c, --%s %s", e.second.shortName, e.first.c_str(),
          argTypePlaceholder.at(e.second.type).c_str());
    for (size_t i =
             e.first.size() + 1 + argTypePlaceholder.at(e.second.type).size();
         i < help_align; ++i) {
      Print(" ");
    }
    Print("%s\n", e.second.help.c_str());
  }

  ::exit(EXIT_FAILURE);
}

OptParse::OptionStorage const &OptParse::get(std::string const &name,
                                             OptionType type) const {
  DS2ASSERT(_options.find(name) != _options.end());
  DS2ASSERT(_options.at(name).type == type);
  return _options.at(name);
}

std::map<std::string, OptParse::OptionStorage>::iterator
OptParse::findShortOpt(char shortOption) {
  return std::find_if(
      _options.begin(), _options.end(),
      [shortOption](std::pair<std::string, OptionStorage> const &arg) {
        return arg.second.shortName == shortOption;
      });
}
} // namespace ds2
