#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <ctime>
#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Material.h"

using namespace std;

int numberOfItems;
GLFWwindow* window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

bool keyToggles[256] = { false }; // only for English keyboards!
int shader = 0;
int materialIndex = 0;
int lightAmount = 10;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Program> progRevolution;

shared_ptr<Shape> shape;
shared_ptr<Shape> shapeTeapot;
shared_ptr<Shape> shape_grass;
shared_ptr<Shape> shape_sun;
shared_ptr<Shape> shape_frustum;
shared_ptr<Shape> shape_revolution;

vector<Material> materials;
vector<glm::vec4> itemProperties;

glm::vec3 lightP[10];
glm::vec3 lightC[10];

static void draw_obj(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float spec, shared_ptr<MatrixStack> P, 
                    shared_ptr<MatrixStack> MV, shared_ptr<Shape> sh, glm::vec3 translate, glm::vec3 rotate, 
                    glm::vec3 scale, float rotateAmount, bool shear, shared_ptr<Program> program_draw, bool revolves) {

    glm::vec3 *camLightP = new glm::vec3[lightAmount];
    for (int i = 0; i < lightAmount; i++) {
        camLightP[i] = MV->topMatrix() * glm::vec4(lightP[i], 1.0);
    }

    MV->translate(translate);
    MV->scale(scale);
    if (shear) {
        glm::mat4 S(1.0f);
        S[1][0] = cos(rotateAmount);
        MV->multMatrix(S);
    }
    else {
        MV->rotate(rotateAmount, rotate);
    }
    glUniformMatrix4fv(program_draw->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
    glUniformMatrix4fv(program_draw->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
    glUniformMatrix4fv(program_draw->getUniform("itMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(MV->topMatrix()))));
    glUniform3f(program_draw->getUniform("ka"), ambient.r, ambient.g, ambient.b);
    glUniform3f(program_draw->getUniform("kd"), diffuse.r, diffuse.g, diffuse.b);
    glUniform3f(program_draw->getUniform("ks"), specular.r, specular.g, specular.b);
    glUniform1f(program_draw->getUniform("s"), spec);
    if (revolves) {
        cout << rotateAmount << endl;
        glUniform1f(program_draw->getUniform("t"), glfwGetTime());
    }

    glUniform3fv(program_draw->getUniform("lightPositions"), lightAmount, glm::value_ptr(camLightP[0]));
    glUniform3fv(program_draw->getUniform("lightColors"), lightAmount, glm::value_ptr(lightC[0]));


    sh->draw(program_draw, revolves);
}
// This function is called when a GLFW error occurs
static void error_callback(int error, const char* description)
{
    cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Get the current mouse position.
    double xmouse, ymouse;
    glfwGetCursorPos(window, &xmouse, &ymouse);
    // Get current window size.
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (action == GLFW_PRESS) {
        bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        bool alt = (mods & GLFW_MOD_ALT) != 0;
        camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
    }
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS) {
        camera->mouseMoved((float)xmouse, (float)ymouse);
    }
}

static void char_callback(GLFWwindow* window, unsigned int key)
{
    keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char* filepath, GLFWwindow* w)
{
    int width, height;
    glfwGetFramebufferSize(w, &width, &height);
    GLsizei nrChannels = 3;
    GLsizei stride = nrChannels * width;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    GLsizei bufferSize = stride * height;
    std::vector<char> buffer(bufferSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    stbi_flip_vertically_on_write(true);
    int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
    if (rc) {
        cout << "Wrote to " << filepath << endl;
    }
    else {
        cout << "Couldn't write to " << filepath << endl;
    }
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
    // Initialize time.
    glfwSetTime(0.0);

    // Set background color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    
    // Enable z-buffer test.
    glEnable(GL_DEPTH_TEST);

    numberOfItems = 100;
    prog = make_shared<Program>();
    prog->setShaderNames(RESOURCE_DIR + "normal_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
    prog->setVerbose(true);
    prog->init();
    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    prog->addUniform("MV");
    prog->addUniform("lightPositions");
    prog->addUniform("lightColors");
	prog->addUniform("itMV");
    prog->addUniform("P");
    prog->addUniform("ka");
    prog->addUniform("kd");
    prog->addUniform("ks");
    prog->addUniform("s");
    prog->setVerbose(false);

    progRevolution = make_shared<Program>();
    progRevolution->setShaderNames(RESOURCE_DIR + "revolution_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
    progRevolution->setVerbose(true);
    progRevolution->init();
    progRevolution->addAttribute("aPos");
    progRevolution->addAttribute("aNor");
    progRevolution->addUniform("MV");
    progRevolution->addUniform("lightPositions");
    progRevolution->addUniform("lightColors");
	progRevolution->addUniform("itMV");
	progRevolution->addUniform("t");
    progRevolution->addUniform("P");
    progRevolution->addUniform("ka");
    progRevolution->addUniform("kd");
    progRevolution->addUniform("ks");
    progRevolution->addUniform("s");
    progRevolution->setVerbose(false);

    camera = make_shared<Camera>();
    camera->setInitDistance(2.0f); // Camera's initial Z translation

    shape = make_shared<Shape>();
    shape->loadMesh(RESOURCE_DIR + "bunny.obj", false);
    shape->init();

    shapeTeapot = make_shared<Shape>();
    shapeTeapot->loadMesh(RESOURCE_DIR + "teapot.obj", false);
    shapeTeapot->init();

    shape_grass = make_shared<Shape>();
    shape_grass->loadMesh(RESOURCE_DIR + "grass.obj", true);
    shape_grass->init();

    shape_sun = make_shared<Shape>();
    shape_sun->loadMesh(RESOURCE_DIR + "sphere.obj", false);
    shape_sun->init();

	vector<float> posBuf;
	vector<float> norBuf;
	vector<float> texBuf;
	vector<unsigned int> indBuf;
    int side_amount = 50;
    for (int i = 0; i < side_amount; i++) {
        for (int j = 0; j < side_amount; j++) {
            float theta = 2 * M_PI * i / (side_amount - 1);
            float x = 10 * (float) j / (side_amount - 1);

            posBuf.push_back(x);
            posBuf.push_back(theta);
            posBuf.push_back(0.0f);

            norBuf.push_back(0.0f);
            norBuf.push_back(0.0f);
            norBuf.push_back(0.0f);

        }
    }
    for (int i = 0; i < side_amount - 1; i++) {
        for (int j = 0; j < side_amount - 1; j++) {
            indBuf.push_back(i * (side_amount) + j);
            indBuf.push_back(i * (side_amount) + (j + 1));
            indBuf.push_back((i + 1) * (side_amount) + (j + 1));
            indBuf.push_back(i * (side_amount) + j);
            indBuf.push_back((i + 1) * (side_amount) + (j + 1));
            indBuf.push_back((i + 1) * (side_amount) + j);
        }
    }
    shape_revolution = make_shared<Shape>();
    shape_revolution->insertBuffers(posBuf, norBuf, indBuf);
    shape_revolution->init();

    glm::vec3 kaPink(.2f, .2f, .2f);
    glm::vec3 kdPink(.8f, .7f, .7f);
    glm::vec3 ksPink(1.0f, .9f, .8f);
    float sPink = 200.0f;
    materials.push_back(Material(kaPink, kdPink, ksPink, sPink));

    glm::vec3 kaBlue(.1f, .1f, .1f);
    glm::vec3 kdBlue(.0f, .1f, .9f);
    glm::vec3 ksBlue(.0f, .6f, 1.0f);
    float sBlue = 200.0f;
    materials.push_back(Material(kaBlue, kdBlue, ksBlue, sBlue));

    glm::vec3 kaMetal(.0f, .0f, .0f);
    glm::vec3 kdMetal(.65f, .65f, .8f);
    glm::vec3 ksMetal(.1f, .1f, .1f);
    float sMetal = 20.0f;
    materials.push_back(Material(kaMetal, kdMetal, ksMetal, sMetal));

    std::srand(std::time(nullptr));
    for (int i = 0; i < numberOfItems; i++) {
        float randomFloatRed = (float) std::rand() / RAND_MAX;
        float randomFloatGreen = (float) std::rand() / RAND_MAX;
        float randomFloatBlue = (float) std::rand() / RAND_MAX;
        float randomSize = (float) std::rand() / RAND_MAX;
        itemProperties.push_back(glm::vec4(randomFloatRed, randomFloatGreen, randomFloatBlue, randomSize));
    }

    for (int i = 0; i < lightAmount; i++) {
        float randomFloatOne = (float) std::rand() / RAND_MAX;
        float randomFloatTwo = (float) std::rand() / RAND_MAX;
        float randomFloatThree = (float) std::rand() / RAND_MAX;
        lightP[i] = glm::vec3(-5 + randomFloatOne * 10, .25, -5 + randomFloatThree * 10);
        lightC[i] = glm::vec3(randomFloatOne, randomFloatTwo, randomFloatThree);
    }

    GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
    // Clear framebuffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (keyToggles[(unsigned)'c']) {
        glEnable(GL_CULL_FACE);
    }
    else {
        glDisable(GL_CULL_FACE);
    }

    if (keyToggles[(unsigned)'z']) {
        camera->changeFOVY(true);
        keyToggles[(unsigned) 'z'] = false;
    }
    if (keyToggles[(unsigned)'Z']) {
        camera->changeFOVY(false);
        keyToggles[(unsigned) 'Z'] = false;
    }

    if (keyToggles[(unsigned) 'w']) {
        camera->wasd('w');
        keyToggles[(unsigned) 'w'] = false;
    }
    if (keyToggles[(unsigned) 'a']) {
        camera->wasd('a');
        keyToggles[(unsigned) 'a'] = false;
    }
    if (keyToggles[(unsigned) 's']) {
        camera->wasd('s');
        keyToggles[(unsigned) 's'] = false;
    }
    if (keyToggles[(unsigned) 'd']) {
        camera->wasd('d');
        keyToggles[(unsigned) 'd'] = false;
    }

        // Get current frame buffer size.
        int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    camera->setAspect((float)width / (float)height);
    float aspect_ratio = (float) width / (float) height;
    double t = glfwGetTime();


    glViewport(0,0, width, height);

    // Matrix stacks
    auto P = make_shared<MatrixStack>();
    auto MV = make_shared<MatrixStack>();
    Material chosenMaterial = materials[materialIndex];



    // Apply camera transforms
    prog->bind();
    P->pushMatrix();
    MV->pushMatrix();
    camera->applyProjectionMatrix(P);
    camera->applyViewMatrix(MV);

    //DRAWING LIGHT SOURCES
    for (int i = 0; i < lightAmount; i++) {
        MV->pushMatrix();
            glm::vec3 translate(lightP[i]);
            glm::vec3 scale(.01);
            draw_obj(lightC[i], glm::vec3(0), glm::vec3(0), 5, P, MV, shape_sun, translate, glm::vec3(1), scale, 0, false, prog, false);
        MV->popMatrix();
    }

    for (int i = 0; i < numberOfItems; i++) {
        glm::vec3 currentColor = itemProperties[i];
        float scaleAmount = itemProperties[i][3] / 5;

        //DRAWING BUNNIES AND TEAPOTS
        float t0 = t * itemProperties[i][3] / (3 / 2) + t * .3;
        if (i % 4 == 0) {
            MV->pushMatrix();
                glm::vec3 translate(-1.0f * (i / 10) + 5, (-.333099f) * (.2 + scaleAmount), 1.0f * (i % 10) - 5);
                glm::vec3 rotate(0.0f, 1.0f, 0.0f);

                glm::vec3 scale(.2 + scaleAmount);
                draw_obj(glm::vec3(0), currentColor, glm::vec3(1), 10, P, MV, shape, translate, rotate, scale, t0, false, prog, false);
            MV->popMatrix();
        }
        else if (i % 4 == 1) {
            MV->pushMatrix();
                glm::vec3 translate(-1.0f * (i / 10) + 5, 0, 1.0f * (i % 10) - 5);
                glm::vec3 scale(.2 + scaleAmount);

                draw_obj(glm::vec3(0), currentColor, glm::vec3(1), 10, P, MV, shapeTeapot, translate, glm::vec3(1), scale, t0, true, prog, false);
            MV->popMatrix();
        }
        else if (i % 4 == 2) {
            MV->pushMatrix();
                glm::vec3 translate(-1.0f * (i / 10) + 5, 1.3 * (.5 * sin((6.28 / 1.7) * (t0)) + .5) + .5 * .06, 1.0f * (i % 10) - 5);
                glm::vec3 scale(.06 -.03 * (.5 * cos((4 * 3.14 / 1.7) * (t0)) + 1/2), .06, .06 -.03 * (.5 * cos((4 * 3.14 / 1.7) * (t0)) + 1/2));

                draw_obj(glm::vec3(0), currentColor, glm::vec3(1), 10, P, MV, shape_sun, translate, glm::vec3(1), scale, 0, false, prog, false);
            MV->popMatrix();   
        }
        else {
            progRevolution->bind();

            MV->pushMatrix();
                glm::vec3 translate(-1.0f * (i / 10) + 5, 0, 1.0f * (i % 10) - 5);
                glm::vec3 rotate(0, 0, 1);
                glm::vec3 scale(.035, .035, .035);

                draw_obj(glm::vec3(0), currentColor, glm::vec3(1), 10, P, MV, shape_revolution, translate, rotate, scale, M_PI / 2, false, progRevolution, true);
            MV->popMatrix();

            prog->bind();
        }
    }

    //DRAWING PLANE
    MV->pushMatrix();
        glm::vec3 translate(0, 0, -1);
        draw_obj(glm::vec3(0, 0, 0), chosenMaterial.kd, chosenMaterial.ks, chosenMaterial.s, P, MV, shape_grass, translate, glm::vec3(1), glm::vec3(1), 0, false, prog, false);
    MV->popMatrix();


    MV->popMatrix();
    P->popMatrix();
    prog->unbind();

    if (OFFLINE) {
        saveImage("output.png", window);
        GLSL::checkError(GET_FILE_LINE);
        glfwSetWindowShouldClose(window, true);
    }
    GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        cout << "Usage: A3 RESOURCE_DIR" << endl;
        return 0;
    }
    RESOURCE_DIR = argv[1] + string("/");

    // Optional argument
    if (argc >= 3) {
        OFFLINE = atoi(argv[2]) != 0;
    }

    // Set error callback.
    glfwSetErrorCallback(error_callback);
    // Initialize the library.
    if (!glfwInit()) {
        return -1;
    }
    // Create a windowed mode window and its OpenGL context.
    window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    // Make the window's context current.
    glfwMakeContextCurrent(window);
    // Initialize GLEW.
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        return -1;
    }
    glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    GLSL::checkVersion();
    // Set vsync.
    glfwSwapInterval(1);
    // Set keyboard callback.
    glfwSetKeyCallback(window, key_callback);
    // Set char callback.
    glfwSetCharCallback(window, char_callback);
    // Set cursor position callback.
    glfwSetCursorPosCallback(window, cursor_position_callback);
    // Set mouse button callback.
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    // Set the window resize call back.
    glfwSetFramebufferSizeCallback(window, resize_callback);
    // Initialize scene.
    init();
    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(window)) {
        // Render scene.
        render();
        // Swap front and back buffers.
        glfwSwapBuffers(window);
        // Poll for and process events.
        glfwPollEvents();
    }
    // Quit program.
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
