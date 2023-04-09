#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "window.hpp"
#include "navigation/camera.hpp"
#include "shader/shader_exception.hpp"

#include "texture/texture_3d.hpp"
#include "texture/texture_2d.hpp"

#include "geometries/cube.hpp"
#include "geometries/gizmo.hpp"
#include "geometries/grid_lines.hpp"

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
  float cam_y = 10;
  glm::vec3 position_cam(25, cam_y, 50);
  glm::vec3 direction_cam(0, -0.5, -1);
  glm::vec3 up_cam(0, 1, 0);
  Camera camera(position_cam, direction_cam, up_cam);

  // transformation matrices
  float near = 0.001,
        far = 100.0;
  float aspect_ratio = (float) window.width / (float) window.height;
  glm::mat4 projection3d = glm::perspective(glm::radians(camera.fov), aspect_ratio, near, far);

  ////////////////////////////////////////////////
  // Renderers
  ////////////////////////////////////////////////

  // create & install vertex & fragment shaders on GPU
  Program program_terrain("assets/shaders/light_terrain.vert", "assets/shaders/light_terrain.frag");
  Program program_skybox("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
  Program program_basic("assets/shaders/basic.vert", "assets/shaders/basic.frag");

  if (program_terrain.has_failed() || program_skybox.has_failed() || program_basic.has_failed()) {
    window.destroy();
    throw ShaderException();
  }

  // 3D cube texture for skybox (left-handed coords system for cubemaps)
  // See faces order: https://www.khronos.org/opengl/wiki/Cubemap_Texture
  // cubemap images have their origin at upper-left corner (=> don't flip)
  // https://stackoverflow.com/a/11690553/2228912
  std::vector<Image> skybox_images = {
    Image("assets/images/skybox/posx.jpg", false), // pos-x (right face)
    Image("assets/images/skybox/negx.jpg", false), // neg-x (left face)
    Image("assets/images/skybox/posy.jpg", false), // pos-y (top face)
    Image("assets/images/skybox/negy.jpg", false), // neg-y (bottom face)
    Image("assets/images/skybox/posz.jpg", false), // pos-z (front face)
    Image("assets/images/skybox/negz.jpg", false)  // neg-z (back face)
  };
  Texture3D texture_skybox(skybox_images);
  Renderer skybox(program_skybox, Cube(true), Attributes::get({"position"}, 8));

  // terrain from triangle strips & textured with image splatmap
  Splatmap terrain(program_terrain);

  // grid & gizmo for debugging
  Renderer gizmo(program_basic, Gizmo(), Attributes::get({"position"}));
  Renderer grid(program_basic, GridLines(50), Attributes::get({"position"}));

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
    // clear color & depth buffers before rendering every frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_TRUE);

    /* rotate camera around pivot */
    glm::vec3 pivot(25, 0, 25); // center of grid

    float radius = 25,
          time = glfwGetTime(),
          angle = time / 2.0; // slow down rotation

    // rotate around origin & translate by offset = pivot
    float cam_x = pivot.x + radius * std::cos(angle),
          cam_z = pivot.z + radius * std::sin(angle);
    camera.position = glm::vec3(cam_x, cam_y, cam_z);

    camera.direction = pivot - camera.position;
    glm::mat4 view = camera.get_view();

    // draw skybox
    // https://learnopengl.com/Advanced-OpenGL/Cubemaps
    // disable depth testing so skybox is always at background
    // mandatory bcos of below otherwise cube will hide everything else (coz it closest to camera)
    glDepthMask(GL_FALSE);

    // no translation of skybox when camera moves
    // camera initially at origin always inside skybox unit cube => skybox looks larger
    glm::mat4 view_without_translation = glm::mat4(glm::mat3(view));
    skybox.set_transform({
      { glm::scale(glm::mat4(1.0f), glm::vec3(2)) },
      view_without_translation, projection3d
    });
    skybox.draw({ {"texture3d", texture_skybox } });

    glDepthMask(GL_TRUE);

    // draw xyz gizmo at origin using GL_LINES
    gizmo.set_transform({ { glm::mat4(1.0) }, view, projection3d });
    gizmo.draw_lines({ {"colors[0]", glm::vec3(1.0f, 0.0f, 0.0f)} }, 2, 0);
    gizmo.draw_lines({ {"colors[0]", glm::vec3(0.0f, 1.0f, 0.0f)} }, 2, 2);
    gizmo.draw_lines({ {"colors[0]", glm::vec3(0.0f, 0.0f, 1.0f)} }, 2, 4);

    // draw horizontal 2d grid using GL_LINES
    grid.set_transform({ { glm::mat4(1.0) }, view, projection3d });
    // grid.draw_lines({ {"colors[0]", glm::vec3(1.0f, 1.0f, 1.0f)} });

    // draw textured terrain using triangle strips
    terrain.set_transform({
      { glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.5f, -14)) },
      // { glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -14)) },
      view, projection3d
    });
    terrain.draw();

    // process events & show rendered buffer
    window.process_events();
    window.render();

    // leave main loop on press on <q>
    if (window.is_key_pressed(GLFW_KEY_Q))
      break;
  }

  // destroy textures
  texture_skybox.free();

  // destroy shaders
  program_terrain.free();
  program_skybox.free();
  program_basic.free();

  // destroy renderers of each shape (frees vao & vbo)
  terrain.free();
  gizmo.free();
  grid.free();

  // destroy window & terminate glfw
  window.destroy();

  return 0;
}
