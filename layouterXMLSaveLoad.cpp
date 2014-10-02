
#include "layouterXMLSaveLoad.h"

namespace STLL
{

void saveLayoutToXML(const textLayout_c & l, pugi::xml_node & node, std::shared_ptr<fontCache_c> c)
{
  auto doc = node.append_child();
  doc.set_name("layout");
  doc.append_attribute("height").set_value(l.getHeight());
  doc.append_attribute("left").set_value(l.getLeft());
  doc.append_attribute("right").set_value(l.getRight());

  // output fonts;
  auto fonts = doc.append_child();
  fonts.set_name("fonts");

  std::vector<std::shared_ptr<STLL::fontFace_c>> found;

  for (const auto & a : l.data)
  {
    if (  (a.command == textLayout_c::commandData::CMD_GLYPH)
        &&(std::find(found.begin(), found.end(), a.font) == found.end())
       )
    {
      found.push_back(a.font);

      auto fnt = fonts.append_child();
      fnt.set_name("font");
      fnt.append_attribute("file").set_value(c->getFontResource(a.font).getDescription().c_str());
      fnt.append_attribute("size").set_value(c->getFontSize(a.font));
    }
  }

  // output commands
  auto commands = doc.append_child();
  commands.set_name("commands");

  for (const auto & a : l.data)
  {
    switch (a.command)
    {
      case STLL::textLayout_c::commandData::CMD_GLYPH:
        {
          auto n = commands.append_child();
          n.set_name("glyph");
          n.append_attribute("x").set_value(a.x);
          n.append_attribute("y").set_value(a.y);
          n.append_attribute("glyphIndex").set_value(static_cast<unsigned int>(a.glyphIndex));
          n.append_attribute("font").set_value(static_cast<int>(std::distance(found.begin(), std::find(found.begin(), found.end(), a.font))));
          n.append_attribute("r").set_value(a.c.r());
          n.append_attribute("g").set_value(a.c.g());
          n.append_attribute("b").set_value(a.c.b());
          n.append_attribute("a").set_value(a.c.a());
        }
        break;
      case STLL::textLayout_c::commandData::CMD_RECT:
        {
          auto n = commands.append_child();
          n.set_name("rect");
          n.append_attribute("x").set_value(a.x);
          n.append_attribute("y").set_value(a.y);
          n.append_attribute("w").set_value(a.w);
          n.append_attribute("h").set_value(a.h);
          n.append_attribute("r").set_value(a.c.r());
          n.append_attribute("g").set_value(a.c.g());
          n.append_attribute("b").set_value(a.c.b());
          n.append_attribute("a").set_value(a.c.a());
        }
        break;
      case STLL::textLayout_c::commandData::CMD_IMAGE:
        {
          auto n = commands.append_child();
          n.set_name("image");
          n.append_attribute("x").set_value(a.x);
          n.append_attribute("y").set_value(a.y);
          n.append_attribute("url").set_value(a.imageURL.c_str());
        }
        break;
    }
  }
}


textLayout_c loadLayoutFromXML(const pugi::xml_node & doc, std::shared_ptr<fontCache_c> c)
{
  // get the fonts from the file
  auto fonts = doc.child("fonts");
  std::vector<std::shared_ptr<fontFace_c>> found;

  for (const auto a : fonts.children())
    found.push_back(c->getFont(fontResource_c(a.attribute("file").value()), std::stoi(a.attribute("size").value())));

  auto commands = doc.child("commands");

  textLayout_c l;

  for (const auto a : commands.children())
  {
    if (a.name() == std::string("glyph"))
    {
      textLayout_c::commandData c;

      c.command = textLayout_c::commandData::CMD_GLYPH;
      c.x = std::stoi(a.attribute("x").value());
      c.y = std::stoi(a.attribute("y").value());
      c.glyphIndex = std::stoi(a.attribute("glyphIndex").value());
      c.font = found[std::stoi(a.attribute("font").value())];
      c.c = color_c(std::stoi(a.attribute("r").value()), std::stoi(a.attribute("g").value()),
                    std::stoi(a.attribute("b").value()), std::stoi(a.attribute("a").value()));

      l.data.push_back(c);
    }
    else if (a.name() == std::string("rect"))
    {
      textLayout_c::commandData c;

      c.command = textLayout_c::commandData::CMD_RECT;
      c.x = std::stoi(a.attribute("x").value());
      c.y = std::stoi(a.attribute("y").value());
      c.w = std::stoi(a.attribute("w").value());
      c.h = std::stoi(a.attribute("h").value());
      c.c = color_c(std::stoi(a.attribute("r").value()), std::stoi(a.attribute("g").value()),
                    std::stoi(a.attribute("b").value()), std::stoi(a.attribute("a").value()));

      l.data.push_back(c);
    }
    else if (a.name() == std::string("image"))
    {
      textLayout_c::commandData c;

      c.command = textLayout_c::commandData::CMD_IMAGE;
      c.x = std::stoi(a.attribute("x").value());
      c.y = std::stoi(a.attribute("y").value());
      c.imageURL = a.attribute("url").value();

      l.data.push_back(c);
    }
  }

  l.setHeight(std::stoi(doc.attribute("height").value()));
  l.setLeft(std::stoi(doc.attribute("left").value()));
  l.setRight(std::stoi(doc.attribute("right").value()));

  return l;
}

}