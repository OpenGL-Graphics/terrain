#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "window.hpp"
#include "navigation/camera.hpp"
#include "texture_2d.hpp"
#include "shader_exception.hpp"

#include "geometries/plane.hpp"
#include "render/splatmap.hpp"

using namespace geometry;

int main() {
  ////////////////////////////////////////////////
  // Window & camera
  ////////////////////////////////////////////////

  // glfw window
  Window window("FPS game");

  if (window.is_null()) {
    std::cout << "Failed to create window or OpenGL context" << "\n";
    return 1;
  }

  // initialize glad before calling gl functions
  window.make_context();
  if (!gladLoadGL()) {
    std::cout << "Failed to load Glad (OpenGL)" << "\n";
    window.destroy();
    return 1;
  } else {
    std::cout << "Opengl version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
  }

  // camera
  glm::vec3 position_cam(15, 10, 25);
  // glm::vec3 position_cam(20, 20, 50);
  // glm::vec3 direction_cam(0, 0, -1);
  glm::vec3 direction_cam(0, -0.5, -1);
  glm::vec3 up_cam(0, 1, 0);
  Camera camera(position_cam, direction_cam, up_cam);

  // transformation matrices
  float near = 0.001,
        far = 50.0;
  float aspect_ratio = (float) window.width / (float) window.height;
  glm::mat4 projection3d = glm::perspective(glm::radians(camera.fov), aspect_ratio, near, far);

  ////////////////////////////////////////////////
  // Renderers
  ////////////////////////////////////////////////

  // create & install vertex & fragment shaders on GPU
  Program program_plane("assets/shaders/plane.vert", "assets/shaders/plane.frag");
  Program program_terrain("assets/shaders/light_terrain.vert", "assets/shaders/light_terrain.frag");

  if (program_plane.has_failed() || program_terrain.has_failed()) {
    window.destroy();
    throw ShaderException();
  }

  // flat grid plane (shape made as a sin wave in vertex shader)
  // renderer (encapsulates VAO & VBO) for each shape to render
  Texture2D texture_wave(Image("assets/images/plane/wave.png"));
  Renderer plane(program_plane, Plane(50, 50), Attributes::get({"position", "normal", "texture_coord"}));
  // Renderer plane(program_plane, Plane(500, 500), Attributes::get({"position", "normal", "texture_coord"}));

  // terrain from triangle strips & textured with image splatmap
  Splatmap terrain(program_terrain);

  // enable depth test & blending & stencil test (for outlines)
  glEnable(GL_DEPTH_TEST);
  glm::vec4 background(0.0f, 0.0f, 0.0f, 1.0f);
  glClearColor(background.r, background.g, background.b, background.a);

  // backface (where winding order is CW) not rendered (boost fps)
  glEnable(GL_CULL_FACE);

  // take this line as a ref. to calculate initial fps (not `glfwInit()`)
  window.init_timer();

  ////////////////////////////////////////////////
  // Game loop
  ////////////////////////////////////////////////

  while (!window.is_closed()) {
    // update transformation matrices (camera fov changes on zoom)
    glm::mat4 view = camera.get_view();
    projection3d = glm::perspective(glm::radians(camera.fov), (float) window.width / (float) window.height, 0.5f, 32.0f);

    // clear color & depth buffers before rendering every frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_TRUE);

    // draw textured terrain using triangle strips
    terrain.set_transform({
      // { glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.5f, -14)) },
      { glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -14)) },
      view, projection3d
    });
    terrain.draw();

    // draw animated & textured wave from plane using triangle strips
    plane.set_transform({
      { glm::translate(glm::mat4(1.0f), glm::vec3(3, 3, 0)) },
      view, projection3d
    });

    plane.draw_plane({
      {"texture2d", texture_wave},
      {"time", static_cast<float>(glfwGetTime())},
    });

    // process events & show rendered buffer
    window.process_events();
    window.render();

    // leave main loop on press on <q>
    if (window.is_key_pressed(GLFW_KEY_Q))
      break;
  }

  // destroy textures
  texture_wave.free();

  // destroy shaders
  program_plane.free();
  program_terrain.free();

  // destroy renderers of each shape (frees vao & vbo)
  terrain.free();
  plane.free();

  // destroy window & terminate glfw
  window.destroy();

  return 0;
}
