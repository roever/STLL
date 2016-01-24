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
#ifndef STLL_LAYOUTER_OPEN_GL
#define STLL_LAYOUTER_OPEN_GL

/** \file
 *  \brief OpenGL output driver
 */

#include "layouterFont.h"
#include "color.h"

#include "internal/glyphAtlas.h"
#include "internal/gamma.h"
#include "internal/ogl_shader.h"

namespace STLL {

/** \brief a class to output layouts using OpenGL
 *
 * To output layouts using this class, create an object of it and then
 * use the showLayout Function to output the layout.
 *
 * The class contains a glyph cache in form of an texture atlas. Once this
 * atlas if full, things available will be output and then the atlas will be cleared
 * and repopulated for the next section of the output. This will slow down output
 * considerably, so choose the size wisely. The atlas will be destroyed once the
 * class is destroyed. Things to consider:
 * - using sub pixel placement triples the space requirements for the glyphs
 * - blurring adds quite some amount of space around the glyphs, but as soon as you blurr the
 *   sub pixel placement will not be used as you will not see the difference anyways
 * - normal rectangles will not go into the cache, but blurres ones will
 * - so in short avoid blurring
 *
 * As OpenGL required function loaders and does not provide a header with all available
 * functionality you will need to include the proper OpenGL before including the header file
 * for this class.
 *
 * Gamma correct output is not handled by this class directly. You need to activate the sRGB
 * property for the target that this paints on
 *
 * \tparam V The OpenGL version you want to use... the class will adapt accordingly and use the
 * available features
 * \tparam C size of the texture cache. The cache is square C time C pixels.
 * \tparam G the gamma calculation function, if you use sRGB output... normally you don't need
 * to change this, keep the default
 */
template <int V, int C, class G = internal::Gamma_c<>>
class showOpenGL
{
  private:
    internal::GlyphAtlas_c cache;
    G gamma;

    GLuint glTextureId = 0;     // OpenGL texture id
    uint32_t uploadVersion = 0; // a counter changed each time the texture changes to know when to update

    internal::OGL_Program_c program;
    GLuint vertexBuffer, vertexArray;

    class vertex
    {
    public:
      GLfloat x, y;   // position
      GLfloat u, v;   // texture
      uint8_t r, g, b, a; // colour
      GLbyte sp;

      vertex (GLfloat _x, GLfloat _y, GLfloat _u, GLfloat _v, Color_c c, uint8_t _sp) :
      x(_x), y(_y), u(_u), v(_v), r(c.r()), g(c.g()), b(c.b()), a(c.a()), sp(_sp) {}
    };

  public:

    /** \brief constructor */
    showOpenGL(void) : cache(C, C) {
      glActiveTexture(GL_TEXTURE0);
      glGenTextures(1, &glTextureId);
      glBindTexture(GL_TEXTURE_2D, glTextureId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      gamma.setGamma(22);

      program.attachShader(GL_FRAGMENT_SHADER, "330 core",
        "uniform sampler2D texture;"
        "uniform vec2 texRshift;"
        "uniform vec2 texGshift;"
        "uniform vec2 texBshift;"

        "in vec2 TexCoord;"
        "in vec4 ourColor;"
        "in float sp;"

        "layout (location = 0, index = 0) out vec4 color;"
        "layout (location = 0, index = 1) out vec4 alpha;"

        "void main()"
        "{"
        "  vec4 r = texture2D(texture, TexCoord+sp*texRshift);"
        "  vec4 g = texture2D(texture, TexCoord+sp*texGshift);"
        "  vec4 b = texture2D(texture, TexCoord+sp*texBshift);"
        "  color = ourColor;"
        "  alpha = vec4(r.r, g.r, b.r, 1.0);"
        "}"
      );

      program.attachShader(GL_VERTEX_SHADER, "330 core",
        "uniform float width;"
        "uniform float height;"

        "layout (location = 0) in vec3 vertex;"
        "layout (location = 1) in vec2 tex_coord;"
        "layout (location = 2) in vec4 color;"
        "layout (location = 3) in float subpixels;"

        "out vec2 TexCoord;"
        "out vec4 ourColor;"
        "out float sp;"

        "void main()"
        "{"
        "  ourColor = vec4(color.r/255.0, color.g/255.0, color.b/255.0, color.a/255.0);"
        "  TexCoord = vec2(tex_coord.x, tex_coord.y);"
        "  gl_Position = vec4((vertex.x-width)/width, 1.0-vertex.y/height, 0, 1.0);"
        "  sp = subpixels;"
        "}"
      );

      program.link();

      program.setUniform("texture", 0);

      glGenVertexArrays(1, &vertexArray);

      glBindVertexArray(vertexArray);

      glGenBuffers(1, &vertexBuffer);

      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

      glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, x));
      glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, u));
      glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, r));
      glEnableVertexAttribArray(3); glVertexAttribPointer(3, 1, GL_BYTE, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, sp));
    }

    ~showOpenGL(void)
    {
        glDeleteTextures(1, &glTextureId);
        glDeleteBuffers(1, &vertexBuffer);
    }

    /** \brief helper class used to draw images */
    class imageDrawerOpenGL_c
    {
      public:
        virtual void draw(int32_t x, int32_t y, uint32_t w, uint32_t h, const std::string & url) = 0;
    };


    /** \brief paint the layout
     *
     * \param l the layout to draw
     * \param sx x position on the target surface in 1/64th pixels
     * \param sy y position on the target surface in 1/64th pixels
     * \param sp which kind of sub-pixel positioning do you want?
     * \param images a pointer to an image drawer class that is used to draw the images, when you give
     *                a nullptr here, no images will be drawn
     * \return if true then the cache had to be cleared... which might hint at a problem with your cache
     * size
     */
    bool showLayout(const TextLayout_c & l, int sx, int sy, SubPixelArrangement sp, imageDrawerOpenGL_c * images)
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, glTextureId );
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

      const auto & dat = l.getData();
      size_t i = 0;
      bool cleared = false;

      while (i < dat.size())
      {
        size_t j = i;

        // make sure that there is a small completely filled rectangle
        // used for drawing filled rectangles
        cache.getRect(640, 640, SUBP_NONE, 0);

        while (j < dat.size())
        {
          auto & ii = dat[j];

          bool found = true;

          switch (ii.command)
          {
            case CommandData_c::CMD_GLYPH:
              // when subpixel placement is on we always create all 3 required images
              found &= (bool)cache.getGlyph(ii.font, ii.glyphIndex, sp, ii.blurr);
              break;
            case CommandData_c::CMD_RECT:
              if (ii.blurr > 0)
                found &= (bool)cache.getRect(ii.w, ii.h, sp, ii.blurr);
              break;

            default:
              break;
          }

          if (!found) break;

          j++;
        }

        if (cache.getVersion() != uploadVersion)
        {
          uploadVersion = cache.getVersion();
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, C, C, 0, GL_RED, GL_UNSIGNED_BYTE, cache.getData());
        }

        std::vector<vertex> vb;

        size_t k = i;

        while (k < j)
        {
          auto & ii = dat[k];

          switch (ii.command)
          {
            case CommandData_c::CMD_GLYPH:
              {
                auto pos = cache.getGlyph(ii.font, ii.glyphIndex, sp, ii.blurr).value();
                Color_c c = gamma.forward(ii.c);

                if ((sp == SUBP_RGB || sp == SUBP_BGR) && (ii.blurr <= cache.blurrmax))
                {
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left,                   (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,             1.0*(pos.pos_y)/C,          c, 1));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left+(pos.width-1)/3.0, (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x+pos.width-1)/C, 1.0*(pos.pos_y)/C,          c, 1));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left+(pos.width-1)/3.0, (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width-1)/C, 1.0*(pos.pos_y+pos.rows)/C, c, 1));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left+(pos.width-1)/3.0, (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width-1)/C, 1.0*(pos.pos_y+pos.rows)/C, c, 1));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left,                   (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x)/C,             1.0*(pos.pos_y+pos.rows)/C, c, 1));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left,                   (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,             1.0*(pos.pos_y)/C,          c, 1));
                }
                else
                {
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left,           (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c, 0));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left+pos.width, (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y)/C,          c, 0));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left+pos.width, (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c, 0));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left+pos.width, (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c, 0));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left,           (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x)/C,           1.0*(pos.pos_y+pos.rows)/C, c, 0));
                  vb.push_back(vertex((sx+ii.x)/64.0+pos.left,           (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c, 0));
                }
              }
              break;

            case CommandData_c::CMD_RECT:

              if (ii.blurr == 0)
              {
                auto pos = cache.getRect(640, 640, SUBP_NONE, 0).value();
                Color_c c = gamma.forward(ii.c);
                vb.push_back(vertex((sx+ii.x+32)/64,      (sy+ii.y+32)/64,      1.0*(pos.pos_x+5)/C,           1.0*(pos.pos_y+5)/C,          c, 0));
                vb.push_back(vertex((sx+ii.x+32+ii.w)/64, (sy+ii.y+32)/64,      1.0*(pos.pos_x+pos.width-6)/C, 1.0*(pos.pos_y+5)/C,          c, 0));
                vb.push_back(vertex((sx+ii.x+32+ii.w)/64, (sy+ii.y+32+ii.h)/64, 1.0*(pos.pos_x+pos.width-6)/C, 1.0*(pos.pos_y+pos.rows-6)/C, c, 0));
                vb.push_back(vertex((sx+ii.x+32+ii.w)/64, (sy+ii.y+32+ii.h)/64, 1.0*(pos.pos_x+pos.width-6)/C, 1.0*(pos.pos_y+pos.rows-6)/C, c, 0));
                vb.push_back(vertex((sx+ii.x+32)/64,      (sy+ii.y+32+ii.h)/64, 1.0*(pos.pos_x+5)/C,           1.0*(pos.pos_y+pos.rows-6)/C, c, 0));
                vb.push_back(vertex((sx+ii.x+32)/64,      (sy+ii.y+32)/64,      1.0*(pos.pos_x+5)/C,           1.0*(pos.pos_y+5)/C,          c, 0));
              }
              else
              {
                auto pos = cache.getRect(ii.w, ii.h, sp, ii.blurr).value();
                Color_c c = gamma.forward(ii.c);
                vb.push_back(vertex((sx+ii.x+32)/64+pos.left,           (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c, 1));
                vb.push_back(vertex((sx+ii.x+32)/64+pos.left+pos.width, (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y)/C,          c, 1));
                vb.push_back(vertex((sx+ii.x+32)/64+pos.left+pos.width, (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c, 1));
                vb.push_back(vertex((sx+ii.x+32)/64+pos.left+pos.width, (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x+pos.width)/C, 1.0*(pos.pos_y+pos.rows)/C, c, 1));
                vb.push_back(vertex((sx+ii.x+32)/64+pos.left,           (sy+ii.y+32)/64-pos.top+pos.rows,1.0*(pos.pos_x)/C,           1.0*(pos.pos_y+pos.rows)/C, c, 1));
                vb.push_back(vertex((sx+ii.x+32)/64+pos.left,           (sy+ii.y+32)/64-pos.top,         1.0*(pos.pos_x)/C,           1.0*(pos.pos_y)/C,          c, 1));
              }
              break;

            case CommandData_c::CMD_IMAGE:
              if (images)
                images->draw(ii.x+sx, ii.y+sy, ii.w, ii.h, ii.imageURL);
              break;
          }
          k++;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vb.size(), vb.data(), GL_STREAM_DRAW);

        switch (sp)
        {
          default:
          case SUBP_NONE:
            program.setUniform("texRshift", 0.0f, 0.0f);
            program.setUniform("texGshift", 0.0f, 0.0f);
            program.setUniform("texBshift", 0.0f, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, vb.size());
            break;

          case SUBP_RGB:
            program.setUniform("texRshift", 0.0f, 0.0f);
            program.setUniform("texGshift", 1.0f/C, 0.0f);
            program.setUniform("texBshift", 2.0f/C, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, vb.size());
            break;

          case SUBP_BGR:
            program.setUniform("texRshift", 2.0f/C, 0.0f);
            program.setUniform("texGshift", 1.0f/C, 0.0f);
            program.setUniform("texBshift", 0.0f/C, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, vb.size());
            break;
        }

        i = j;

        if (i < dat.size())
        {
          // atlas is not big enough, it needs to be cleared
          // and will be repopulated for the next batch of the layout
          cache.clear();
          cleared = true;
        }
      }

      return cleared;
    }

    /** \brief helper function to setup the projection matrices for
     * the showLayout function. It will change the viewport and the
     * modelview and projection matrix to an orthogonal projection
     */
    void setupMatrixes(int width, int height)
    {
      glViewport(0, 0, width, height);
      program.setUniform("width", width/2.0f);
      program.setUniform("height", height/2.0f);
    }

    /** \brief get a pointer to the texture atlas with all the glyphs
     *
     * This is mainly helpful to check how full the texture atlas is
     */
    const uint8_t * getData(void) const { return cache.getData(); }

    void clear(void) { cache.clear(); }
};

}

#endif
