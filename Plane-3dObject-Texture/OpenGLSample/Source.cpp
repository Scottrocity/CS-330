#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "David D. Scott 3-3"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao1;         // Handle for the vertex array object
        GLuint vao2;
        GLuint vao3;
        GLuint vbos1[2];     // Handles for the vertex buffer objects
        GLuint vbos2[2];
        GLuint vbos3[2];
        GLuint nIndices1;    // Number of indices of the mesh
        GLuint nIndices2;
        GLuint nIndices3;
        GLuint nIndices4;
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    //Texture id
    GLuint gTextureId;
    // Shader program
    GLuint gProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
layout(location = 1) in vec4 color;  // Color data from Vertex Attrib Pointer 1

out vec4 vertexColor; // variable to transfer color data to the fragment shader

//Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
    vertexColor = color; // references incoming color data
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec4 vertexColor; // Variable to hold incoming color data from vertex shader

out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(vertexColor);
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "../../resources/textures/smiley.png";
    if (!UCreateTexture(texFilename, gTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {   
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release shader program
    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);
    


    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f * gDeltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        gCamera.ToggleDisplay(0);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

void UKeyButtonCallBack(GLFWwindow* window, string button)
{
    if (button == "p")

        cout << "lah";
           
}

// Functioned called to render a frame
void URender()
{   
    const int nrows = 10;
    const int ncols = 10;
    const int nlevels = 10;

    const float xsize = 10.0f;
    const float ysize = 10.0f;
    const float zsize = 10.0f;

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Scaling, rotation, translation for 1st model
    glm::mat4 scale1 = glm::scale(glm::vec3(15.0f, -2.0f, 15.0f));
    glm::mat4 rotation1 = glm::rotate(40.0f, glm::vec3(0.0, 1.0f, 0.0f));
    glm::mat4 translation1 = glm::translate(glm::vec3(-1.0f, 0.0f, 0.0f));
    glm::mat4 model1 = translation1 * rotation1 * scale1;

    // Scaling, rotation, translation for 2nd model
    glm::mat4 translation2 = glm::translate(glm::vec3(4.0f, 0.0f, 0.0f));//moving Model2 
    glm::mat4 model2 = translation2 * rotation1 * scale1;

    // Scaling, rotation, translation for 3rd model
    glm::mat4 translation3 = glm::translate(glm::vec3(23.0f, 0.0f, 1.0f));//moving Model3. Leave for project
    glm::mat4 scale2 = glm::scale(glm::vec3(5.0f, 2.0f, 5.0f));
    glm::mat4 model3 = translation3 * rotation1 * scale2;

    // Scaling, rotation, translation for 4rd model
    glm::mat4 translation4 = glm::translate(glm::vec3(23.0f, 0.0f, 1.0f));//moving Model4. Leave for project
    glm::mat4 scale3 = glm::scale(glm::vec3(1.0f, 2.0f, 1.0f));
    glm::mat4 model4 = translation4 * rotation1 * scale3;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();


    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    
    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao1);//Plane

    // Linking and drawing Model1
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model1));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//sets color mode to line
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model1

    // Linking and drawing Model2
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));//PLANE
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//sets color mode to fill
    glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model2

    // Linking and drawing Model3
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model3));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//sets color mode to fill
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model3

     // Linking and drawing Model4
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model4));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//sets color mode to fill
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model4

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    glBindVertexArray(gMesh.vao2);//Starting the second VBO, Tree Trunk

    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model1));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//sets color mode to line
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices2, GL_UNSIGNED_SHORT, NULL); // Draws the Model1. Leave for project

    // Linking and drawing Model2
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//sets color mode to fill
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices2, GL_UNSIGNED_SHORT, NULL); // Draws the Model2. Leave for project

    // Linking and drawing Model3
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model3));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//sets color mode to fill
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices2, GL_UNSIGNED_SHORT, NULL); // Draws the Model3. Leave for project

     // Linking and drawing Model4
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model4));//Tree Trunk
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//sets color mode to fill
    glDrawElements(GL_TRIANGLES, gMesh.nIndices2, GL_UNSIGNED_SHORT, NULL); // Draws the Model4. Leave for project

    glBindVertexArray(0);

    glBindVertexArray(gMesh.vao3);//Tree Top

    // Linking and drawing Model1
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model1));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//sets color mode to line
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model1

    // Linking and drawing Model2
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//sets color mode to fill
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model2

    // Linking and drawing Model3
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model3));
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//sets color mode to fill
    //glDrawElements(GL_TRIANGLES, gMesh.nIndices2, GL_UNSIGNED_SHORT, NULL); // Draws the Model3

     // Linking and drawing Model4
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model3));//Tree Top
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//sets color mode to fill
    glDrawElements(GL_TRIANGLES, gMesh.nIndices1, GL_UNSIGNED_SHORT, NULL); // Draws the Model4

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts1[] = {//tree trunk
        // Vertex Positions    // Colors (r,g,b,a)
        -0.3f, -0.5f, 0.1f,   1.0f, 0.0f, 0.0f, 1.0f, //first half of the circle, bottom part of the cylinder
        -0.2f, -0.5f, 0.2f,   0.0f, 1.0f, 0.0f, 1.0f, 
        -0.1f, -0.5f, 0.3f,   0.0f, 0.0f, 1.0f, 1.0f, 
         0.0f, -0.5f, 0.4f,   1.0f, 0.0f, 1.0f, 1.0f, 
         0.1f, -0.5f, 0.3f,   1.0f, 0.0f, 0.0f, 1.0f,
         0.2f, -0.5f, 0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
         0.3f, -0.5f, 0.1f,   0.0f, 0.0f, 1.0f, 1.0f,

        -0.3f, -0.5f, -0.1f,   1.0f, 0.0f, 0.0f, 1.0f, //second half of the circle, bottom part of the cylinder
        -0.2f, -0.5f, -0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
        -0.1f, -0.5f, -0.3f,   0.0f, 0.0f, 1.0f, 1.0f,
         0.0f, -0.5f, -0.4f,   1.0f, 0.0f, 1.0f, 1.0f,
         0.1f, -0.5f, -0.3f,   1.0f, 0.0f, 0.0f, 1.0f,
         0.2f, -0.5f, -0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
         0.3f, -0.5f, -0.1f,   0.0f, 0.0f, 1.0f, 1.0f,

        -0.3f, 0.5f, 0.1f,   1.0f, 0.0f, 0.0f, 1.0f, //first half of the circle, top part of the cylinder
        -0.2f, 0.5f, 0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
        -0.1f, 0.5f, 0.3f,   0.0f, 0.0f, 1.0f, 1.0f,
         0.0f, 0.5f, 0.4f,   1.0f, 0.0f, 1.0f, 1.0f,
         0.1f, 0.5f, 0.3f,   1.0f, 0.0f, 0.0f, 1.0f,
         0.2f, 0.5f, 0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
         0.3f, 0.5f, 0.1f,   0.0f, 0.0f, 1.0f, 1.0f,

        -0.3f, 0.5f, -0.1f,   1.0f, 0.0f, 0.0f, 1.0f, //second half of the circle, top part of the cylinder
        -0.2f, 0.5f, -0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
        -0.1f, 0.5f, -0.3f,   0.0f, 0.0f, 1.0f, 1.0f,
         0.0f, 0.5f, -0.4f,   1.0f, 0.0f, 1.0f, 1.0f,
         0.1f, 0.5f, -0.3f,   1.0f, 0.0f, 0.0f, 1.0f,
         0.2f, 0.5f, -0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
         0.3f, 0.5f, -0.1f,   0.0f, 0.0f, 1.0f, 1.0f,

        
    };

    GLfloat verts2[] = {//Tree top
        // Vertex Positions    // Colors (r,g,b,a)
        -0.3f, 0.5f, 0.1f,   1.0f, 0.0f, 0.0f, 1.0f, //first half of the circle, bottom part of the cone
        -0.2f, 0.5f, 0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
        -0.1f, 0.5f, 0.3f,   0.0f, 0.0f, 1.0f, 1.0f,
         0.0f, 0.5f, 0.4f,   1.0f, 0.0f, 1.0f, 1.0f,
         0.1f, 0.5f, 0.3f,   1.0f, 0.0f, 0.0f, 1.0f,
         0.2f, 0.5f, 0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
         0.3f, 0.5f, 0.1f,   0.0f, 0.0f, 1.0f, 1.0f,

         0.3f, 0.5f, -0.1f,   0.0f, 0.0f, 1.0f, 1.0f, //second half of the circle, bottom part of the cone
         0.2f, 0.5f, -0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
         0.1f, 0.5f, -0.3f,   1.0f, 0.0f, 0.0f, 1.0f,
         0.0f, 0.5f, -0.4f,   1.0f, 0.0f, 1.0f, 1.0f,
        -0.1f, 0.5f, -0.3f,   0.0f, 0.0f, 1.0f, 1.0f,
        -0.2f, 0.5f, -0.2f,   0.0f, 1.0f, 0.0f, 1.0f,
        -0.3f, 0.5f, -0.1f,   1.0f, 0.0f, 0.0f, 1.0f,

         0.0f, 3.0f,  0.0f,   0.0f, 0.0f, 1.0f, 1.0f //center for the cone (14)
    };

    GLfloat verts3[] = {//Flat plane
        //Vertex Positions     //Colors (r,g,b,a)
        -1.0f, 0.5f,  1.0f,    1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, 0.5f, -1.0f,    0.0f, 1.0f, 0.0f, 1.0f,
         1.0f, 0.5f,  1.0f,    0.0f, 0.0f, 1.0f, 1.0f,
         1.0f, 0.5f, -1.0f,    1.0f, 1.0f, 0.0f, 1.0f,

    };

    GLfloat verts4[] = {//Pyramid
        //Vertex Positions     //Colors (r,g,b,a)
        -1.0f, 0.5f,  1.0f,    1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, 0.5f, -1.0f,    0.0f, 1.0f, 0.0f, 1.0f,
         1.0f, 0.5f,  1.0f,    0.0f, 0.0f, 1.0f, 1.0f,
         1.0f, 0.5f, -1.0f,    1.0f, 1.0f, 0.0f, 1.0f,

         0.0f, 3.0f,  0.0f,    0.0f, 0.0f, 1.0f, 1.0f //center for the pyramid 
    };
    

    // Index data to share position data
    GLushort indices1[] = {//creating the rest of the triangles from the base
         0,  1, 15,//first triangle, rectangle 1, side 1
        15, 14,  0,//second triangle, rectangle 1, side 1

         1,  2, 16,//first triangle, rectangle 2, side 1
        15, 16,  1,//second triangle, rectangle 2, side 1

         2,  3, 17,//first triangle, rectangle 3, side 1
        16, 17,  2,//second triangle, rectangle 3, side 1     

         3,  4, 18,//first triangle, rectangle 4, side 1
        17, 18,  3,//second triangle, rectangle 4, side 1
         
         4,  5, 19,//first triangle, rectangle 5, side 1
        18, 19,  4,//second triangle, rectangle 5, side 1

         5,  6, 20,//first triangle, rectangle 6, side 1
        19, 20,  5,//second triangle, rectangle 6, side 1

         0,  7, 21,//first triangle, rectangle 7, side 2
        14, 21,  0,//second triangle, rectangle 7, side 2
        
         7,  8, 22,//first triangle, rectangle 8, side 2
        22, 21,  7,//second triangle, rectangle 8, side 2

         8,  9, 23,//first triangle, rectangle 9, side 2
        22, 23,  8,//second triangle, rectangle 9, side 2

         9, 10, 24,//first triangle, rectangle 10, side 2
        23, 24,  9,//second triangle, rectangle 10, side 2

        10, 11, 25,//first triangle, rectangle 11, side 2
        24, 25, 10,//second triangle, rectangle 11, side 2

        11, 12, 26,//first triangle, rectangle 12, side 2
        25, 26, 11,//second triangle, rectangle 12, side 2

        12, 13, 27,//first triangle, rectangle 13, side 2
        26, 27, 12,//second triangle, rectangle 13, side 2

         6, 13, 20,//first triangle, rectangle 14, side 2
        27, 20,  6,//second triangle, rectangle 14, side 2
    };

    // Index data to share position data
    GLushort indices2[] = {
        0,  1, 14,//Creating the cone shape
        1,  2, 14,
        2,  3, 14,
        3,  4, 14,
        4,  5, 14,
        5,  6, 14,
        6,  7, 14,
        7,  8, 14,
        8,  9, 14,
        9, 10, 14,
       10, 11, 14,
       11, 12, 14,
       12, 13, 14,
       13,  1, 14
    };

    GLushort indices3[] = {
        0,  1, 2,//Creating flat plane
        2,  3, 1
    };

    GLushort indices4[] = {
        0, 1, 4,
        1, 3, 4,
        3, 2, 4,
        2, 0, 4,
        0, 1, 2,
        2, 3, 1
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;

    //------------------------OBJECT 1(PLANE)----------------------------------------------------

    glGenVertexArrays(2, &mesh.vao1); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao1);

    // Create 2 buffers: first one for the vertex data; second one for the indices. THIS THE PLANE OF THE SCENE
    glGenBuffers(2, mesh.vbos1);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos1[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts3), verts3, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
    //glBufferData(GL_ARRAY_BUFFER, sizeof(verts2), verts2, GL_STATIC_DRAW);

    mesh.nIndices1 = sizeof(indices1) / sizeof(indices1[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos1[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices3), indices3, GL_STATIC_DRAW);
    
    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride1 = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride1, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride1, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    //-------------------------OBJECT 2(Tree Trunk)-----------------------------------------------------

    glGenVertexArrays(2, &mesh.vao2); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao2);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos2);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos2[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts1), verts1, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts1), verts1, GL_STATIC_DRAW);

    mesh.nIndices2 = sizeof(indices1) / sizeof(indices1[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos2[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices1), indices1, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride2 = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride2, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride2, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    //------------------------OBJECT 3(Tree Top)----------------------------------------------------

    glGenVertexArrays(2, &mesh.vao3); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao3);

    // Create 2 buffers: first one for the vertex data; second one for the indices. THIS THE PLANE OF THE SCENE
    glGenBuffers(2, mesh.vbos3);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos3[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts2), verts2, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts2), verts2, GL_STATIC_DRAW);

    mesh.nIndices1 = sizeof(indices2) / sizeof(indices2[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos3[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2), indices2, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride3 = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride1, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride1, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao1);
    glDeleteVertexArrays(1, &mesh.vao2);
    glDeleteVertexArrays(1, &mesh.vao3);
    glDeleteBuffers(2, mesh.vbos1);
    glDeleteBuffers(2, mesh.vbos2);
    glDeleteBuffers(2, mesh.vbos3);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

