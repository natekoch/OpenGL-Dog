/* project2B.cxx
 * Nate Koch
 * 2/23/23
 * Gray dog (or my attempt at Dante from Pixar's Coco) modeled in OpenGL.
 * Animations featured head tilt and tail wag.
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>

using std::endl;
using std::cerr;

#include <GL/glew.h>    // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>   // glm::vec3
#include <glm/vec4.hpp>   // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale

class RenderManager;

void        SetUpDog(int, RenderManager &);
const char *GetVertexShader();
const char *GetFragmentShader();

// This file is split into four parts:
// - Part 1: code to set up spheres and cylinders
// - Part 2: a "RenderManager" module
// - Part 3: main function
// - Part 4: SetUpDog and the shader programs -- things you modify
//
// It is intended that you will only need to modify code in Part 4.
// That said, you will need functions in Part 2 and should review
// those functions.
// Further, you are encouraged to look through the entire code base.
//


//
//
// PART 1: code to set up spheres and cylinders
//
//

class Triangle
{
  public:
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
};

std::vector<Triangle> SplitTriangle(std::vector<Triangle> &list)
{
    std::vector<Triangle> output(4*list.size());
    output.resize(4*list.size());
    for (unsigned int i = 0 ; i < list.size() ; i++)
    {
        Triangle t = list[i];
        glm::vec3 vmid1, vmid2, vmid3;
        vmid1 = (t.v0 + t.v1) / 2.0f;
        vmid2 = (t.v1 + t.v2) / 2.0f;
        vmid3 = (t.v0 + t.v2) / 2.0f;
        output[4*i+0].v0 = t.v0;
        output[4*i+0].v1 = vmid1;
        output[4*i+0].v2 = vmid3;
        output[4*i+1].v0 = t.v1;
        output[4*i+1].v1 = vmid2;
        output[4*i+1].v2 = vmid1;
        output[4*i+2].v0 = t.v2;
        output[4*i+2].v1 = vmid3;
        output[4*i+2].v2 = vmid2;
        output[4*i+3].v0 = vmid1;
        output[4*i+3].v1 = vmid2;
        output[4*i+3].v2 = vmid3;
    }
    return output;
}

void PushVertex(std::vector<float>& coords,
                const glm::vec3& v)
{
  coords.push_back(v.x);
  coords.push_back(v.y);
  coords.push_back(v.z);
}

//
// Sets up a cylinder that is the circle x^2+y^2=1 extruded from
// Z=0 to Z=1.
//
void GetCylinderData(std::vector<float>& coords, std::vector<float>& normals)
{
  int nfacets = 30;
  for (int i = 0 ; i < nfacets ; i++)
  {
    double angle = 3.14159*2.0*i/nfacets;
    double nextAngle = (i == nfacets-1 ? 0 : 3.14159*2.0*(i+1)/nfacets);
    glm::vec3 fnormal(0.0f, 0.0f, 1.0f);
    glm::vec3 bnormal(0.0f, 0.0f, -1.0f);
    glm::vec3 fv0(0.0f, 0.0f, 1.0f);
    glm::vec3 fv1(cos(angle), sin(angle), 1);
    glm::vec3 fv2(cos(nextAngle), sin(nextAngle), 1);
    glm::vec3 bv0(0.0f, 0.0f, 0.0f);
    glm::vec3 bv1(cos(angle), sin(angle), 0);
    glm::vec3 bv2(cos(nextAngle), sin(nextAngle), 0);
    // top and bottom circle vertices
    PushVertex(coords, fv0);
    PushVertex(normals, fnormal);
    PushVertex(coords, fv1);
    PushVertex(normals, fnormal);
    PushVertex(coords, fv2);
    PushVertex(normals, fnormal);
    PushVertex(coords, bv0);
    PushVertex(normals, bnormal);
    PushVertex(coords, bv1);
    PushVertex(normals, bnormal);
    PushVertex(coords, bv2);
    PushVertex(normals, bnormal);
    // curves surface vertices
    glm::vec3 v1normal(cos(angle), sin(angle), 0);
    glm::vec3 v2normal(cos(nextAngle), sin(nextAngle), 0);
    //fv1 fv2 bv1
    PushVertex(coords, fv1);
    PushVertex(normals, v1normal);
    PushVertex(coords, fv2);
    PushVertex(normals, v2normal);
    PushVertex(coords, bv1);
    PushVertex(normals, v1normal);
    //fv2 bv1 bv2
    PushVertex(coords, fv2);
    PushVertex(normals, v2normal);
    PushVertex(coords, bv1);
    PushVertex(normals, v1normal);
    PushVertex(coords, bv2);
    PushVertex(normals, v2normal);
  }
}

//
// Sets up a sphere with equation x^2+y^2+z^2=1
//
void
GetSphereData(std::vector<float>& coords, std::vector<float>& normals)
{
  int recursionLevel = 3;
  std::vector<Triangle> list;
  {
    Triangle t;
    t.v0 = glm::vec3(1.0f,0.0f,0.0f);
    t.v1 = glm::vec3(0.0f,1.0f,0.0f);
    t.v2 = glm::vec3(0.0f,0.0f,1.0f);
    list.push_back(t);
  }
  for (int r = 0 ; r < recursionLevel ; r++)
  {
      list = SplitTriangle(list);
  }

  for (int octant = 0 ; octant < 8 ; octant++)
  {
    glm::mat4 view(1.0f);
    float angle = 90.0f*(octant%4);
    if(angle != 0.0f)
      view = glm::rotate(view, glm::radians(angle), glm::vec3(1, 0, 0));
    if (octant >= 4)
      view = glm::rotate(view, glm::radians(180.0f), glm::vec3(0, 0, 1));
    for(int i = 0; i < list.size(); i++)
    {
      Triangle t = list[i];
      float mag_reci;
      glm::vec3 v0 = view*glm::vec4(t.v0, 1.0f);
      glm::vec3 v1 = view*glm::vec4(t.v1, 1.0f);
      glm::vec3 v2 = view*glm::vec4(t.v2, 1.0f);
      mag_reci = 1.0f / glm::length(v0);
      v0 = glm::vec3(v0.x * mag_reci, v0.y * mag_reci, v0.z * mag_reci);
      mag_reci = 1.0f / glm::length(v1);
      v1 = glm::vec3(v1.x * mag_reci, v1.y * mag_reci, v1.z * mag_reci);
      mag_reci = 1.0f / glm::length(v2);
      v2 = glm::vec3(v2.x * mag_reci, v2.y * mag_reci, v2.z * mag_reci);
      PushVertex(coords, v0);
      PushVertex(coords, v1);
      PushVertex(coords, v2);
      PushVertex(normals, v0);
      PushVertex(normals, v1);
      PushVertex(normals, v2);
    }
  }
}


//
//
// PART 2: RenderManager module
//
//

void _print_shader_info_log(GLuint shader_index) {
  int max_length = 2048;
  int actual_length = 0;
  char shader_log[2048];
  glGetShaderInfoLog(shader_index, max_length, &actual_length, shader_log);
  printf("shader info log for GL index %u:\n%s\n", shader_index, shader_log);
}

class RenderManager
{
  public:
   enum ShapeType
   {
      SPHERE,
      CYLINDER
   };

                 RenderManager();
   void          SetView(glm::vec3 &c, glm::vec3 &, glm::vec3 &);
   void          SetUpGeometry();
   void          SetColor(double r, double g, double b);
   void          Render(ShapeType, glm::mat4 model);
   GLFWwindow   *GetWindow() { return window; };

  private:
   glm::vec3 color;
   GLuint sphereVAO;
   GLuint sphereNumPrimitives;
   GLuint cylinderVAO;
   GLuint cylinderNumPrimitives;
   GLuint mloc;
   GLuint mvploc;
   GLuint colorloc;
   GLuint camloc;
   GLuint ldirloc;
   glm::mat4 projection;
   glm::mat4 view;
   GLuint shaderProgram;
   GLFWwindow *window;

   void SetUpWindowAndShaders();
   void MakeModelView(glm::mat4 &);
};

RenderManager::RenderManager()
{
  SetUpWindowAndShaders();
  SetUpGeometry();
  projection = glm::perspective(
        glm::radians(45.0f), (float)1000 / (float)1000,  5.0f, 100.0f);

  // Get a handle for our MVP and color uniforms
  mloc = glGetUniformLocation(shaderProgram, "M");
  mvploc = glGetUniformLocation(shaderProgram, "MVP");
  colorloc = glGetUniformLocation(shaderProgram, "color");
  camloc = glGetUniformLocation(shaderProgram, "cameraloc");
  ldirloc = glGetUniformLocation(shaderProgram, "lightdir");

  glm::vec4 lightcoeff(0.3, 0.7, 2.8, 50.5); // Lighting coeff, Ka, Kd, Ks, alpha
  GLuint lcoeloc = glGetUniformLocation(shaderProgram, "lightcoeff");
  glUniform4fv(lcoeloc, 1, &lightcoeff[0]);
}

void
RenderManager::SetView(glm::vec3 &camera, glm::vec3 &origin, glm::vec3 &up)
{ 
   glm::mat4 v = glm::lookAt(
                       camera, // Camera in world space
                       origin, // looks at the origin
                       up      // and the head is up
                 );
   view = v; 
   glUniform3fv(camloc, 1, &camera[0]);
   // Direction of light
   glm::vec3 lightdir = glm::normalize(camera - origin);   
   glUniform3fv(ldirloc, 1, &lightdir[0]);
};

void
RenderManager::SetUpWindowAndShaders()
{
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(700, 700, "CIS 441", NULL, NULL);
  if (!window) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);
  // start GLEW extension handler
  glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte *version = glGetString(GL_VERSION);   // version as a string
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);

  // tell GL to only draw onto a pixel if the shape is closer to the viewer
  glEnable(GL_DEPTH_TEST); // enable depth-testing
  glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

  const char* vertex_shader = GetVertexShader();
  const char* fragment_shader = GetFragmentShader();

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  glCompileShader(vs);
  int params = -1;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
    _print_shader_info_log(vs);
    exit(EXIT_FAILURE);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
    _print_shader_info_log(fs);
    exit(EXIT_FAILURE);
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, fs);
  glAttachShader(shaderProgram, vs);
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);
}

void RenderManager::SetColor(double r, double g, double b)
{
   color[0] = r;
   color[1] = g;
   color[2] = b;
}

void RenderManager::MakeModelView(glm::mat4 &model)
{
   glm::mat4 modelview = projection * view * model;
   glm::mat4 M = model;
   glUniformMatrix4fv(mloc, 1, GL_FALSE, &M[0][0]);
   glUniformMatrix4fv(mvploc, 1, GL_FALSE, &modelview[0][0]);
}

void RenderManager::Render(ShapeType st, glm::mat4 model)
{
   int numPrimitives = 0;
   if (st == SPHERE)
   {
      glBindVertexArray(sphereVAO);
      numPrimitives = sphereNumPrimitives;
   }
   else if (st == CYLINDER)
   {
      glBindVertexArray(cylinderVAO);
      numPrimitives = cylinderNumPrimitives;
   }
   MakeModelView(model);
   glUniform3fv(colorloc, 1, &color[0]);
   glDrawElements(GL_TRIANGLES, numPrimitives, GL_UNSIGNED_INT, NULL);
}

void SetUpVBOs(std::vector<float> &coords, std::vector<float> &normals,
               GLuint &points_vbo, GLuint &normals_vbo, GLuint &index_vbo)
{
  int numIndices = coords.size()/3;
  std::vector<GLuint> indices(numIndices);
  for(int i = 0; i < numIndices; i++)
    indices[i] = i;

  points_vbo = 0;
  glGenBuffers(1, &points_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
  glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(float), coords.data(), GL_STATIC_DRAW);

  normals_vbo = 0;
  glGenBuffers(1, &normals_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);

  index_vbo = 0;    // Index buffer object
  glGenBuffers(1, &index_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
}

void RenderManager::SetUpGeometry()
{
  std::vector<float> sphereCoords;
  std::vector<float> sphereNormals;
  GetSphereData(sphereCoords, sphereNormals);
  sphereNumPrimitives = sphereCoords.size() / 3;
  GLuint sphere_points_vbo, sphere_normals_vbo, sphere_indices_vbo;
  SetUpVBOs(sphereCoords, sphereNormals, 
            sphere_points_vbo, sphere_normals_vbo, sphere_indices_vbo);

  std::vector<float> cylCoords;
  std::vector<float> cylNormals;
  GetCylinderData(cylCoords, cylNormals);
  cylinderNumPrimitives = cylCoords.size() / 3;
  GLuint cyl_points_vbo, cyl_normals_vbo, cyl_indices_vbo;
  SetUpVBOs(cylCoords, cylNormals, 
            cyl_points_vbo, cyl_normals_vbo, cyl_indices_vbo);

  GLuint vao[2];
  glGenVertexArrays(2, vao);

  glBindVertexArray(vao[SPHERE]);
  sphereVAO = vao[SPHERE];
  glBindBuffer(GL_ARRAY_BUFFER, sphere_points_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_normals_vbo);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_indices_vbo);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glBindVertexArray(vao[CYLINDER]);
  cylinderVAO = vao[CYLINDER];
  glBindBuffer(GL_ARRAY_BUFFER, cyl_points_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, cyl_normals_vbo);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cyl_indices_vbo);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
}

//
// PART3: main function
//
int main() 
{
  RenderManager rm;
  GLFWwindow *window = rm.GetWindow();

  glm::vec3 origin(0, 0, 0);
  glm::vec3 up(0, 1, 0);

  int counter=0;
  while (!glfwWindowShouldClose(window)) 
  {
    double angle=counter/300.0*2*M_PI;
    counter++;

    glm::vec3 camera(10*sin(angle), 0, 10*cos(angle));
    //glm::vec3 camera(0, 0, 10);
    rm.SetView(camera, origin, up);

    // wipe the drawing surface clear
    glClearColor(0.3, 0.3, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SetUpDog(counter, rm);

    // update other events like input handling
    glfwPollEvents();
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers(window);

    usleep(15000);
  }

  // close GL context and any other GLFW resources
  glfwTerminate();
  return 0;
}

glm::mat4 RotateMatrix(float degrees, float x, float y, float z)
{
   glm::mat4 identity(1.0f);
   glm::mat4 rotation = glm::rotate(identity, 
                                    glm::radians(degrees), 
                                    glm::vec3(x, y, z));
   return rotation;
}

glm::mat4 ScaleMatrix(double x, double y, double z)
{
   glm::mat4 identity(1.0f);
   glm::vec3 scale(x, y, z);
   return glm::scale(identity, scale);
}

glm::mat4 TranslateMatrix(double x, double y, double z)
{
   glm::mat4 identity(1.0f);
   glm::vec3 translate(x, y, z);
   return glm::translate(identity, translate);
}

void SetUpEyeball(glm::mat4 modelSoFar, RenderManager &rm)
{
   glm::mat4 scaled10 = ScaleMatrix(0.1, 0.1, 0.1);
   rm.SetColor(1,1,1);
   rm.Render(RenderManager::SPHERE, modelSoFar*scaled10);

   glm::mat4 translateNew = TranslateMatrix(0, 0, 0.07);
   glm::mat4 scaled20 = ScaleMatrix(0.05, 0.05, 0.05);
   rm.SetColor(1, 0.5, 0);
   rm.Render(RenderManager::SPHERE, modelSoFar*translateNew*scaled20);

   glm::mat4 translate = TranslateMatrix(0, 0, 0.95);
   glm::mat4 scaled30 = ScaleMatrix(0.3, 0.3, 0.3);
   rm.SetColor(0,0,0);
   rm.Render(RenderManager::SPHERE, modelSoFar*scaled10*translateNew*translate*scaled30);
}

void 
SetUpHead(glm::mat4 modelSoFar, RenderManager &rm)
{
    //glm::mat4 translate = TranslateMatrix(0, 1, 2);
    glm::mat4 translate = TranslateMatrix(0, 0, 0);
    
    glm::mat4 leftEyeTranslate = TranslateMatrix(-0.2, 0.25, 0);
    glm::mat4 rotateInFromLeft = RotateMatrix(15, 0, 1, 0);
    SetUpEyeball(modelSoFar*translate*leftEyeTranslate*rotateInFromLeft, rm);

    glm::mat4 rightEyeTranslate = TranslateMatrix(0.2, 0.25, 0);
    glm::mat4 rotateInFromRight = RotateMatrix(-15, 0, 1, 0);
    SetUpEyeball(modelSoFar*translate*rightEyeTranslate*rotateInFromRight, rm);

    glm::mat4 skullScale = ScaleMatrix(0.4, 0.35, 0.4);
    glm::mat4 skullTranslate = TranslateMatrix(0, 0.17, -0.35);
    rm.SetColor(0.4, 0.4, 0.4);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*skullTranslate*skullScale);

    rm.SetColor(0.3, 0.3, 0.3);
    glm::mat4 snoutScale = ScaleMatrix(0.25, 0.13, 0.35);
    glm::mat4 snoutTranslate = TranslateMatrix(0, 0.1, 0);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*snoutTranslate*snoutScale);

    glm::mat4 lowerJawScale = ScaleMatrix(0.25, 0.08, 0.3);
    glm::mat4 lowerJawTranslate = TranslateMatrix(0, 0, 0);
    glm::mat4 lowerJawRotate = RotateMatrix(30, 1, 0, 0);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*lowerJawTranslate*lowerJawRotate*lowerJawScale);

    glm::mat4 earScale = ScaleMatrix(0.13, 0.3, 0.05);

    glm::mat4 leftEarTranslate = TranslateMatrix(0.21, 0.5, -0.33);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*leftEarTranslate*earScale);
    
    glm::mat4 rightEarTranslate = TranslateMatrix(-0.21, 0.5, -0.33);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*rightEarTranslate*earScale);

    glm::mat4 noseScale = ScaleMatrix(0.09, 0.06, 0.05);
    glm::mat4 noseTranslate = TranslateMatrix(0, 0.1, 0.35);
    glm::mat4 noseRotate = RotateMatrix(25, 1, 0, 0);
    rm.SetColor(0, 0, 0);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*noseTranslate*noseRotate*noseScale);

    glm::mat4 tongueScale = ScaleMatrix(0.08, 0.03, 0.5);
    glm::mat4 tongueTranslate = TranslateMatrix(0, 0.065, 0);
    glm::mat4 tongueRotate = RotateMatrix(25, 1, 0, 0);
    rm.SetColor(1, 102/255, 1);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*tongueTranslate*tongueRotate*tongueScale);
}

void 
SetUpLegs(glm::mat4 modelSoFar, RenderManager &rm)
{
  rm.SetColor(0.45, 0.45, 0.45);

  glm::mat4 translate = TranslateMatrix(0,0,0);

  glm::mat4 scaleFrontLeg = ScaleMatrix(0.1, 0.1, 1);
  glm::mat4 rotateFrontLeg = RotateMatrix(90, 1, 0, 0);

  glm::mat4 translateFrontLeftLeg = TranslateMatrix(0.17, 0.3, 1.1);
  rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateFrontLeftLeg*rotateFrontLeg*scaleFrontLeg);

  glm::mat4 translateFrontRightLeg = TranslateMatrix(-0.17, 0.3, 1.1);
  rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateFrontRightLeg*rotateFrontLeg*scaleFrontLeg);

  // Tighs
  glm::mat4 scaleThigh = ScaleMatrix(0.13, 0.13, 0.7);

  glm::mat4 translateLeftThigh = TranslateMatrix(0.17, -0.25, 0.2);
  glm::mat4 rotateLeftThigh = RotateMatrix(30, 0, 1, 0);
  rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateLeftThigh*rotateLeftThigh*scaleThigh);

  glm::mat4 translateRightThigh = TranslateMatrix(-0.17, -0.25, 0.2);
  glm::mat4 rotateRightThigh = RotateMatrix(-30, 0, 1, 0);
  rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateRightThigh*rotateRightThigh*scaleThigh);

  // Shins
  glm::mat4 scaleShin = ScaleMatrix(0.13, 0.13, 0.45);
  glm::mat4 rotateShin = RotateMatrix(90, 1, 0, 0);
  
  glm::mat4 translateLeftShin = TranslateMatrix(0.517, -0.25, 0.8);
  rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateLeftShin*rotateShin*scaleShin);

  glm::mat4 translateRightShin = TranslateMatrix(-0.517, -0.25, 0.8);
  rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateRightShin*rotateShin*scaleShin);

  // Knees
  glm::mat4 scaleKnee = ScaleMatrix(0.133, 0.133, 0.133);

  glm::mat4 translateLeftKnee = TranslateMatrix(0.517, -0.25, 0.8);
  rm.Render(RenderManager::SPHERE, modelSoFar*translate*translateLeftKnee*scaleKnee);

  glm::mat4 translateRightKnee = TranslateMatrix(-0.517, -0.25, 0.8);
  rm.Render(RenderManager::SPHERE, modelSoFar*translate*translateRightKnee*scaleKnee); 
}

void 
SetUpPaws(glm::mat4 modelSoFar, RenderManager &rm)
{
  rm.SetColor(0.3, 0.3, 0.3);

  glm::mat4 translate = TranslateMatrix(0, -0.7, 0);

  glm::mat4 scalePaw = ScaleMatrix(0.17, 0.03, 0.23);

  glm::mat4 translateFrontRightPaw = TranslateMatrix(-0.17, 0, 1.17);
  rm.Render(RenderManager::SPHERE, modelSoFar*translate*translateFrontRightPaw*scalePaw);

  glm::mat4 translateFrontLeftPaw = TranslateMatrix(0.17, 0, 1.17);
  rm.Render(RenderManager::SPHERE, modelSoFar*translate*translateFrontLeftPaw*scalePaw);

  glm::mat4 translateBackRightPaw = TranslateMatrix(-0.517, 0, 0.87);
  rm.Render(RenderManager::SPHERE, modelSoFar*translate*translateBackRightPaw*scalePaw);

  glm::mat4 translateBackLeftPaw = TranslateMatrix(0.517, 0, 0.87);
  rm.Render(RenderManager::SPHERE, modelSoFar*translate*translateBackLeftPaw*scalePaw);
}

void 
SetUpBody(glm::mat4 modelSoFar, RenderManager &rm)
{
    glm::mat4 translate = TranslateMatrix(0, 0.1, 0.57);
    
    rm.SetColor(0.45, 0.45, 0.45);

    // Neck
    glm::mat4 translateNeck = TranslateMatrix(0, 0.65, 0.5);
    glm::mat4 scaleNeck = ScaleMatrix(0.19, 0.19, 0.7);
    glm::mat4 rotateNeck = RotateMatrix(-45, 1, 0, 0);
    rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateNeck*rotateNeck*scaleNeck);

    // Torso
    glm::mat4 scaleTorso = ScaleMatrix(0.5, 0.5, 1);
    glm::mat4 rotateTorso = RotateMatrix(-50, 1, 0, 0);
    rm.Render(RenderManager::SPHERE, modelSoFar*translate*rotateTorso*scaleTorso);
}

void 
SetUpTail(glm::mat4 modelSoFar, RenderManager &rm)
{
    glm::mat4 translate = TranslateMatrix(0, 0, 0);
    
    // Tail
    rm.SetColor(0.3, 0.3, 0.3); 
    glm::mat4 translateTail = TranslateMatrix(0, -0.5, -1.5);
    glm::mat4 scaleTail = ScaleMatrix(0.05, 0.05, 1);
    glm::mat4 rotateTail = RotateMatrix(-100, 0, 0, 1);
    rm.Render(RenderManager::CYLINDER, modelSoFar*translate*translateTail*rotateTail*scaleTail);
}

void
SetUpDog(int counter, RenderManager &rm)
{
    glm::mat4 identity(1.0f);

    double var=(counter%100)/99.0;
    if ((counter/100 % 2) == 1)
       var=1-var;

    glm::mat4 rotateHead = RotateMatrix(-50*sin(var), 0, 0, 1);
    glm::mat4 translateHead = TranslateMatrix(0, 1, 2);
    glm::mat4 headTilt = translateHead*rotateHead;

    glm::mat4 rotateTail = RotateMatrix(20*sin(var), 1, 0, 0);
    glm::mat4 translateTail = TranslateMatrix(0, 0, 0.57);
    glm::mat4 tailWag = translateTail*rotateTail;

    SetUpHead(headTilt, rm);
    SetUpBody(identity, rm);
    SetUpTail(tailWag, rm);
    SetUpLegs(identity, rm);
    SetUpPaws(identity, rm);
}

const char *GetVertexShader()
{
   static char vertexShader[4096];
   strcpy(vertexShader, 
          "#version 400\n"
          "layout (location = 0) in vec3 vertex_position;\n"
          "layout (location = 1) in vec3 vertex_normal;\n"
          "uniform mat4 M;\n"
          "vec3 normal = normalize(mat3(transpose(inverse(M))) * vertex_normal);\n"
          "uniform mat4 MVP;\n"
          "uniform vec3 cameraloc;  // Camera position \n"
          "uniform vec3 lightdir;   // Lighting direction \n"
          "uniform vec4 lightcoeff; // Lighting coeff, Ka, Kd, Ks, alpha\n"
          "vec3 reflection, viewDir;\n"
          "float LdotN, diffuse, RdotV, specular;\n"
          "out float shading_amount;\n"
          "void main() {\n"
          "  vec4 position = vec4(vertex_position, 1.0);\n"
          "  gl_Position = MVP*position;\n"
          "  LdotN = dot(lightdir, normal);\n"
          "  diffuse = lightcoeff[1] * max(0.0, LdotN);\n"
          "  reflection = 2 * LdotN * normal - lightdir;\n"
          "  viewDir = cameraloc-vertex_position;\n"
          "  RdotV = dot(normalize(reflection), normalize(viewDir));\n"
          "  specular = abs(lightcoeff[2] * pow(max(0.0, RdotV), lightcoeff[3]));\n"
          "  shading_amount = lightcoeff[0] + diffuse + specular;\n"
          "}\n"
         );
   return vertexShader;
}

const char *GetFragmentShader()
{
   static char fragmentShader[4096];
   strcpy(fragmentShader, 
          "#version 400\n"
          "uniform vec3 color;\n" 
          "in float shading_amount;\n"
          "out vec4 frag_color;\n"
          "void main() {\n"
          "  frag_color = vec4(color*shading_amount, 1.0);\n"
          "}\n"
         );
   return fragmentShader;
}

