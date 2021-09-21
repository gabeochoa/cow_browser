#ifndef CSS_PARSER_HPP
#define CSS_PARSER_HPP

#include <fstream>
#include <optional>
#include <variant>
#include <vector>

#include "html_parser.cpp"
#include "parser.cpp"

enum Unit {
  px,
  em,
};

struct Color {
  int r, g, b, a;

  friend std::ostream &operator<<(std::ostream &os, const Color &c) {
    os << "Color: ";
    os << "(";
    os << c.r << "," << c.g << "," << c.b << "," << c.a;
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
struct EdgeSize {
  int left = 0, right = 0, top = 0, bottom = 0;
};

struct Rect {
  int x = 0, y = 0, width = 0, height = 0;

  Rect expanded_by(EdgeSize edge) {
    return Rect{
        this->x - edge.left,
        this->y - edge.top,
        this->width + edge.left + edge.right,
        this->height + edge.top + edge.bottom,
    };
  }

  friend std::ostream &operator<<(std::ostream &os, const Rect &r) {
    os << "Rect(" << r.x << "," << r.y << "," << r.width << "," << r.height
       << ")";
    return os;
  }
};

struct Dimensions {
  Rect content;
  EdgeSize padding;
  EdgeSize margin;
  EdgeSize border;

  Rect padding_box() { return this->content.expanded_by(this->padding); }

  Rect border_box() { return this->padding_box().expanded_by(this->border); }
  Rect margin_box() { return this->border_box().expanded_by(this->margin); }
};

enum BoxType { b_BLOCK, b_INLINE, b_ANON };
enum DisplayType { BLOCK, INLINE, NONE };
typedef std::map<std::string, DeclarationValueType> PropertyMap;

struct StyledNode {
  Node node;
  PropertyMap values;
  std::vector<StyledNode> children;

  std::optional<DeclarationValueType> value(std::string name) {
    try {
      return values.at(name);
    } catch (const std::exception &e) {
      return {};
    }
  }

  DisplayType display() {
    std::string display = "inline";
    try {
      auto value = this->values.at("display");
      display = std::get<std::string>(value);
    } catch (const std::exception &e) {
    }

    if (display == "block") {
      return DisplayType::BLOCK;
    } else if (display == "none") {
      return DisplayType::NONE;
    }
    return DisplayType::INLINE;
  }
};

struct LayoutBox {
  Dimensions dims;
  BoxType type;
  std::vector<LayoutBox> children;

  LayoutBox get_inline_container() {
    switch (type) {
    case BoxType::b_INLINE:
    case BoxType::b_ANON:
      return *this;
    case BoxType::b_BLOCK:
      LayoutBox last_child = this->children.back();
      if (last_child.type == BoxType::b_ANON) {
        // do nothing
      } else {
        this->children.push_back(newbox(BoxType::b_ANON));
      }
      break;
    }
    return this->children.back();
  }

  LayoutBox newbox(BoxType type) {
    LayoutBox b;
    b.type = type;
    return b;
  }

  void layout(Dimensions container) {
    std::cout << "todo this is where id do the layout" << std::endl;
    // if (this->type == BoxType::BLOCK) {
    // this->layout_block(container);
    // }
    // TODO other box types;
  }
};

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

    struct {
      bool operator()(Selector a, Selector b) const {
        // count number of id selectors
        int a_a = a.id.size();
        int a_b = b.id.size();
        if (a_a != a_b) {
          return a_a < a_b;
        }
        // count number of class selectors
        int b_a = a.classes.size();
        int b_b = b.classes.size();
        if (b_a != b_b) {
          return b_a < b_b;
        }
        // count number of type
        int c_a = a.name.size();
        int c_b = b.name.size();
        if (c_a != c_b) {
          return b_a < b_b;
        }
        // ignore universal
        return true;
      }
    } specificity;
    std::sort(selectors.begin(), selectors.end(), specificity);

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
        // std::cout << "is eof " << std::endl;
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

bool matches_selector(ElementNode node, Selector s) {

  bool type_matches = s.name == node.name;
  if (!type_matches) {
    return false;
  }

  bool id_matches = false;
  try {
    id_matches = s.id == node.id();
  } catch (const std::exception &e) {
  }
  if (!id_matches) {
    return false;
  }

  auto selector_classes = s.classes;
  auto element_class_set = node.classes();
  auto class_matches = std::any_of(
      selector_classes.begin(), selector_classes.end(),
      [&](std::string c) { return element_class_set.count(c) == 1; });
  if (!class_matches) {
    return false;
  }

  return true;
}

std::optional<Rule> matched_rule(ElementNode elem, Rule rule) {
  for (auto selector : rule.selectors) {
    bool m = matches_selector(elem, selector);
    if (m) {
      return rule;
    }
  }
  return {};
}

std::vector<Rule> matching_rules(ElementNode elem, StyleSheet sheet) {
  std::vector<Rule> matched;
  std::copy_if(sheet.rules.begin(), sheet.rules.end(),
               std::back_inserter(matched),
               [&](Rule r) { return matched_rule(elem, r); });
  return matched;
}

PropertyMap specified_values(ElementNode elem, StyleSheet sheet) {
  // TODO also include any directly added style tag
  // <p style="color: red"> hi </p>
  PropertyMap values;
  std::vector<Rule> rules = matching_rules(elem, sheet);
  // TODO sort rules by highest specificity
  for (Rule rule : rules) {
    for (Declaration decl : rule.declarations) {
      values[decl.name] = decl.value;
    }
  }
  return values;
}

BoxType display_to_box_type(DisplayType d) {
  switch (d) {
  case DisplayType::BLOCK:
    return BoxType::b_BLOCK;
  case DisplayType::INLINE:
    return BoxType::b_INLINE;
  case DisplayType::NONE:
    return BoxType::b_ANON;
  }
}

LayoutBox build_layout_tree(StyledNode styled_node) {
  LayoutBox root;

  DisplayType display = styled_node.display();
  if (display == DisplayType::NONE) {
    std::cout << "Root node has display:none" << std::endl;
    assert(false);
  }
  root.type = display_to_box_type(display);

  for (auto child : styled_node.children) {
    display = child.display();
    if (display == DisplayType::BLOCK) {
      root.children.push_back(build_layout_tree(child));
    } else if (display == DisplayType::INLINE) {
      auto container = root.get_inline_container();
      container.children.push_back(build_layout_tree(child));
    } else {
      // display none, dont render them
    }
  }
  return root;
}

StyleSheet parse_css(std::string input) {
  auto parser = CSSParser(input);
  auto sheet = parser.parse_sheet();
  return sheet;
}
#endif
