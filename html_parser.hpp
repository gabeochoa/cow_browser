#ifndef HTML_PARSER
#define HTML_PARSER

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

enum NodeType { Unknown = 0, Text, Element };
std::string print_type(const NodeType type) {
  switch (type) {
  case NodeType::Element:
    return "Element";
  case NodeType::Text:
    return "Text";
  case NodeType::Unknown:
  default:
    return "Unknown";
  }
}

typedef std::pair<std::string, std::string> Attribute;
typedef std::map<std::string, std::string> AttributeMap;

struct Node {
  NodeType type = NodeType::Unknown;
  std::vector<Node *> children;

  Node(NodeType t = NodeType::Unknown) : type(t) {}
  Node(NodeType t, std::vector<Node *> c) : type(t), children(c) {}

  virtual ~Node() {
    while (!children.empty())
      delete children.back(), children.pop_back();
  }

  // Here's our overloaded operator<<
  friend std::ostream &operator<<(std::ostream &out, const Node &n) {
    return n.print(out);
  }

  virtual std::ostream &print(std::ostream &os) const {
    os << "Node (Type: " << print_type(this->type) << ")";
    os << "\n";
    return os;
  }

  std::ostream &print_children(std::ostream &os) const {
    for (Node *child : this->children) {
      os << *child << std::endl;
    }
    return os;
  }
};

struct TextNode : public Node {
  std::string content;
  TextNode(std::string c) : Node(NodeType::Text), content(c) {}

  std::ostream &print(std::ostream &os) const override {
    Node::print(os);
    os << "TextNode (Type: " << this->type << ")\n";
    os << "content: " << this->content << "\n";
    return os;
  }
};

std::ostream &operator<<(std::ostream &os, const AttributeMap &a) {
  os << "Attributes\n";
  for (auto const &pair : a) {
    os << "{" << pair.first << ": " << pair.second << "}\n";
  }
  return os;
}

struct ElementNode : public Node {
  std::string name;
  AttributeMap attrs;

  ElementNode(std::string n, AttributeMap a)
      : Node(NodeType::Element), name(n), attrs(a) {}
  ElementNode(std::string n, AttributeMap a, std::vector<Node *> c)
      : Node(NodeType::Element, c), name(n), attrs(a) {}
  std::ostream &print(std::ostream &os) const override {
    Node::print(os);
    os << "ElementNode (Type: " << this->type << ")\n";
    os << "name: " << this->name << "\n";
    os << this->attrs << "\n";
    this->print_children(os);
    return os;
  }
};

TextNode *createText(std::string content) { return new TextNode(content); }

ElementNode *createElement(std::string name, AttributeMap attrs,
                           std::vector<Node *> children) {
  return new ElementNode(name, attrs, children);
}

bool is_space(char c) { return isspace(c); }
bool is_not_space(char c) { return !isspace(c); }
bool is_alpha(char c) { return isalpha(c); }
bool is_not_alpha(char c) { return !isalpha(c); }
bool is_not_lt(char c) { return c != '<'; }
bool is_quote(char c) { return (c == '"' || c == '\''); }
bool is_not_quote(char c) { return !is_quote(c); }

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

  Node *parse_text() {
    // std::cout << "parse text " << std::endl;
    auto is_lt = ([](char c) { return c == '<'; });
    std::string content = this->consume_until(is_lt);
    // std::cout << " parse content end (" << content << ")" << std::endl;
    return createText(content);
  }

  std::string parse_tag_name() {
    // std::cout << "parse tag name" << std::endl;
    auto is_not_tag_name = ([&](char ch) { return !isalnum(ch); });
    return this->consume_until(is_not_tag_name);
  }

  std::string parse_attribute_value() {
    // std::cout << "parse attribute value" << std::endl;
    char openq = this->consume_next_character();
    assert(is_quote(openq));
    auto is_matching_quote = ([&](char ch) { return ch == openq; });
    std::string value = this->consume_until(is_matching_quote);
    char closeq = this->consume_next_character();
    // TODO should check for close quote?
    assert(is_quote(closeq));
    return value;
  }

  Attribute parse_attribute() {
    // std::cout << "parse attribute" << std::endl;
    char c;
    std::string name = this->parse_tag_name();
    c = this->consume_next_character();
    assert(c == '=');
    std::string value = this->parse_attribute_value();
    return std::make_pair(name, value);
  }

  AttributeMap parse_attributes() {
    // std::cout << "parse attributes" << std::endl;
    AttributeMap m;
    for (;;) {
      this->consume_spaces();
      if (this->next_character() == '>') {
        break;
      }
      auto attribute = this->parse_attribute();
      m[attribute.first] = attribute.second;
    }
    return m;
  }

  Node *parse_element() {
    // std::cout << "parse element " << std::endl;
    // parse opening tag
    char c = this->consume_next_character();
    assert(c == '<');
    std::string tag_name = this->parse_tag_name();
    AttributeMap attrs = this->parse_attributes();
    c = this->consume_next_character();
    assert(c == '>');

    auto children = this->parse_nodes();

    // parse closing tag
    c = this->consume_next_character();
    assert(c == '<');
    c = this->consume_next_character();
    assert(c == '/');
    auto tag = this->parse_tag_name();
    assert(tag == tag_name);
    c = this->consume_next_character();
    assert(c == '>');

    return createElement(tag_name, attrs, children);
  }

  Node *parse_node() {
    // std::cout << "parse node " << std::endl;
    char c = this->next_character();
    if (c == '<') {
      return this->parse_element();
    } else {
      return this->parse_text();
    }
  }

  std::vector<Node *> parse_nodes() {
    // std::cout << "parse nodes" << std::endl;
    std::vector<Node *> nodes;
    for (;;) {
      this->consume_spaces();
      if (this->is_eof() || this->starts_with("</")) {
        break;
      }
      auto n = this->parse_node();
      nodes.push_back(n);
    }
    return nodes;
  }
};

Node *parse(std::string input) {
  auto parser = Parser(input);
  auto nodes = parser.parse_nodes();
  if (nodes.size() == 1) {
    return nodes[0];
  } else {
    AttributeMap m;
    return new ElementNode("html", m, nodes);
  }
}


#endif
