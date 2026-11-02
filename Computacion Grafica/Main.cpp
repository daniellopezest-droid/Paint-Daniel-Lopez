#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// Inclusión de Dear ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// ============================================================================
// CONFIGURACIÓN DE LA GRILLA
// ============================================================================
const int COLUMNAS = 50;
const int FILAS = 35;
const int TAMANO_CELDA = 15;
const int ANCHO_VENTANA = COLUMNAS * TAMANO_CELDA;
const int ALTO_VENTANA = FILAS * TAMANO_CELDA;

// ============================================================================
// ESTADOS Y VARIABLES DE DIBUJO
// ============================================================================
float colorPincel[3] = { 0.9f, 0.2f, 0.2f }; // Color actual de dibujo (R, G, B)
bool modoBorrar = false;

// ============================================================================
// MATRIZ QUE REPRESENTA LA GRILLA
// ============================================================================
std::vector<std::vector<bool>> grilla(
    FILAS,
    std::vector<bool>(COLUMNAS, false)
);

// ============================================================================
// FUNCIÓN AUXILIAR PARA ACTIVAR O DESACTIVAR UNA CELDA
// ============================================================================
void marcarCelda(int x, int y, bool activar)
{
    if (x >= 0 && x < COLUMNAS && y >= 0 && y < FILAS)
    {
        grilla[y][x] = activar;
    }
}

// ============================================================================
// ALGORITMO DE BRESENHAM PARA LÍNEAS
// ============================================================================
void calcularLineaBresenham(int x0, int y0, int x1, int y1)
{
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true)
    {
        marcarCelda(x0, y0, true);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// ============================================================================
// DIBUJA LOS 8 PUNTOS SIMÉTRICOS DEL CÍRCULO
// ============================================================================
void dibujarPuntosCirculo(int xc, int yc, int x, int y)
{
    marcarCelda(xc + x, yc + y, true);
    marcarCelda(xc - x, yc + y, true);
    marcarCelda(xc + x, yc - y, true);
    marcarCelda(xc - x, yc - y, true);
    marcarCelda(xc + y, yc + x, true);
    marcarCelda(xc - y, yc + x, true);
    marcarCelda(xc + y, yc - x, true);
    marcarCelda(xc - y, yc - x, true);
}

// ============================================================================
// ALGORITMO DE BRESENHAM PARA CÍRCULOS
// ============================================================================
void calcularCirculoBresenham(int xc, int yc, int radio)
{
    int x = 0;
    int y = radio;
    int d = 3 - (2 * radio);

    while (x <= y)
    {
        dibujarPuntosCirculo(xc, yc, x, y);

        if (d < 0)
        {
            d = d + (4 * x) + 6;
        }
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

// ============================================================================
// CONVERSIÓN A COORDENADAS NORMALIZADAS (NDC)
// ============================================================================
void transformarACoordenadasNDC(float x, float y, float& ndcX, float& ndcY)
{
    ndcX = (x / ANCHO_VENTANA) * 2.0f - 1.0f;
    ndcY = (y / ALTO_VENTANA) * 2.0f - 1.0f;
}

// ============================================================================
// CÓDIGO FUENTE DE LOS SHADERS (GLSL)
// ============================================================================
static const char* codigoFuenteShaderVertices = R"glsl(
#version 330 core

layout(location = 0) in vec2 posAtributo;

uniform mat4 uTransformacion;

void main()
{
    gl_Position = uTransformacion * vec4(posAtributo, 0.0, 1.0);
}
)glsl";

static const char* codigoFuenteShaderFragmentos = R"glsl(
#version 330 core
out vec4 colorFragmento;
uniform vec3 uColor;
void main(){
    colorFragmento = vec4(uColor, 1.0);
}
)glsl";

// ============================================================================
// UTILIDADES DE SHADERS
// ============================================================================
GLuint compilarShader(GLenum tipoShader, const char* fuente)
{
    GLuint shader = glCreateShader(tipoShader);
    glShaderSource(shader, 1, &fuente, NULL);
    glCompileShader(shader);

    int exito;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &exito);
    if (!exito)
    {
        char registroInfo[512];
        glGetShaderInfoLog(shader, 512, NULL, registroInfo);
        std::cout << "Error de compilación en el Shader: " << registroInfo << "\n";
    }
    return shader;
}

GLuint crearPrograma()
{
    GLuint shaderVert = compilarShader(GL_VERTEX_SHADER, codigoFuenteShaderVertices);
    GLuint shaderFrag = compilarShader(GL_FRAGMENT_SHADER, codigoFuenteShaderFragmentos);

    GLuint programa = glCreateProgram();
    glAttachShader(programa, shaderVert);
    glAttachShader(programa, shaderFrag);
    glLinkProgram(programa);

    int exito;
    glGetProgramiv(programa, GL_LINK_STATUS, &exito);
    if (!exito)
    {
        char registroInfo[512];
        glGetProgramInfoLog(programa, 512, NULL, registroInfo);
        std::cout << "Error al enlazar el programa de Shaders: " << registroInfo << "\n";
    }

    glDeleteShader(shaderVert);
    glDeleteShader(shaderFrag);
    return programa;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================
int main()
{
    if (!glfwInit())
    {
        std::cout << "Error al inicializar GLFW\n";
        return -1;
    }

    GLFWwindow* ventana = glfwCreateWindow(
        ANCHO_VENTANA,
        ALTO_VENTANA,
        "Paint Grilla con Bresenham e ImGui",
        NULL,
        NULL
    );

    if (!ventana)
    {
        std::cout << "Error al crear ventana\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(ventana);
    glfwSwapInterval(1); // Habilitar VSync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error al cargar GLAD\n";
        return -1;
    }

    // --- INICIALIZAR DEAR IMGUI ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(ventana, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Opcional: Figuras iniciales
    calcularCirculoBresenham(25, 25, 6);
    calcularLineaBresenham(5, 2, 44, 14);

    while (!glfwWindowShouldClose(ventana))
    {
        glfwPollEvents();

        // --- GESTIÓN DEL MOUSE PARA PINTAR ---
        if (glfwGetMouseButton(ventana, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            // Evitar pintar si el mouse está sobre una ventana de ImGui
            if (!io.WantCaptureMouse)
            {
                double mouseX, mouseY;
                glfwGetCursorPos(ventana, &mouseX, &mouseY);

                // Convertir coordenadas de ventana a coordenadas de grilla
                int gx = static_cast<int>(mouseX / TAMANO_CELDA);
                int gy = static_cast<int>((ALTO_VENTANA - mouseY) / TAMANO_CELDA); // Invertir eje Y

                marcarCelda(gx, gy, !modoBorrar);
            }
        }

        // --- INICIAR FRAME DE IMGUI ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- CREAR VENTANA DE CONTROL IMGUI ---
        ImGui::Begin("Panel de Control - Paint");
        ImGui::Text("Herramientas de Dibujo");

        ImGui::Checkbox("Modo Borrador", &modoBorrar);
        ImGui::ColorEdit3("Color del Pincel", colorPincel);

        if (ImGui::Button("Limpiar Grilla"))
        {
            for (int y = 0; y < FILAS; y++)
                for (int x = 0; x < COLUMNAS; x++)
                    grilla[y][x] = false;
        }

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::End();

        // Renderizar ImGui
        ImGui::Render();

        // --- RENDERIZADO OPENGL ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        std::vector<float> verticesRelleno;
        std::vector<float> verticesLineas;

        for (int y = 0; y < FILAS; y++)
        {
            for (int x = 0; x < COLUMNAS; x++)
            {
                float xMinimo = x * TAMANO_CELDA;
                float yMinimo = y * TAMANO_CELDA;
                float xMaximo = xMinimo + TAMANO_CELDA;
                float yMaximo = yMinimo + TAMANO_CELDA;

                float ndcX1, ndcY1, ndcX2, ndcY2;
                transformarACoordenadasNDC(xMinimo, yMinimo, ndcX1, ndcY1);
                transformarACoordenadasNDC(xMaximo, yMaximo, ndcX2, ndcY2);

                if (grilla[y][x])
                {
                    // Primer triángulo
                    verticesRelleno.push_back(ndcX1); verticesRelleno.push_back(ndcY1);
                    verticesRelleno.push_back(ndcX2); verticesRelleno.push_back(ndcY1);
                    verticesRelleno.push_back(ndcX2); verticesRelleno.push_back(ndcY2);

                    // Segundo triángulo
                    verticesRelleno.push_back(ndcX2); verticesRelleno.push_back(ndcY2);
                    verticesRelleno.push_back(ndcX1); verticesRelleno.push_back(ndcY2);
                    verticesRelleno.push_back(ndcX1); verticesRelleno.push_back(ndcY1);
                }

                // Bordes de celdas (Grilla)
                verticesLineas.push_back(ndcX1); verticesLineas.push_back(ndcY1);
                verticesLineas.push_back(ndcX2); verticesLineas.push_back(ndcY1);

                verticesLineas.push_back(ndcX2); verticesLineas.push_back(ndcY1);
                verticesLineas.push_back(ndcX2); verticesLineas.push_back(ndcY2);

                verticesLineas.push_back(ndcX2); verticesLineas.push_back(ndcY2);
                verticesLineas.push_back(ndcX1); verticesLineas.push_back(ndcY2);

                verticesLineas.push_back(ndcX1); verticesLineas.push_back(ndcY2);
                verticesLineas.push_back(ndcX1); verticesLineas.push_back(ndcY1);
            }
        }

        static GLuint programaShaders = 0;
        static GLuint arregloVerticesVAO = 0, buferVerticesVBO = 0;

        // Matriz identidad
        float matrizTransformacion[16] =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        if (programaShaders == 0)
        {
            programaShaders = crearPrograma();
            glGenVertexArrays(1, &arregloVerticesVAO);
            glGenBuffers(1, &buferVerticesVBO);
        }

        glBindVertexArray(arregloVerticesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, buferVerticesVBO);

        // 1. Dibuja celdas activas (usando el color del selector ImGui)
        if (!verticesRelleno.empty())
        {
            glBufferData(GL_ARRAY_BUFFER, verticesRelleno.size() * sizeof(float), verticesRelleno.data(), GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

            glUseProgram(programaShaders);

            int ubicacionTransformacion =
                glGetUniformLocation(programaShaders, "uTransformacion");

            glUniformMatrix4fv(
                ubicacionTransformacion,
                1,
                GL_FALSE,
                matrizTransformacion
            );

            int ubicacionColor =
                glGetUniformLocation(programaShaders, "uColor");

            glUniform3f(
                ubicacionColor,
                colorPincel[0],
                colorPincel[1],
                colorPincel[2]
            );

            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verticesRelleno.size() / 2));
        }

        // 2. Dibuja las líneas de la grilla
        if (!verticesLineas.empty())
        {
            glBufferData(GL_ARRAY_BUFFER, verticesLineas.size() * sizeof(float), verticesLineas.data(), GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

            glUseProgram(programaShaders);

            int ubicacionTransformacion2 =
                glGetUniformLocation(programaShaders, "uTransformacion");

            glUniformMatrix4fv(
                ubicacionTransformacion2,
                1,
                GL_FALSE,
                matrizTransformacion
            );

            int ubicacionColor2 =
                glGetUniformLocation(programaShaders, "uColor");

            glUniform3f(
                ubicacionColor2,
                0.25f,
                0.25f,
                0.25f
            );

            glDrawArrays(GL_LINES, 0, (GLsizei)(verticesLineas.size() / 2));
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Renderizar los elementos gráficos de ImGui encima de OpenGL
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ventana);
    }

    // --- LIMPIEZA DE IMGUI ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}