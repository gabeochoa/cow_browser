#ifndef PARSER
#define PARSER

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

bool is_space(char c) { return isspace(c); }
bool is_not_space(char c) { return !isspace(c); }
bool is_alpha(char c) { return isalpha(c); }
bool is_not_alpha(char c) { return !isalpha(c); }
bool is_not_lt(char c) { return c != '<'; }
bool is_quote(char c) { return (c == '"' || c == '\''); }
bool is_not_quote(char c) { return !is_quote(c); }
bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}
struct Parser {
  int position;
  std::string input;
  Parser(std::string i) : position(0), input(i) {}

  // peek the next character
  char next_character() {
    char c = input[position];
    // std::cout << "peek " << c << std::endl;
    return c;
  }

  // Do the next characters start with the given string?
  bool starts_with(std::string prefix) {
    auto found = std::mismatch(prefix.begin(), prefix.end(),
                               input.substr(position).begin());
    return found.first == prefix.end();
  }

  bool is_eof() { return this->position > this->input.size(); }

  char consume_next_character() {
    auto c = this->next_character();
    // std::cout << "consume (" << c << ")" << std::endl;
    this->position += 1;
    return c;
  }

  std::string consume_until(const std::function<bool(char)> &predicate) {
    // TODO replace with string builder
    std::string token;
    for (;;) {
      if (this->is_eof() || predicate(input[this->position])) {
        break;
      }
      token += this->consume_next_character();
    }
    // std::cout << "token " << token << std::endl;
    return token;
  }

  std::string consume_until_space() {
    // std::cout << "consume until space" << std::endl;
    return this->consume_until(&is_space);
  }
  std::string consume_spaces() {
    // std::cout << "consume spaces" << std::endl;
    return this->consume_until(&is_not_space);
  }
  std::string consume_tag() {
    // std::cout << "consume tag" << std::endl;
    return this->consume_until(&is_not_alpha);
  }
};

#endif
