
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include "base_window.hpp"
#include "css_parser.cpp"
#include "html_parser.cpp"
#include "painter.cpp"

void loop(GLFWwindow *window, Node *root) {
  ImGui::Begin("My name is window");
  ImGui::Text("im am text");
  ImGui::End();

  bool show = true;
  ImGui::ShowDemoWindow(&show);
}

void paint_item(DisplayCommand command) {
  if (command.type == DisplayCommandType::SOLID_COLOR) {
    auto box = command.box;
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(
        ImVec2(box.x, box.y), ImVec2(box.x + box.width, box.y + box.height),
        ImColor(command.color.r, command.color.g, command.color.b));
  }
}

StyleSheet example_parse_css() {
  std::ifstream css("example_html/index.css");
  std::stringstream cssbuffer;
  cssbuffer << css.rdbuf();
  StyleSheet sheet = parse_css(cssbuffer.str());
  // std::cout << sheet << std::endl;
  return sheet;
}

Node *example_parse_html() {
  std::ifstream t("example_html/index.html");
  std::stringstream buffer;
  buffer << t.rdbuf();
  // TODO read directly?
  Node *root = parse_html(buffer.str());
  // std::cout << *root << std::endl;
  return root;
}

StyledNode style_tree(Node *root, StyleSheet sheet) {
  StyledNode styled_node;
  PropertyMap values;

  if (root->type == NodeType::Element) {
    ElementNode *elem = dynamic_cast<ElementNode *>(root);
    values = specified_values(*elem, sheet);
  } else {
    // case NodeType::Text:
    // case NodeType::Unknown:
    // leave empty
  }

  std::vector<StyledNode> children;
  for (Node *node : root->children) {
    children.push_back(style_tree(node, sheet));
  }

  styled_node.node = *root;
  styled_node.values = values;
  styled_node.children = children;
  return styled_node;
}

int main() {
  Node *root = example_parse_html();
  StyleSheet sheet = example_parse_css();
  StyledNode styled_root = style_tree(root, sheet);
  LayoutBox layed_root = build_layout_tree(styled_root);
  DisplayList display_list = build_display_list(layed_root);

  GLFWwindow *window = init_window();
  if (window == nullptr) {
    std::cout << "Failed to init glfw window" << std::endl;
    return -1;
  }

  run_until_close(window, std::bind(loop, std::placeholders::_1, root));

  return 0;
}
