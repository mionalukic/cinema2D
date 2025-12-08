#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include "Util.h"
#include <vector>
#include <ctime>

GLFWcursor* cursor;

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

enum SeatState { FREE, RESERVED, BOUGHT };

struct Seat {
    float x, y;
    SeatState state = FREE;
};

std::vector<Seat> seats;
unsigned int personTexture;
unsigned int seatTexFree, seatTexReserved, seatTexBought, bottomImageTexture, nameTexture;

std::vector<int> seatsPerRow = { 16,20,20,20,20,20,20,20 };

struct Person {
    float x, y;
    float targetX, targetY;
    float midX, midY;
    float speed = 0.003f;
    float personalSpeed;     
    float startDelay;       
    float horizontalOffset; 
    bool arrived = false;
    int stage = 0;
};


std::vector<Person> persons;
bool peopleGenerated = false;

enum AppState { IDLE, PEOPLE_ENTERING, FILM_PLAYING, PEOPLE_EXITING, RESETTING };
AppState appState = IDLE;

double filmStartTime = 0.0;
int filmFrameCounter = 0;
float currentFilmR = 1.0f, currentFilmG = 1.0f, currentFilmB = 1.0f;


void generateSeats(const std::vector<int>& seatsPerRow)
{
    seats.clear();

    float dx = 0.06f;
    float dy = 0.15f;

    int R = seatsPerRow.size();

    float startY = 0.3f;

    for (int row = 0; row < R; row++)
    {
        int n = seatsPerRow[row];
        float rowWidth = (n - 1) * dx;

        float offsetX = -rowWidth / 2.0f;

        float y = startY - row * dy;

        for (int i = 0; i < n; i++)
        {
            seats.push_back({
                offsetX + i * dx,
                y
                });
        }
    }
}



void generatePeople()
{
    persons.clear();

    std::vector<Seat*> taken;
    for (auto& s : seats)
        if (s.state == RESERVED || s.state == BOUGHT)
            taken.push_back(&s);

    if (taken.empty()) return;

    int maxPeople = taken.size();
    int count = rand() % (maxPeople + 1);
    if (count == 0) return;

    for (int i = 0; i < count; i++)
    {
        Seat* seat = taken[i];

        Person p;

        // ulazna pozicija
        p.x = -1.15f;
        p.y = 0.70f;

        // random delay
        p.startDelay = (rand() % 1200) / 1000.0f;   // 0–1.2 sec
        // različite brzine
        p.personalSpeed = 0.0025f + (rand() % 1500) / 1000000.0f;
        // malo odstupanje levo-desno
        p.horizontalOffset = ((rand() % 100) / 100.0f - 0.5f) * 0.08f;

        // STAGE 0 – prolaz
        p.midX = -0.95f + p.horizontalOffset;
        p.midY = 0.70f;

        // STAGE 1 – vertikalno do reda
        p.targetY = seat->y;

        // STAGE 2 – horizontalno do sedišta
        p.targetX = seat->x;

        p.stage = 0;
        p.arrived = false;

        persons.push_back(p);
    }

    peopleGenerated = true;
}

void updatePeopleEntering()
{
    bool allArrived = true;
    double currentTime = glfwGetTime();

    for (auto& p : persons)
    {
        if (p.arrived) continue;

        // čekaj dok startDelay ne istekne
        if (currentTime < filmStartTime + p.startDelay)
        {
            allArrived = false;
            continue;
        }

        allArrived = false;

        float spd = p.personalSpeed;

        if (p.stage == 0)
        {
            if (fabs(p.x - p.midX) > 0.01f)
                p.x += (p.midX > p.x ? spd : -spd);
            else
                p.stage = 1;
        }
        else if (p.stage == 1)
        {
            if (fabs(p.y - p.targetY) > 0.01f)
                p.y += (p.targetY > p.y ? spd : -spd);
            else
                p.stage = 2;
        }
        else if (p.stage == 2)
        {
            if (fabs(p.x - p.targetX) > 0.01f)
                p.x += (p.targetX > p.x ? spd : -spd);
            else
                p.arrived = true;
        }
    }

    if (!persons.empty() && allArrived)
    {
        appState = FILM_PLAYING;
        filmStartTime = glfwGetTime();
        filmFrameCounter = 0;
    }
}

void startPeopleExit()
{
    appState = PEOPLE_EXITING;
    for (auto& p : persons)
    {
        p.stage = 0;
        p.arrived = false;
        p.speed = 0.004f;

        // 1. segment — horizontalno ka vertikalnom prolazu
        p.midX = -0.95f;
        p.midY = p.y;

        // 2. segment — vertikalno do nivoa vrata
        p.targetX = -0.95f;
        p.targetY = 0.70f;

        // konačno — kroz vrata
    }
}

bool updatePeopleExitingAndCheckAllOut()
{
    bool allOut = true;

    for (auto& p : persons)
    {
        if (p.stage == 0)
        {
            if (fabs(p.x - p.midX) > 0.01f)
                p.x += (p.midX > p.x ? p.speed : -p.speed);
            else
                p.stage = 1;
        }
        else if (p.stage == 1)
        {
            if (fabs(p.y - p.targetY) > 0.01f)
                p.y += (p.targetY > p.y ? p.speed : -p.speed);
            else
                p.stage = 2;
        }
        else if (p.stage == 2)
        {
            // kroz vrata napolje
            if (p.x > -1.15f)
                p.x -= p.speed;
        }

        if (p.x > -1.15f)
            allOut = false;
    }

    return allOut;
}



void mouseClick(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float ndcX = 2.0f * (x / width) - 1.0f;
        float ndcY = 1.0f - 2.0f * (y / height);

        for (auto& s : seats) {
            if (fabs(ndcX - s.x) < 0.04f && fabs(ndcY - s.y) < 0.04f)
                if (s.state == FREE)
                    s.state = RESERVED;
                else if (s.state == RESERVED)
                    s.state = FREE;

        }
    }
}



void buySeat(int n, const std::vector<int>& seatsPerRow)
{
    int index = 0;

    std::vector<std::pair<int, int>> rowRanges;
    for (int count : seatsPerRow)
    {
        int start = index;
        int end = index + count - 1;
        rowRanges.push_back({ start, end });
        index += count;
    }

    for (int r = rowRanges.size() - 1; r >= 0; r--)
    {
        int start = rowRanges[r].first;
        int end = rowRanges[r].second;

        int count = 0;
        int pos = -1;

        for (int i = end; i >= start; i--)
        {
            if (seats[i].state == FREE)
            {
                count++;
            }
            else
            {
                count = 0;
            }

            if (count == n)
            {
                pos = i; 
                break;
            }
        }

        if (pos != -1)
        {
            for (int i = pos; i < pos + n; i++)
                seats[i].state = BOUGHT;

            return;
        }
    }

    std::cout << "Nema dovoljno susednih slobodnih sedista!" << std::endl;
}

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); 
    glBindTexture(GL_TEXTURE_2D, texture); 

    glGenerateMipmap(GL_TEXTURE_2D);
   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

int main()
{

    srand((unsigned)time(nullptr));

    // Inicijalizacija GLFW i postavljanje na verziju 3 sa programabilnim pajplajnom
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Formiranje prozora za prikaz sa datim dimenzijama i naslovom
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(
        mode->width,
        mode->height,
        "Sala 1",
        monitor,     //full screan
        NULL
    );

    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    // Inicijalizacija GLEW
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //providnost sivog pravugaonika

    unsigned int basicShader = createShader("basic.vert", "basic.frag");

 
    float vertices[] = {
         0.0
    };
    float overlayVertices[] = {
        -1.0f,  1.0f,   0.05f, 0.05f, 0.05f, 0.5f,
        -1.0f, -1.0f,   0.05f, 0.05f, 0.05f, 0.5f,
         1.0f, -1.0f,   0.05f, 0.05f, 0.05f, 0.5f,

        -1.0f,  1.0f,   0.05f, 0.05f, 0.05f, 0.5f,
         1.0f, -1.0f,   0.05f, 0.05f, 0.05f, 0.5f,
         1.0f,  1.0f,   0.05f, 0.05f, 0.05f, 0.5f,
    };

    float screenVertices[] = {
         -0.8f,  0.95f,   1,1,1,1,
          0.8f,  0.95f,   1,1,1,1,
          0.8f,  0.85f,   1,1,1,1,

         -0.8f,  0.95f,   1,1,1,1,
          0.8f,  0.85f,   1,1,1,1,
         -0.8f,  0.85f,   1,1,1,1
    };




    // Vrata u gornjem levom uglu – uska, 1 cm visine, malo spuštena
    float doorVertices[] = {
        -1.0f,  0.80f,    0.55f, 0.27f, 0.07f, 1.0f,   // gornji levi (šarka)
        -1.0f,  0.60f,   0.55f, 0.27f, 0.07f, 1.0f,   // donji levi
        -0.98f, 0.60f,   0.55f, 0.27f, 0.07f, 1.0f,   // donji desni

        -1.0f,  0.8f,    0.55f, 0.27f, 0.07f, 1.0f,   // gornji levi
        -0.98f, 0.80f,    0.55f, 0.27f, 0.07f, 1.0f,   // gornji desni
        -0.98f, 0.60f,   0.55f, 0.27f, 0.07f, 1.0f    // donji desni
    };



    float seatVertices[] = {
        // x,     y,    u,   v
        -0.04f,  0.07f, 0.0f, 1.0f,
        -0.04f, -0.07f, 0.0f, 0.0f,
         0.04f, -0.07f, 1.0f, 0.0f,

        -0.04f,  0.07f, 0.0f, 1.0f,
         0.04f, -0.07f, 1.0f, 0.0f,
         0.04f,  0.07f, 1.0f, 1.0f
    };

    float personVertices[] = {
        -0.04f,  0.04f,   0.0f, 1.0f,
        -0.04f, -0.04f,   0.0f, 0.0f,
         0.04f, -0.04f,   1.0f, 0.0f,
        -0.04f,  0.04f,   0.0f, 1.0f,
         0.04f, -0.04f,   1.0f, 0.0f,
         0.04f,  0.04f,   1.0f, 1.0f
    };

    float bottomImageVertices[] = {
        // x,      y,       u,    v
        -0.3f, -1.0f,    0.0f, 0.0f,
         0.3f, -1.0f,    1.0f, 0.0f,
         0.3f, -0.75f,    1.0f, 1.0f,

        -0.3f, -1.0f,    0.0f, 0.0f,
         0.3f, -0.75f,    1.0f, 1.0f,
        -0.3f, -0.75f,    0.0f, 1.0f
    };

    float nameVertices[] = {
        //   x       y       u   v
         0.7f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f, -0.75f,  1.0f, 1.0f,

         0.7f, -1.0f,   0.0f, 0.0f,
         1.0f, -0.75f,  1.0f, 1.0f,
         0.7f, -0.75f,  0.0f, 1.0f
    };



    // Inicijalizacija VAO i VBO, tipičnih struktura za čuvanje podataka o verteksima
    unsigned int VAO;
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);



    //sivi pravugaonik

    unsigned int overlayVAO, overlayVBO;
    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);

    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //platno
 
    unsigned int screenVAO, screenVBO;
    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);

    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    //vrata
 
    unsigned int doorVAO, doorVBO;
    glGenVertexArrays(1, &doorVAO);
    glGenBuffers(1, &doorVBO);

    glBindVertexArray(doorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(doorVertices), doorVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);



    //sediste
    unsigned int seatVAO, seatVBO;
    glGenVertexArrays(1, &seatVAO);
    glGenBuffers(1, &seatVBO);

    glBindVertexArray(seatVAO);
    glBindBuffer(GL_ARRAY_BUFFER, seatVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(seatVertices), seatVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
 
    glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f); 

    glBindVertexArray(0);


    // covek
    unsigned int personVAO, personVBO;
    glGenVertexArrays(1, &personVAO);
    glGenBuffers(1, &personVBO);

    glBindVertexArray(personVAO);
    glBindBuffer(GL_ARRAY_BUFFER, personVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(personVertices), personVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(0);

    // slika dole na sredini
    unsigned int bottomVAO, bottomVBO;
    glGenVertexArrays(1, &bottomVAO);
    glGenBuffers(1, &bottomVBO);

    glBindVertexArray(bottomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bottomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bottomImageVertices), bottomImageVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(0);

    //ime
    unsigned int nameVAO, nameVBO;
    glGenVertexArrays(1, &nameVAO);
    glGenBuffers(1, &nameVBO);

    glBindVertexArray(nameVAO);
    glBindBuffer(GL_ARRAY_BUFFER, nameVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(nameVertices), nameVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttrib4f(1, 1, 1, 1, 1);
    glBindVertexArray(0);



    glClearColor(0.05f, 0.1f, 0.1f, 1.0f);// Postavljanje boje pozadine

    glfwSetMouseButtonCallback(window, mouseClick);

    float overlayY = 0.0f;          // trenutna y pozicija
    float targetY = 0.0f;         
    bool overlayVisible = true;   
    bool keyPressed = false;        // da sprečimo dupli klik
    bool numberKeyPressed[10] = { false };

    float doorAngle = 0.0f;        // trenutni ugao
    float targetDoorAngle = 0.0f;

    float pivotX = -1.0f;
    float pivotY = 0.8f;


    preprocessTexture(personTexture, "res/person.png");
    preprocessTexture(seatTexFree, "res/blue_seat.png");      
    preprocessTexture(seatTexReserved, "res/yellow_seat.png"); 
    preprocessTexture(seatTexBought, "res/red_seat.png");   
    preprocessTexture(bottomImageTexture, "res/meaning.png");
    preprocessTexture(nameTexture, "res/student.png");


    
    cursor = loadImageToCursor("res/camera.png");
    if (cursor) {
        glfwSetCursor(window, cursor);
    }
    else {
        std::cout << "Koristim podrazumevani kursor (slika nije ucitana)." << std::endl;
    }


    generateSeats(seatsPerRow);


    while (!glfwWindowShouldClose(window))
    {
        // --- input + tipična logika ---
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        {
            if (!keyPressed)
            {
                keyPressed = true;

                if (overlayVisible) {
                    overlayVisible = false;
                    targetDoorAngle = 3.14159f / 2.0f;  // otvori 90°
                    appState = PEOPLE_ENTERING;
                    generatePeople();
                }
                else {
                    overlayVisible = true;
                    targetDoorAngle = 0.0f;
                }

                if (!overlayVisible && !peopleGenerated)
                    generatePeople();

            }
        }
        else
        {
            keyPressed = false;
        }

        // State-driven updates
        if (appState == PEOPLE_ENTERING)
        {
            updatePeopleEntering();
        }
        else if (appState == FILM_PLAYING)
        {
            // zatvori vrata 
            targetDoorAngle = 0.0f;

            // film: menja boju svakih 20 frejmova; traje 20 sekundi
            filmFrameCounter++;
            if (filmFrameCounter % 20 == 0)
            {
                // nova nasumična boja
                currentFilmR = static_cast<float>(rand()) / RAND_MAX;
                currentFilmG = static_cast<float>(rand()) / RAND_MAX;
                currentFilmB = static_cast<float>(rand()) / RAND_MAX;

                // update screenVBO boja (6 verteksa, svaki ima 4 color vrednosti na offsetu)
                float updatedScreenVertices[36];
                // kopiramo pozicije iz originalnog screenVertices, menjamo boje
                // originalno screenVertices ima 6 vrhova * 6 vrednosti (x,y,r,g,b,a)
                // Rekreiramo sve vrednosti ovde: (x,y, r,g,b,a)
                float alpha = 1.0f;
                float coords[6][2] = {
                    {-0.8f,  0.95f},
                    { 0.8f,  0.95f},
                    { 0.8f,  0.85f},
                    {-0.8f,  0.95f},
                    { 0.8f,  0.85f},
                    {-0.8f,  0.85f}
                };
                for (int i = 0; i < 6; ++i)
                {
                    updatedScreenVertices[i * 6 + 0] = coords[i][0];
                    updatedScreenVertices[i * 6 + 1] = coords[i][1];
                    updatedScreenVertices[i * 6 + 2] = currentFilmR;
                    updatedScreenVertices[i * 6 + 3] = currentFilmG;
                    updatedScreenVertices[i * 6 + 4] = currentFilmB;
                    updatedScreenVertices[i * 6 + 5] = alpha;
                }

                glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(updatedScreenVertices), updatedScreenVertices);
            }
            double now = glfwGetTime();
            if (now - filmStartTime >= 20.0)
            {
                //std::cout << now-filmStartTime << std::endl;

                // kraj filma
                float whiteScreen[36];
                float coords[6][2] = {
                    {-0.8f,  0.95f},
                    { 0.8f,  0.95f},
                    { 0.8f,  0.85f},
                    {-0.8f,  0.95f},
                    { 0.8f,  0.85f},
                    {-0.8f,  0.85f}
                };
                for (int i = 0; i < 6; ++i)
                {
                    whiteScreen[i * 6 + 0] = coords[i][0];
                    whiteScreen[i * 6 + 1] = coords[i][1];
                    whiteScreen[i * 6 + 2] = 1.0f;
                    whiteScreen[i * 6 + 3] = 1.0f;
                    whiteScreen[i * 6 + 4] = 1.0f;
                    whiteScreen[i * 6 + 5] = 1.0f;
                }
                glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(whiteScreen), whiteScreen);

                targetDoorAngle = 3.14159f / 2.0f; // otvori vrata
                startPeopleExit();
            }
        }
        else if (appState == PEOPLE_EXITING)
        {
            bool allOut = updatePeopleExitingAndCheckAllOut();
            if (allOut)
            {
                // svi su izasli 
                targetDoorAngle = 0.0f;
                appState = RESETTING;
            }
        }
        else if (appState == RESETTING)
        {
            // vrati overlay/pravougaonik u početno stanje (vidljiv)
            overlayVisible = true;
            targetY = 0.0f;
            peopleGenerated = false;
            persons.clear();
            appState = IDLE;
            for (int i = seats.size() - 1; i >= 0; i--)
                seats[i].state = FREE;
            // po potrebi ostavi sedista u stanju koje su bila (rezervacije ostaju)
        }

        // Animacije
        doorAngle += (targetDoorAngle - doorAngle) * 0.1f;

        // crtanje scena (bez izmena) - samo zovemo tvoj postojeći draw kod
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(basicShader);

        glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 0.0f);
        glBindVertexArray(screenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUniform1i(glGetUniformLocation(basicShader, "isDoor"), 1);
        glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 0.0f);
        glUniform1f(glGetUniformLocation(basicShader, "doorAngle"), doorAngle);
        glUniform2f(glGetUniformLocation(basicShader, "doorPivot"), pivotX, pivotY);

        glBindVertexArray(doorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUniform1i(glGetUniformLocation(basicShader, "isDoor"), 0);
        glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 1.0f);
        glBindVertexArray(overlayVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // crtanje slike dole u centru
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bottomImageTexture);
        glUniform1i(glGetUniformLocation(basicShader, "texture1"), 0);

        glUniform1i(glGetUniformLocation(basicShader, "isSeat"), 1);   // koristi teksturu
        glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 0.0f);
        glUniform2f(glGetUniformLocation(basicShader, "offsetSeat"), 0.0f, 0.0f); // već je pozicija u verteksima

        glBindVertexArray(bottomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // crtanje imena
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, nameTexture);
        glUniform1i(glGetUniformLocation(basicShader, "texture1"), 0);

        glUniform1i(glGetUniformLocation(basicShader, "isSeat"), 1);
        glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 0.0f);
        glUniform2f(glGetUniformLocation(basicShader, "offsetSeat"), 0.0f, 0.0f);

        glBindVertexArray(nameVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // sedista
        for (auto& s : seats) {
            glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 0.0f);
            glUniform2f(glGetUniformLocation(basicShader, "offsetSeat"), s.x, s.y);
            glUniform1i(glGetUniformLocation(basicShader, "isSeat"), 1); // koristi teksturu

           
            glActiveTexture(GL_TEXTURE0);
            if (s.state == FREE) {
                glBindTexture(GL_TEXTURE_2D, seatTexFree);
            }
            else if (s.state == RESERVED) {
                glBindTexture(GL_TEXTURE_2D, seatTexReserved);
            }
            else { 
                glBindTexture(GL_TEXTURE_2D, seatTexBought);
            }
            glUniform1i(glGetUniformLocation(basicShader, "texture1"), 0);

            glBindVertexArray(seatVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        // crtanje ljudi
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, personTexture);
        glUniform1i(glGetUniformLocation(basicShader, "texture1"), 0);
        glUniform1i(glGetUniformLocation(basicShader, "isSeat"), 1); // koristimo teksturu

        glBindVertexArray(personVAO);
        for (auto& p : persons)
        {
            glUniform2f(glGetUniformLocation(basicShader, "offsetSeat"), p.x, p.y);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glUniform1i(glGetUniformLocation(basicShader, "isSeat"), 0);


        
        for (int key = GLFW_KEY_1; key <= GLFW_KEY_9; key++)
        {
            int index = key - GLFW_KEY_1 + 1; // 1..9

            if (glfwGetKey(window, key) == GLFW_PRESS)
            {
                if (!numberKeyPressed[index])
                {
                    numberKeyPressed[index] = true;  // markiraj da je obraden
                    buySeat(index, seatsPerRow);                  // pozovi samo JEDNOM
                }
            }
            else
            {
                numberKeyPressed[index] = false;     // reset kad se taster pusti
            }
        }

        // cleanup uniforms
        glUniform1i(glGetUniformLocation(basicShader, "isSeat"), 0);
        glUniform2f(glGetUniformLocation(basicShader, "offsetSeat"), 0.0, 0.0);

       

        if (overlayVisible)
        {
            glUseProgram(basicShader);
            glUniform1f(glGetUniformLocation(basicShader, "useOffset"), 1.0f);
            glUniform1i(glGetUniformLocation(basicShader, "isDoor"), 0);

            glBindVertexArray(overlayVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


