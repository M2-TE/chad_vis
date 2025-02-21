import engine;

#if __has_include(<GLFW/glfw3.h>)
    int main() {
        Engine engine;
        engine.run();
    }
#endif