//
//
//

#ifndef BWLIST_HPP
#define BWLIST_HPP

#include <regex>

#include <string>

#include <fstream>

#include <vector>

class BWList {
public:
  enum class ListMode : int { CONJUNCTIVE, DISJUNCTIVE };

  explicit BWList(ListMode mode = ListMode::DISJUNCTIVE) : m_Mode{mode} {
    return;
  }

  bool addRegex(std::ifstream &file) {
    if (!file.is_open())
      return false;

    std::string pattern;
    while (!file) {
      file >> pattern;
      addRegex(pattern);
    }

    return file.eof() || file.good();
  }

  bool addRegex(const char *pattern) {
    m_Patterns.emplace_back(pattern);
    return true;
  }

  bool addRegex(const std::string &pattern) {
    addRegex(pattern.c_str());
    return true;
  }

  bool match(const char *target) {
    return ListMode::DISJUNCTIVE == m_Mode ? match_any(target)
                                           : match_all(target);
  }

  bool match(const std::string &target) { return match(target.c_str()); }

private:
  bool match_any(const char *target) {
    std::cmatch match;

    for (const auto &pat : m_Patterns)
      if (std::regex_match(target, match, pat))
        return true;

    return false;
  }

  bool match_all(const char *target) {
    std::cmatch match;

    for (const auto &pat : m_Patterns)
      if (!std::regex_match(target, match, pat))
        return false;

    return true;
  }

  const ListMode m_Mode;
  std::vector<std::regex> m_Patterns;
};

#endif // ifndef BWLIST_HPP
