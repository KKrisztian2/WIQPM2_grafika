#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>

struct objCube{
    GLfloat vertices[8][3];
    GLfloat texcoords[4][2];
    GLfloat normals[6][3];
    int faces[12][3][3];
    int v_count, vt_count, vn_count, f_count;
};

struct Camera{
    float cameraX;
    float cameraY;
    float cameraZ;
    float cameraAngleX;
    float cameraAngleZ;
    float rotationAngle;
};

struct Lights{
    GLfloat lightAmbient[4];
    GLfloat lightDiffuse[4];
    GLfloat lightPosition[4];
};

GLuint loadTexture(const char *filename) {
    SDL_Surface *image = IMG_Load(filename);
    if (!image) {
        printf("Can't load texture, %s\n", IMG_GetError());
        return 0;
    }
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->w, image->h, 0, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SDL_FreeSurface(image);

    return textureID;
}

void readFromOBJ(const char *filename, struct objCube* Cube) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Can't open .obj file\n");
        return;
    }

    char line[256];
    Cube->v_count = 0;
    Cube->vt_count = 0;
    Cube->vn_count = 0;
    Cube->f_count = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v') {
            if (line[1] == ' ') {
                sscanf(line, "v %f %f %f", &Cube->vertices[Cube->v_count][0], &Cube->vertices[Cube->v_count][1], &Cube->vertices[Cube->v_count][2]);
                Cube->v_count++;
            } else if (line[1] == 't') {
                sscanf(line, "vt %f %f", &Cube->texcoords[Cube->vt_count][0],  &Cube->texcoords[Cube->vt_count][1]);
                Cube->vt_count++;
            } else if (line[1] == 'n') {
                sscanf(line, "vn %f %f %f", &Cube->normals[Cube->vn_count][0], &Cube->normals[Cube->vn_count][1], &Cube->normals[Cube->vn_count][2]);
                Cube->vn_count++;
            }
        } else if (line[0] == 'f') {
            sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                   &Cube->faces[Cube->f_count][0][0], &Cube->faces[Cube->f_count][0][1], &Cube->faces[Cube->f_count][0][2],
                   &Cube->faces[Cube->f_count][1][0], &Cube->faces[Cube->f_count][1][1], &Cube->faces[Cube->f_count][1][2],
                   &Cube->faces[Cube->f_count][2][0], &Cube->faces[Cube->f_count][2][1], &Cube->faces[Cube->f_count][2][2]);
            Cube->f_count++;
        }
    }
    fclose(file);
}

void drawCube(struct objCube Cube){
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < Cube.f_count; i++) {
        for (int j = 0; j < 3; j++) {
            glNormal3fv(Cube.normals[Cube.faces[i][j][2] - 1]);
            glTexCoord2fv(Cube.texcoords[Cube.faces[i][j][1] - 1]);
            glVertex3fv(Cube.vertices[Cube.faces[i][j][0] - 1]);
        }
    }
    glEnd();
}


void update(struct Camera* camera) {
    static Uint32 lastTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    camera->rotationAngle += 10.0f * deltaTime;
    if (camera->rotationAngle >= 360.0f){
        camera->rotationAngle -= 360.0f;
    }
}

void display(struct objCube Cube, struct Lights light, struct Camera camera, SDL_Window* window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glRotatef(-(camera.cameraAngleZ), 1.0f, 0.0f, 0.0f);
    glRotatef(-(camera.cameraAngleX), 0.0f, 1.0f, 0.0f);
    glTranslatef(-(camera.cameraX), -(camera.cameraY), -(camera.cameraZ));

    glPushMatrix();
    glRotatef(camera.rotationAngle, 0.5f, 1.0f, 0.1f);
    drawCube(Cube);
    glPopMatrix();

    GLfloat lightRelativeToCamera[] = { light.lightPosition[0] - camera.cameraX, light.lightPosition[1] - camera.cameraY, light.lightPosition[2] - camera.cameraZ, light.lightPosition[3] };
    glLightfv(GL_LIGHT0, GL_POSITION, lightRelativeToCamera);

    SDL_GL_SwapWindow(window);
}

void init(struct Lights* light) {
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1, 1, -1, 1, 1, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    for(int i = 0; i < 4; i++){
        light->lightAmbient[i] = 0.2f;
        light->lightDiffuse[i] = 0.8f;
        light->lightPosition[i] = 0.0f;
    }
    light->lightPosition[1] = 5.0f;
    light->lightPosition[2] = 20.0f;
    light->lightPosition[3] = 2.0f;
    glLightfv(GL_LIGHT0, GL_AMBIENT, light->lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light->lightDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light->lightPosition);
}

void handleMovement(SDL_KeyboardEvent *event, struct Camera* camera) {
    const float cameraSpeed = 0.05f;

    switch (event->keysym.sym) {
        case SDLK_w:
            camera->cameraX += 0.1f * sin(-(camera->cameraAngleX) * M_PI / 180.0f);
            camera->cameraZ -= 0.1f * cos(-(camera->cameraAngleX) * M_PI / 180.0f);
            break;
        case SDLK_s:
            camera->cameraX -= 0.1f * sin(-(camera->cameraAngleX) * M_PI / 180.0f);
            camera->cameraZ += 0.1f * cos(-(camera->cameraAngleX) * M_PI / 180.0f);
            break;
        case SDLK_a:
            camera->cameraX -= 0.1f * cos((camera->cameraAngleX) * M_PI / 180.0f);
            camera->cameraZ += 0.1f * sin((camera->cameraAngleX) * M_PI / 180.0f);
            break;
        case SDLK_d:
            camera->cameraX += 0.1f * cos((camera->cameraAngleX) * M_PI / 180.0f);
            camera->cameraZ -= 0.1f * sin((camera->cameraAngleX) * M_PI / 180.0f);
            break;
        case SDLK_UP:
            camera->cameraAngleZ += 1.0f;
            break;
        case SDLK_DOWN:
            camera->cameraAngleZ -= 1.0f;
            break;
        case SDLK_LEFT:
            camera->cameraAngleX += 1.0f;
            break;
        case SDLK_RIGHT:
            camera->cameraAngleX -= 1.0f;
            break;
        case SDLK_q:
            camera->cameraY -= cameraSpeed;
            break;
        case SDLK_e:
            camera->cameraY += cameraSpeed;
            break;
        default:
            break;
    }
}

void handleLights(SDL_KeyboardEvent *event, struct Lights* light){
    const float lightIntensityChange = 0.1f;
    switch (event->keysym.sym) {
        case SDLK_MINUS:
            for(int i = 0; i < 3; i++){
                light->lightDiffuse[i] -= lightIntensityChange;
                if (light->lightDiffuse[i] < 0) light->lightDiffuse[i] = 0;
            }
            glLightfv(GL_LIGHT0, GL_DIFFUSE, light->lightDiffuse);
            break;
        case SDLK_PLUS:
            for(int i = 0; i < 3; i++){
                light->lightDiffuse[i] += lightIntensityChange;
                if (light->lightDiffuse[i] > 1) light->lightDiffuse[i] = 1;
            }
            glLightfv(GL_LIGHT0, GL_DIFFUSE, light->lightDiffuse);
            break;
        default:
            break;
    }
}

void helpMenu(SDL_KeyboardEvent *event, SDL_Window* window){
    switch (event->keysym.sym) {
        case SDLK_F1:
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Használati útmutató",
                                         "W, A, S, D: Kamera mozgatása előre, balra, hátra, jobbra\n"
                                         "Nyilak: Kamera nézet forgatása\n"
                                         "Q, E: Kamera magasságának növelése és csökkentése\n"
                                         "+, -: Fény diffúz intenzitásának növelése és csökkentése",
                                         window);
        default:
            break;
    }
}

int main(int argc, char** argv) {

    SDL_Window* window;
    struct objCube Cube;
    struct Lights light;
    struct Camera camera;
    camera.cameraX = 0.0f;
    camera.cameraY = 0.0f;
    camera.cameraZ = 3.0f;
    camera.cameraAngleX = 0.0f;
    camera.cameraAngleZ = 0.0f;
    camera.rotationAngle = 0.0f;


    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("3D Kocka", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Window create error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        printf("OpenGL context error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (IMG_Init(IMG_INIT_JPG) == 0) {
        printf("SDL_image error: %s\n", IMG_GetError());
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    init(&light);

    GLuint textureID = loadTexture("assets/texture.jpg");
    if (textureID == 0) {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, textureID);
    readFromOBJ("assets/cube.obj", &Cube);

    SDL_Event event;
    int running = 1;
    while (running) {

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_KEYDOWN:
                    handleMovement(&event.key, &camera);
                    handleLights(&event.key, &light);
                    helpMenu(&event.key, window);
                    break;
                default:
                    break;
            }
        }
        display(Cube, light, camera, window);
        update(&camera);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
