#ifndef PAINTER_CPP
#define PAINTER_CPP

#include <vector>

#include "css_parser.cpp"

enum DisplayCommandType {
  SOLID_COLOR,
};

struct DisplayCommand {
  DisplayCommandType type;
  Color color;
  Rect box;

  friend std::ostream &operator<<(std::ostream &os, const DisplayCommand &dc) {
    os << "DisplayCommand: ";
    os << "(" << dc.type << ")";
    os << " color: " << dc.color;
    os << "\n";
    os << dc.box;
    return os;
  }
};

typedef std::vector<DisplayCommand> DisplayList;

Color get_color(LayoutBox layout, std::string name) {
  // TODO support get color
  return Color{100, 100, 100, 255};
}

void render_background(DisplayList &list, LayoutBox layout) {
  auto color = get_color(layout, "background");
  list.push_back(DisplayCommand{DisplayCommandType::SOLID_COLOR, color,
                                layout.dims.border_box()});
}

void render_borders(DisplayList &list, LayoutBox layout) {
  auto color = get_color(layout, "border-color");

  auto dims = layout.dims;
  auto border = dims.border_box();
  list.push_back(DisplayCommand{DisplayCommandType::SOLID_COLOR, color,
                                Rect{
                                    border.x,
                                    border.y,
                                    dims.border.left,
                                    border.height,
                                }});

  list.push_back(DisplayCommand{DisplayCommandType::SOLID_COLOR, color,
                                Rect{
                                    border.x + border.width - dims.border.right,
                                    border.y,
                                    dims.border.right,
                                    border.height,
                                }});
  list.push_back(DisplayCommand{DisplayCommandType::SOLID_COLOR, color,
                                Rect{
                                    border.x,
                                    border.y,
                                    border.width,
                                    dims.border.top,
                                }});
  list.push_back(
      DisplayCommand{DisplayCommandType::SOLID_COLOR, color,
                     Rect{
                         border.x,
                         border.y + border.height - dims.border.bottom,
                         border.width,
                         dims.border.bottom,
                     }});
}

void render_layout_box(DisplayList &list, LayoutBox layout) {
  render_background(list, layout);
  render_borders(list, layout);
  // TODO render text
  for (auto child : layout.children) {
    render_layout_box(list, child);
  }
}

DisplayList build_display_list(LayoutBox root) {
  DisplayList list;
  render_layout_box(list, root);
  return list;
}

#endif
