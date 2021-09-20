
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include "base_window.hpp"
#include "html_parser.hpp"

#ifndef CSS_PARSER_HPP
#define CSS_PARSER_HPP

#include <variant>
#include <vector>

enum Unit {
  px,
  em,
};

struct Color {
  int r, g, b, a;

  friend std::ostream &operator<<(std::ostream &os, const Color &c) {
    os << "Color: ";
    os << "(";
    os << c.r;
    os << ",";
    os << c.g;
    os << ",";
    os << c.b;
    os << ",";
    os << c.a;
    os << ")";
    return os;
  }
};

struct Length {
  float num;
  Unit unit;

  friend std::ostream &operator<<(std::ostream &os, const Length &l) {
    os << "Length : ";
    os << l.num;
    // TODO add unit
    return os;
  }
};

struct Selector {
  std::string name;
  std::string id;
  std::vector<std::string> classes;

  friend std::ostream &operator<<(std::ostream &os, const Selector &s) {
    os << "Selector: ";
    os << s.name;
    os << "(" << s.id << ")";
    os << " : ";
    os << " \n";
    for (std::string child : s.classes) {
      os << child << std::endl;
    }
    return os;
  }
};

typedef std::variant<std::string, int, Color, Length> DeclarationValueType;
struct Declaration {
  std::string name;
  DeclarationValueType value;

  friend std::ostream &operator<<(std::ostream &os, const Declaration &d) {
    os << "Declaration: ";
    os << d.name;
    os << " : ";
    std::visit([&](const auto &x) { os << x; }, d.value);
    os << " \n";
    return os;
  }
};

struct Rule {
  std::vector<Selector> selectors;
  std::vector<Declaration> declarations;

  friend std::ostream &operator<<(std::ostream &os, const Rule &r) {
    os << "Rule\n";
    for (Selector child : r.selectors) {
      os << child << std::endl;
    }
    for (Declaration child : r.declarations) {
      os << child << std::endl;
    }
    return os;
  }
};

struct StyleSheet {
  std::vector<Rule> rules;

  // Here's our overloaded operator<<
  friend std::ostream &operator<<(std::ostream &out, const StyleSheet &s) {
    return s.print(out);
  }

  virtual std::ostream &print(std::ostream &os) const {
    os << "StyleSheet\n";
    for (Rule child : this->rules) {
      os << child << std::endl;
    }
    return os;
  }
};

bool is_valid_id(char c) {
  // std::cout << "is valid id? " << c << std::endl;
  return isalnum(c) || c == '-' || c == '_';
}

struct CSSParser : public Parser {
  CSSParser(std::string i) : Parser(i) {}

  std::string parse_id() {
    return this->consume_until([](char c) { return !is_valid_id(c); });
  }

  // type#id.class1.class2.class3
  Selector parse_selector() {
    Selector selector;
    bool keep_running = true;
    while (!this->is_eof() && keep_running) {
      char next = this->next_character();
      switch (next) {
      case '#':
        this->consume_next_character();
        selector.id = this->parse_id();
        break;
      case '.':
        this->consume_next_character();
        selector.classes.push_back(this->parse_id());
        break;
      case '*':
        this->consume_next_character();
        break;
      default:
        bool valid = is_valid_id(next);
        if (!valid) {
          // std::cout << "not valid " << next << std::endl;
          keep_running = false;
          break;
        }
        selector.name = this->parse_id();
        break;
      }
    }
    // std::cout << "return selector " << std::endl;
    return selector;
  }

  std::vector<Selector> parse_selectors() {
    std::vector<Selector> selectors;
    bool is_running = true;
    while (is_running) {
      selectors.push_back(this->parse_selector());
      this->consume_spaces();
      char next = this->next_character();
      switch (next) {
      case ',':
        this->consume_next_character();
        this->consume_spaces();
        break;
      case '{':
        is_running = false;
        break;

      default:
        std::cout << "Something wrong with selector list: (" << (int)next << ")"
                  << std::endl;
        assert(false);
        break;
      }
    }
    // TODO sort by specificity
    return selectors;
  }

  float parse_float() {
    std::string f_as_str = this->consume_until([](char c) {
      // consume until not a number or dot
      return !(std::isdigit(c) || c == '.');
    });
    return std::atof(f_as_str.c_str());
  }

  Unit parse_unit() {
    std::string unit = this->consume_until(is_not_alpha);
    if (unit == "px") {
      return Unit::px;
    } else if (unit == "em") {
      return Unit::em;
    } else {
      // TODO unknown css unit?
      return Unit::px;
    }
  }

  Length parse_length() {
    Length l;
    l.num = this->parse_float();
    l.unit = this->parse_unit();
    return l;
  }

  int parse_hex_pair() {
    std::string hx_as_str = this->input.substr(0, 2);
    this->position += 2;
    int hx = std::stoul(hx_as_str, nullptr, 16);
    return hx;
  }

  Color parse_color() {
    Color c;
    c.r = this->parse_hex_pair();
    c.g = this->parse_hex_pair();
    c.b = this->parse_hex_pair();
    c.a = 255;
    return c;
  }

  std::string parse_keyword() { return this->parse_id(); }

  DeclarationValueType parse_value() {
    char next = this->next_character();
    if (isnumber(next)) {
      return this->parse_length();
    }
    if (next == '#') {
      return this->parse_color();
    }
    return this->parse_keyword();
  }

  Declaration parse_declaration() {
    Declaration declaration;
    char c;

    declaration.name = this->parse_id();
    this->consume_spaces();
    c = this->consume_next_character();
    assert(c == ':');
    this->consume_spaces();
    declaration.value = this->parse_value();
    this->consume_spaces();
    c = this->consume_next_character();
    assert(c == ';');

    return declaration;
  }

  std::vector<Declaration> parse_declarations() {
    std::vector<Declaration> declarations;
    char c = this->consume_next_character();
    assert(c == '{');
    for (;;) {
      this->consume_spaces();
      if (this->next_character() == '}') {
        this->consume_next_character();
        break;
      }
      declarations.push_back(this->parse_declaration());
    }
    // std::cout << "return declarations" << std::endl;
    return declarations;
  }

  Rule parse_rule() {
    Rule r;
    r.selectors = this->parse_selectors();
    r.declarations = this->parse_declarations();
    return r;
  }

  std::vector<Rule> parse_rules() {
    std::vector<Rule> rules;
    for (;;) {
      this->consume_spaces();
      // TODO why do we need to check for ascii 0?
      // eof doesnt catch?
      if (this->is_eof() || this->next_character() == 0) {
        std::cout << "is eof " << std::endl;
        break;
      }
      rules.push_back(this->parse_rule());
    }
    return rules;
  }

  StyleSheet parse_sheet() {
    StyleSheet sheet;
    sheet.rules = this->parse_rules();
    return sheet;
  }
};

StyleSheet parse_css(std::string input) {
  auto parser = CSSParser(input);
  auto sheet = parser.parse_sheet();
  return sheet;
}

#endif

void loop(GLFWwindow *window, Node *root) {
  ImGui::Begin("My name is window");
  ImGui::Text("im am text");
  ImGui::End();
}

int main() {
  std::ifstream css("example_html/index.css");
  std::stringstream cssbuffer;
  cssbuffer << css.rdbuf();
  StyleSheet sheet = parse_css(cssbuffer.str());
  std::cout << sheet << std::endl;
  return 0;

  GLFWwindow *window = init_window();
  if (window == nullptr) {
    std::cout << "Failed to init glfw window" << std::endl;
    return -1;
  }

  std::ifstream t("example_html/index.html");
  std::stringstream buffer;
  buffer << t.rdbuf();
  // TODO read directly?
  Node *root = parse_html(buffer.str());
  // std::cout << *root << std::endl;

  run_until_close(window, std::bind(loop, std::placeholders::_1, root));

  return 0;
}
