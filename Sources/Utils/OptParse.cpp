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
#include <cstring>

namespace ds2 {

void OptParse::addOption(OptionType type, std::string const &name,
                         char shortName, std::string const &help, bool hidden) {
  DS2ASSERT(_options.find(name) == _options.end());
  DS2ASSERT(findShortOpt(shortName) == _options.end());
  _options[name] = {shortName, type, {false, "", {}}, help, hidden};
}

void OptParse::addPositional(std::string const &name, std::string const &help,
                             bool hidden) {
  DS2ASSERT(_positionals.find(name) == _positionals.end());
  _positionals[name] = {"", help, hidden};
}

int OptParse::parse(int argc, char **argv) {
#define CHECK(COND, ...)                                                       \
  do {                                                                         \
    if (!(COND)) {                                                             \
      usageDie(__VA_ARGS__);                                                   \
    }                                                                          \
  } while (0)

  // Skip argv[0] which contains the program name, and argv[1] which contains
  // the run mode.
  int idx = 2;

  // Save the run mode for potentially displaying it in `usageDie()`.
  _runMode = argv[1];

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
      auto nextPositional =
          std::find_if(_positionals.begin(), _positionals.end(),
                       [](PositionalCollection::value_type &e) {
                         return e.second.value.empty();
                       });

      if (nextPositional != _positionals.end()) {
        nextPositional->second.value = argv[idx];
      } else {
        break;
      }
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

std::string const &OptParse::getPositional(std::string const &name) const {
  DS2ASSERT(_positionals.find(name) != _positionals.end());
  return _positionals.at(name).value;
}

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
  // `parse()` fills in this information, and we shouldn't call this function
  // without having started to parse arguments.
  DS2ASSERT(!_runMode.empty());

  if (format != nullptr) {
    va_list ap;

    Print("error: ");
    va_start(ap, format);
    VPrint(format, ap);
    va_end(ap);
    Print("\n");
  }

  static std::map<OptionType, std::string> const argTypePlaceholder = {
      {stringOption, "ARG"},
      {vectorOption, "ARG..."},
      {boolOption, ""},
  };

  size_t helpAlign = 0;
  auto optionPrintLength = [](std::string const &name, OptionType type) {
    return ::strlen("-?, --") + name.length() + ::strlen(" ") +
           argTypePlaceholder.at(type).length();
  };
  auto positionalPrintLength = [](std::string const &name) {
    return name.length();
  };

  Print("usage: ds2 %s [OPTIONS]", _runMode.c_str());
  for (const auto &e : _positionals) {
    Print(" %s", e.first.c_str());
    helpAlign = std::max(helpAlign, positionalPrintLength(e.first));
  }
  Print(" %s\n", "[-- PROGRAM [ARGUMENTS...]]");

  for (const auto &e : _options) {
    helpAlign = std::max(helpAlign, optionPrintLength(e.first, e.second.type));
  }

  helpAlign += 2;

  for (auto const &e : _options) {
    if (e.second.hidden) {
      continue;
    }

    Print("  -%c, --%s %s", e.second.shortName, e.first.c_str(),
          argTypePlaceholder.at(e.second.type).c_str());
    for (size_t i = optionPrintLength(e.first, e.second.type); i < helpAlign;
         ++i) {
      Print(" ");
    }
    Print("%s\n", e.second.help.c_str());
  }

  for (auto const &e : _positionals) {
    if (e.second.hidden) {
      continue;
    }

    Print("  %s", e.first.c_str());
    for (size_t i = positionalPrintLength(e.first); i < helpAlign; ++i) {
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
