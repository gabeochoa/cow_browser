#ifndef HTML_PARSER
#define HTML_PARSER

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "parser.cpp"

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

  // TODO this causes a segfault so lets just leak
  // virtual ~Node() {
  // while (!children.empty())
  // delete children.back(), children.pop_back();
  // }

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

std::vector<std::string> split(std::string s, std::string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
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

  std::string id() { return attrs.at("id"); }

  std::set<std::string> classes() {
    std::set<std::string> s;
    try {
      std::string classes_str = attrs.at("class");
      auto classes = split(classes_str, " ");
      for (auto class_ : classes) {
        s.insert(class_);
      }
    } catch (const std::exception &e) {
      return s;
    }
    return s;
  }
};

TextNode *createText(std::string content) { return new TextNode(content); }

ElementNode *createElement(std::string name, AttributeMap attrs,
                           std::vector<Node *> children) {
  return new ElementNode(name, attrs, children);
}

struct HtmlParser : public Parser {
  HtmlParser(std::string i) : Parser(i) {}

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

Node *parse_html(std::string input) {
  auto parser = HtmlParser(input);
  auto nodes = parser.parse_nodes();
  if (nodes.size() == 1) {
    return nodes[0];
  } else {
    AttributeMap m;
    return new ElementNode("html", m, nodes);
  }
}

#endif
