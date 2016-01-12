// you have to define the xml library you want to use
// STLL needs to be compiled with support for this library
#define USE_PUGI_XML

#include <stll/layouter.h>
#include <stll/output_SDL.h>
#include <stll/layouterXHTML.h>

using namespace STLL;

int main()
{
  // setup a stylesheet
  TextStyleSheet_c styleSheet;

  // add the fonts that are required
  // we create one font family names "sans" with 2 members, a normal and a bold one
  styleSheet.font("sans", FontResource_c("tests/FreeSans.ttf"));
  styleSheet.font("sans", FontResource_c("tests/FreeSansBold.ttf"), "normal", "normal", "bold");

  // add the CSS rules you need, this should be familiar to everybody knowing CSS
  styleSheet.addRule("body", "color", "#ffffff");
  styleSheet.addRule("body", "font-size", "20px");
  styleSheet.addRule("body", "text-align", "justify");
  styleSheet.addRule("body", "padding", "10px");
  styleSheet.addRule("h1", "font-weight", "bold");
  styleSheet.addRule("h1", "font-size", "60px");
  styleSheet.addRule("h1", "text-align", "center");
  styleSheet.addRule("h1", "background-color", "#FF8080");

  // The XHTML code we want to format, it needs to be utf-8 encoded
  std::string text = u8"<html><body><h1>Title</h1><p>Some text</p></body></html>";

  // layout the XHTML code, in a 200 pixel wide rectangle
  auto layout = layoutXHTMLPugi(text, styleSheet, RectangleShape_c(200*64) );

  // again output the layout
  SDL_Surface *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
  showLayoutSDL(layout, 20*64, 20*64, screen);
  SDL_Flip(screen);
  sleep(10);
}