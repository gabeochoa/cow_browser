
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include "base_window.hpp"
#include "html_parser.hpp"

#ifndef CSS_PARSER_HPP
#define CSS_PARSER_HPP

#endif

void loop(GLFWwindow *window, Node *root) {
  ImGui::Begin("My name is window");
  ImGui::Text("im am text");
  ImGui::End();
}

int main() {
  GLFWwindow *window = init_window();
  if (window == nullptr) {
    std::cout << "Failed to init glfw window" << std::endl;
    return -1;
  }

  std::ifstream t("example_html/index.html");
  std::stringstream buffer;
  buffer << t.rdbuf();
  // TODO read directly?
  Node *root = parse(buffer.str());
  // std::cout << *root << std::endl;

  run_until_close(window, std::bind(loop, std::placeholders::_1, root));

  return 0;
}
