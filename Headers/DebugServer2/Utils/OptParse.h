//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Utils_OptParse_h
#define __DebugServer2_Utils_OptParse_h

#include "DebugServer2/Types.h"

#include <map>
#include <string>
#include <vector>

namespace ds2 {

class OptParse {
public:
  enum OptionType {
    boolOption,
    stringOption,
    vectorOption,
  };

public:
  void addOption(OptionType type, std::string const &name, char shortName,
                 std::string const &help = std::string(), bool hidden = false,
                 std::string const &envVarName = std::string());

public:
  int parse(int argc, char **argv, EnvironmentBlock const &env);

public:
  bool getBool(std::string const &name);
  std::string const &getString(std::string const &name);
  std::vector<std::string> const &getVector(std::string const &name);

public:
  void usageDie(std::string const &message = std::string());

private:
  void UpdateOptionsFromEnvVars(EnvironmentBlock const &env);

private:
  struct OptionStorage {
    char shortName;
    OptionType type;
    struct {
      bool boolValue;
      std::string stringValue;
      std::vector<std::string> vectorValue;
    } values;
    std::string help;
    bool hidden;
    std::string envVarName;
    bool isSet;

    void setBool(bool value) {
      isSet = true;
      values.boolValue = value;
    }

    void setString(std::string value) {
      isSet = true;
      values.stringValue = value;
    }

    void addToVector(std::string const &value) {
      isSet = true;
      values.vectorValue.push_back(value);
    }
  };

  typedef std::map<std::string, OptionStorage> OptionCollection;

  OptionCollection _options;

  OptionCollection::iterator findShortOpt(char shortOption);
  OptionStorage const &get(std::string const &name, OptionType type);
};
}

#endif // !__DebugServer2_Utils_OptParse_h
