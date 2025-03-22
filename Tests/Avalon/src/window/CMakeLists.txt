// IGlfwWrapper.h
#include <GLFW/glfw3.h>

class IGlfwWrapper {
public:
    virtual ~IGlfwWrapper() = default;
    virtual int init() = 0;
    virtual void terminate() = 0;
    virtual void windowHint(int hint, int value) = 0;
    virtual GLFWwindow* createWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share) = 0;
    virtual void destroyWindow(GLFWwindow* window) = 0;
    virtual void setWindowUserPointer(GLFWwindow* window, void* pointer) = 0;
};
