#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
glm::mat3 lightDirMatrix;
glm::vec3 pointLightPos;

// shader uniform locations
GLuint modelLoc;
GLuint viewLoc;
GLuint projectionLoc;
GLuint normalMatrixLoc;
GLuint lightDirLoc;
GLuint lightColorLoc;
GLuint lightDirMatrixLoc;
GLuint pointLightPosLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 1.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
gps::Model3D ground;
gps::Model3D tank1;
gps::Model3D tank2;
gps::Model3D tank3;
gps::Model3D lightCube;
gps::Model3D barracks;
gps::Model3D dog;
gps::Model3D soldier;
gps::Model3D forest;
gps::Model3D m4;
gps::Model3D barricade;
gps::Model3D lamp;

GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader lightShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool toLeft = true;
bool toRight = false;
float dogX = -3.0f;
float dogZ = -1.0f;

bool toFront = true;
bool toBack = false;
float soldierX = 5.0f;
float soldierZ = -4.0f;

bool move = false;
gps::MOVE_DIRECTION pressedDir;
gps::MOVE_DIRECTION reverseDir;

// mouse
float lastX = myWindow.getWindowDimensions().width, lastY = myWindow.getWindowDimensions().height;
bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;
float sensitivity = 0.1f;

// skybox
gps::SkyBox skybox;
std::vector<const GLchar*> faces;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);

    WindowDimensions dim = { width, height };
    myWindow.setWindowDimensions(dim);

    myBasicShader.useShaderProgram();

    //set projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
    //send matrix data to shader
    GLint projLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //set Viewport transform
    glViewport(0, 0, width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }
    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);

        pressedDir = gps::MOVE_FORWARD;
        reverseDir = gps::MOVE_BACKWARD;
        move = true;
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);

        pressedDir = gps::MOVE_BACKWARD;
        reverseDir = gps::MOVE_FORWARD;
        move = true;
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);

        pressedDir = gps::MOVE_LEFT;
        reverseDir = gps::MOVE_RIGHT;
        move = true;
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

        pressedDir = gps::MOVE_RIGHT;
        reverseDir = gps::MOVE_LEFT;
        move = true;
	}

    if (move) {
        glm::vec3 position = myCamera.getCameraPosition();
        //printf("%f %f %f\n", position.x, position.y, position.z);

        // Tank collision
        if (position.x > -2.7f && position.x < 4.85f && position.y > 0.0f && position.y < 2.5f && position.z > -7.6f && position.z < -3.6f) {
            if (pressedDir != reverseDir) {
                myCamera.move(reverseDir, cameraSpeed);
            }
        }
        
        // Barrack collision
        if (position.x > -16.7f && position.x < -5.9f && position.y > 0.0f && position.y < 5.78f && position.z > 1.1f && position.z < 7.0f) {
            if (pressedDir != reverseDir) {
                myCamera.move(reverseDir, cameraSpeed);
            }
        }
    }

    if (pressedKeys[GLFW_KEY_P]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (pressedKeys[GLFW_KEY_O]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (pressedKeys[GLFW_KEY_I]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    move = false;
}

void initSkyBoxFaces() {
    faces.push_back("textures/skybox_custom/oasisnight_rt.tga");
    faces.push_back("textures/skybox_custom/oasisnight_lf.tga");
    faces.push_back("textures/skybox_custom/oasisnight_up.tga");
    faces.push_back("textures/skybox_custom/oasisnight_dn.tga");
    faces.push_back("textures/skybox_custom/oasisnight_bk.tga");
    faces.push_back("textures/skybox_custom/oasisnight_ft.tga");
}

glm::mat4 computeLightSpaceTrMatrix() {
    // Return the light-space transformation matrix
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(-1.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 1.0f, far_plane = 35.0f;
    glm::mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -8.0f, 8.0f, near_plane, far_plane);

    return lightProjection * lightView;
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    ground.LoadModel("models/ground_highres/ground.obj", "models/ground_highres/");
    tank1.LoadModel("models/tanks/tank1.obj", "models/tanks/");
    tank2.LoadModel("models/tanks/tank2.obj", "models/tanks/");
    tank3.LoadModel("models/tanks/tank3.obj", "models/tanks/");
    lightCube.LoadModel("models/cube/cube.obj", "models/cube/");
    barracks.LoadModel("models/barrack/barrack.obj", "models/barrack/");
    dog.LoadModel("models/dog/dog.obj", "models/dog/");
    soldier.LoadModel("models/soldier/soldier.obj", "models/soldier/");
    forest.LoadModel("models/forest/trees.obj", "models/forest/");
    m4.LoadModel("models/gun_m4/m4.obj", "models/gun_m4/");
    barricade.LoadModel("models/barricade/barricade.obj", "models/barricade/");
    lamp.LoadModel("models/lamp/lamp.obj", "models/lamp/");
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initFBO() {
    // Create the FBO, the depth texture and attach the depth texture to the FBO
    glGenFramebuffers(1, &shadowMapFBO);

    // create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
        0);

    // bind nothing to color and stencil attachments
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // one FBO complete => unbind until use
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void initUniforms() {
	myBasicShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    lightDirMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix");
    pointLightPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 200.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(1.0f, 8.0f, -15.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // point light ----------------------------------------------------------------------------------------------
    myBasicShader.useShaderProgram();
    pointLightPos = glm::vec3(2.0f, 1.2f, -2.3f);
    glUniform3fv(pointLightPosLoc, 1, glm::value_ptr(pointLightPos));

	// first pass ----------------------------------------------------------------------------------------------
    depthMapShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // TANKS ----------------
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model));
    tank1.Draw(depthMapShader);
    tank2.Draw(depthMapShader);
    model = glm::translate(model, glm::vec3(-13.0f, 0.0f, -18.0f));
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model));
    tank3.Draw(depthMapShader);

    // BARRACKS ----------------
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model));
    barracks.Draw(depthMapShader);

    // M4 ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.33f, 0.0f, 2.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model));
    m4.Draw(depthMapShader);

    // BARRIACDE ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 0.0f, 4.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.015, 0.015, 0.015));
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model));
    barricade.Draw(depthMapShader);

    // GROUND ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(model));
    ground.Draw(depthMapShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);




	// second pass ----------------------------------------------------------------------------------------------
    myBasicShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"),
        1,
        GL_FALSE,
        glm::value_ptr(view));

    lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
    glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    myBasicShader.useShaderProgram();

    //bind the depth map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    // TANKS ----------------
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    tank1.Draw(myBasicShader);
    tank2.Draw(myBasicShader);
    model = glm::translate(model, glm::vec3(-13.0f, 0.0f, -18.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    tank3.Draw(myBasicShader);

    // BARRACKS ----------------
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    barracks.Draw(myBasicShader);

    // FOREST ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-21.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    forest.Draw(myBasicShader);
    
    // DOG ----------------
    // animate dog
    if (toLeft) {
        if (dogZ >= 3.0f) {
            toLeft = false;
            toRight = true;
        }
        else {
            dogZ += 0.03f;
        }
    }
    else if (toRight) {
        if (dogZ <= -3.0f) {
            toLeft = true;
            toRight = false;
        }
        else {
            dogZ -= 0.03f;
        }
    } 
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.0f, dogZ));
    if (toLeft) {
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else {
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    model = glm::scale(model, glm::vec3(0.028f, 0.028f, 0.028f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    dog.Draw(myBasicShader);

    // SOLDIER ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -3.0f));
    model = glm::rotate(model, glm::radians(85.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    soldier.Draw(myBasicShader);

    // M4 ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.33f, 0.0f, 2.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    m4.Draw(myBasicShader);

    // BARRICADE ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 0.0f, 4.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.015, 0.015, 0.015));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    barricade.Draw(myBasicShader);

    // LAMP ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(1.93f, 1.05f, -2.2f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    lamp.Draw(myBasicShader);

    // GROUND ----------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    ground.Draw(myBasicShader);

    // LIGHTCUBE ----------------
    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    model = glm::translate(glm::mat4(1.0f), 1.0f * lightDir);
    model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    lightCube.Draw(lightShader);

    // SKYBOX ----------------
    skybox.Draw(skyboxShader, view, projection);

}

void cleanup() {
    myWindow.Delete();

    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initSkyBoxFaces();
    skybox.Load(faces);

    initOpenGLState();
    initFBO();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
