/*
 * STLL Simple Text Layouting Library
 *
 * STLL is the legal property of its developers, whose
 * names are listed in the COPYRIGHT file, which is included
 * within the source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stll/layouter.h>
#include <stll/layouterFont.h>
#include "layouterXMLSaveLoad.h"

#include <GL/gl.h>
#include <stll/output_OpenGL.h>

#include <GLFW/glfw3.h>


using namespace STLL;

static void key_callback(GLFWwindow* window, int key, int, int action, int)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argv, char ** args)
{
  pugi::xml_document doc;

  if (argv != 2)
  {
    printf("specify the layout to load as an argument\n");
    return 1;
  }

  auto res = doc.load_file(args[1]);

  if (!res)
  {
    printf("%s\n", (std::string("oopsi loading file...") + res.description()).c_str());
    return 1;
  }

  auto c = std::make_shared<FontCache_c>();
  auto l = loadLayoutFromXML(doc.child("layout"), c);

  if (!glfwInit())
  {
    fprintf(stderr, "Failed to initialize GLFW");
    return 1;
  }

  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

  GLFWwindow* screen = glfwCreateWindow(l.getRight()/64, l.getHeight()/64, "OpenGL Layout Viewer", NULL, NULL);

  if(!screen)
  {
    fprintf(stderr, "Failed to create OpenGL window");
    return 1;
  }

  glfwMakeContextCurrent(screen);
  glfwSetKeyCallback(screen, key_callback);

  showOpenGL<2, 512> openGL;

  while (!glfwWindowShouldClose(screen))
  {
    int width, height;
    glfwGetFramebufferSize(screen, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,width,height, 0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(50.0/255, 50.0/255, 50.0/255);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    for (int x = 0; x < 1+(int)l.getRight()/640; x++)
      for (int y = 0; y < 1+(int)l.getHeight()/640; y++)
      {
        if ((x + y) % 2)
        {
          glVertex3f(0.5+10*x,    0.5+10*y,    0);
          glVertex3f(0.5+10*x+10, 0.5+10*y,    0);
          glVertex3f(0.5+10*x+10, 0.5+10*y+10, 0);
          glVertex3f(0.5+10*x,    0.5+10*y+10, 0);
        }
      }
    glEnd();

    glEnable(GL_FRAMEBUFFER_SRGB);
    openGL.showLayout(l, 0, 0, SUBP_NONE, 0);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glfwSwapBuffers(screen);
    glfwPollEvents();
  }

  {
    FILE * f = fopen("tex.data", "wb");
    fwrite(openGL.getData(), 1, 512*512, f);
    fclose(f);
  }

  glfwDestroyWindow(screen);
  glfwTerminate();

}
