#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

int width = 400;
int height = 300;
float paddleSpeed = 0.4;
float ballSpeed = 4;

const int targetFPS = 60;
const double targetFrameTime = 1000.0 / targetFPS;
double frameCount;

typedef struct {
    /* 
        these should be unsigned char 
        this is because unsigned char is 

        0 - 255
    */
    
    unsigned char red;
    unsigned char green;
    unsigned char blue;

    /*
        pretty sure alpha is never really used
        could've just replaced Color->Alpha with 255 each time
    */
    unsigned char alpha;
} Color;

#define createColor(r, g, b) (Color){r, g, b, 255}

typedef struct {
  float x;
  float y;
} PVector;

typedef struct {
    PVector position;
    PVector velocity;
    float width;
    float height;
    Color color;
    
    /* 
    since you only draw rects, you could just check if the width == the height instead 
    having a bool floating around is a bit silly
    */
    bool isCircle; 
} GameObject;

//https://stackoverflow.com/questions/10192903/time-in-milliseconds-in-c

void sleepMilliseconds(long milliseconds) {
    if (milliseconds > 0) {
        struct timespec ts;
        ts.tv_sec = milliseconds / 1000;
        ts.tv_nsec = (milliseconds % 1000) * 1000000;
        nanosleep(&ts, NULL);
    }
}


long long millis(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}


void GameObject_init(GameObject* gameObject, float x, float y, float w, float h, Color c) {
    gameObject->position.x = x;
    gameObject->position.y = y;
    gameObject->velocity.x = 0;
    gameObject->velocity.y = 0;
    gameObject->width = w;
    gameObject->height = h;
    gameObject->color = c;
    gameObject->isCircle = false;
}

/* 
not too sure why this is a function,

it's so short just put it in the main loop
*/
void GameObject_update(GameObject* gameObject) {
  gameObject->position.x += gameObject->velocity.x;
  gameObject->position.y += gameObject->velocity.y;
}

float clamp(float value, float min, float max) {
  if (value < min) {
    return min;
  } else if (value > max) {
    return max;
  }
  return value;
}

bool GameObject_checkCollision(GameObject* gameObject, GameObject* other) {
return (gameObject->position.x < other->position.x + other->width &&
            gameObject->position.x + gameObject->width > other->position.x &&
            gameObject->position.y < other->position.y + other->height &&
            gameObject->position.y + gameObject->height > other->position.y);
}

/* 
    not sure why this is used rather than C's standard random generation

    pretty sure this method would be really slow
*/

int linuxRandom(int from, int to) {
    // Otwarcie /dev/urandom w trybie odczytu
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData == -1) {
        fprintf(stderr, "Błąd: Nie można otworzyć /dev/urandom\n");
        exit(1);
    }

    int range = to - from + 1;
    int randomValue;
    ssize_t bytesRead = read(randomData, &randomValue, sizeof(randomValue));
    if (bytesRead != sizeof(randomValue)) {
        fprintf(stderr, "Błąd: Nie można odczytać losowych danych\n");
        close(randomData);
        exit(1);
    }

    close(randomData);

    // Zamiana otrzymanej losowej wartości na wartość w zakresie [from, to]
    int result = from + abs(randomValue) % range;
    return result;
}

/*
    since this is only used once it really doesn't need to be a function

    could also be a marco if you really wanted it like that
*/

float getDistance(float PositionAX, float PositionAY, float PositionBX, float PositionBY) {
    return sqrt(pow(PositionAX - PositionBX, 2) + pow(PositionAY - PositionBY, 2));
}

void IDrawCircle(XImage* image, float PositionX, float PositionY, float SizeX, Color color) {
    int width = image->width;
    int height = image->height;

    for (int y = PositionY - SizeX; y < PositionY + SizeX; y++) {
        for (int x = PositionX - SizeX; x < PositionX + SizeX; x++) {
            if (x>=0 && x<=width & &y>=0 && y<=height) {
                int i = (y * width + x) * 4;
                if (getDistance(PositionX, PositionY, x, y) < SizeX) {
                    unsigned char* pixel = &image->data[i];
                    pixel[0] = color.blue;     // składowa niebieska
                    pixel[1] = color.green;   // składowa zielona
                    pixel[2] = color.red;   // składowa czerwona
                    pixel[3] = color.alpha; // składowa przezroczystości
                }
            }
        }
    }
}


void IDrawRect(XImage* image, int startX, int startY, int rectWidth, int rectHeight, Color color) {
    int width = image->width;
    int height = image->height;

    for (int y = startY; y < startY + rectHeight; y++) {
        for (int x = startX; x < startX + rectWidth; x++) {
            if (x >= 0 && x <= width && y >= 0 && y <= height) {
                int i = (y * width + x) * 4;
                unsigned char* pixel = &image->data[i];
                pixel[0] = color.blue;     // składowa niebieska
                pixel[1] = color.green;   // składowa zielona
                pixel[2] = color.red;   // składowa czerwona
                pixel[3] = color.alpha; // składowa przezroczystości
            }
        }
    }
}

/* 
probably could ditch the function instead of relying 

on this weird functional/oop style
*/
void GameObject_draw(XImage* image, GameObject* gameObject) {
    if (gameObject->isCircle) IDrawCircle(image, gameObject->position.x, gameObject->position.y, gameObject->width, gameObject->color);
        else IDrawRect(image, gameObject->position.x, gameObject->position.y, gameObject->width, gameObject->height, gameObject->color);
}

void background(XImage* image, Color color) {
        int buffer_size = width * height * 4; // 4 bajty na piksel (RGBA)

    for (int i = 0; i < buffer_size; i += 4) {
        image->data[i] = color.blue;     // składowa niebieska
        image->data[i + 1] = color.green;   // składowa zielona
        image->data[i + 2] = color.red;   // składowa czerwona
        image->data[i + 3] = color.alpha; // składowa przezroczystości
    }
}

int main() {
    Display *display;
    Window window;
    XEvent event;
    const char *window_name = "Proste okno";

    // Inicjalizacja połączenia z serwerem X
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Błąd: Nie można otworzyć połączenia z serwerem X\n");
        return 1;
    }

    // Tworzenie głównego okna
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 400, 300, 0, 0, WhitePixel(display, 0));

    // Ustawianie tytułu okna
    XStoreName(display, window, window_name);

    // Rejestrowanie zainteresowania zdarzeniami
    XSelectInput(display, window, ExposureMask | KeyPressMask);

    // Wyświetlanie okna
    XMapWindow(display, window);

    // Tworzenie bufora

    XImage *image = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, 0), ZPixmap, 0, NULL, width, height, 32, 0);

    int buffer_size = width * height * 4; // 4 bajty na piksel (RGBA)
    image->data = (char *)malloc(buffer_size);

    // Inicjalizacja bufora
    background(image, createColor(200, 200, 200));

    // Wysyłanie bufora do serwera X
    XPutImage(display, window, DefaultGC(display, 0), image, 0, 0, 0, 0, width, height);

    //Tutaj pisz kod przed

    /* 
        you never even freed this array
        this causes a memory leak 

        and this method is pretty messy as it is 

        might as well replace it with a static array (since it's loaded like one anyway)

        probably would be a good idea to replace the array system as a whole tbh

        at least it's not real OOP though 
    */

    const int gameObjects_size = 92;
    GameObject gameObjects[92];

    //paddle
    
    GameObject* paddle = (GameObject*)((char*)gameObjects + sizeof(GameObject) * 0);
    GameObject_init(paddle, width/2-30, height-10, 60, 10, createColor(22, 22, 22));

    //ball
    GameObject* ball = (GameObject*)((char*)gameObjects + sizeof(GameObject) * 1);
    GameObject_init(ball, width/2, height/2, 9, 9, createColor(50, 50, 50));
    ball->isCircle=true;

    //blocks generator

    int objectCounter = 2;
    
    /* 
        you could use your vector struct here if you really wanna be fancy 
        or use an unsigned char array

        using unsigned char here again because, 0 - 255
    */
    unsigned char randomOffsetX = linuxRandom(0, 255);
    unsigned char randomOffsetY = linuxRandom(0, 255);
    
    for (int x = 20; x < width-20; x+=40) {
        for (int y = 15; y < 215; y+=20) {
            GameObject* block = (GameObject*)((char*)gameObjects + sizeof(GameObject) * objectCounter);
            objectCounter++;
            GameObject_init(block, x, y, 40, 20, createColor(x+randomOffsetX, y+randomOffsetY, y));
        }
    }

    long timeStart = millis();

    /* which keys are pressed (left/right key) */
    unsigned char keyPressed = 0;

    bool ballOnPaddle = true;
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask);

    // Główna pętla programu
    while (1) {
        // Obsługa zdarzenia Exposure (np. odświeżenie okna)
        while (XPending(display) > 0) {
            XNextEvent(display, &event);
            if (event.type == Expose) {
                // Obsługa zdarzenia Exposure (np. odświeżenie okna)
                // ...
            } else if (event.type == KeyPress) {
                // Odczytanie naciśniętego klawisza
                KeySym key = XLookupKeysym(&event.xkey, 0);

                // Ustawienie odpowiedniej flagi w zależności od wciśniętego klawisza
                if (key == XK_Left || key == XK_Right) {
                    keyPressed = key % 255;
                } else if (key == XK_space) {
                    if (ballOnPaddle) {
                        ballOnPaddle = false;
                        ball->velocity.y=sqrt(ballSpeed);
                        //ball->velocity.x=paddle->velocity.x;
                    }
                }
            } else if (event.type == KeyRelease) {
                //printf("KeyReleased\n");
                // Odczytanie zwolnionego klawisza
                KeySym key = XLookupKeysym(&event.xkey, 0);

                // Wyzerowanie odpowiedniej flagi w zależności od zwolnionego klawisza
                if (key % 255 == keyPressed) {
                    keyPressed = 0;
                }
            } else if (event.type == ClientMessage) {
                // Obsługa zdarzenia ClientMessage (np. zamknięcie okna)
                Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
                if (event.xclient.data.l[0] == wmDeleteMessage) {
                    // Zdarzenie zamknięcia okna
                    // Przerwanie pętli i zakończenie programu
                    // Zwolnienie pamięci bufora
                    
                    free(image->data);
                    image->data = NULL;
                    XDestroyImage(image);

                    // Zamykanie połączenia z serwerem X
                    XCloseDisplay(display);
                    printf("Program ended!");

                    return 0;
                }
            }
        }

        //paddle movement
        if (keyPressed == XK_Left % 255) {
            paddle->velocity.x -=paddleSpeed;
            if (paddle->position.x < 0) paddle->position.x=0;
        } else if (keyPressed == XK_Right % 255) {
            paddle->velocity.x += paddleSpeed;
            if (paddle->position.x > width-paddle->width) paddle->position.x=width-paddle->width;
        }
        paddle->velocity.x *=0.9;

        //if ball is on paddle update ball position
        if (ballOnPaddle) {
            ball->position.x = paddle->position.x + paddle->width / 2;
            ball->position.y = paddle->position.y - ball->width / 2 - paddle->height / 2;
        }

        //ball-paddle collision
        
        if (GameObject_checkCollision(ball, paddle) && ball->velocity.y > 0) {
            ball->velocity.y*=-1;
            ball->velocity.x+=paddle->velocity.x*0.2;
        }

        background(image, createColor(200, 200, 200));
        for (int i = 0; i < gameObjects_size; i++) {
            GameObject* gameObject = (GameObject*)((char*)gameObjects + sizeof(GameObject) * i);
            if (!(gameObject->width == 0||gameObject->height == 0)) {
                GameObject_update(gameObject);
                GameObject_draw(image, gameObject);
            }
        }

        //ball-wall collision
        if ((ball->position.x < ball->width/2&&ball->velocity.x < 0)||(ball->position.x > width-ball->width&&ball->velocity.x>0)) ball->velocity.x*=-1;
        if (ball->position.y < ball->width/2&&ball->velocity.y < 0) ball->velocity.y*=-1;
        //special ball-floor collison
        if (ball->position.y > height-ball->width&&ball->velocity.y>0) {
            ballOnPaddle = true;
            ball->velocity.x = 0;

            /* 
                no need to redefine any of these
            */
            objectCounter = 2;
            
            randomOffsetX=linuxRandom(0, 255);
            randomOffsetY=linuxRandom(0, 255);
            
            for (int x = 20; x < width-20; x+=40) {
                for (int y = 15; y < 215; y+=20) {
                    GameObject* block = (GameObject*)((char*)gameObjects + sizeof(GameObject) * objectCounter);
                    objectCounter++;
                    GameObject_init(block, x, y, 40, 20, createColor(x+randomOffsetX, y+randomOffsetY, y));
                }
            }
        }

        //ball-blocks collision
        bool ballNeedBounce = false;
        for (int i = 2; i < objectCounter; i++) {
            GameObject* block = (GameObject*)((char*)gameObjects + sizeof(GameObject) * i);
            if (GameObject_checkCollision(ball, block)) {
                block->width=0;
                block->height=0;
                block->position.x=-100;
                ballNeedBounce = true;
            }
        }
        if (ballNeedBounce) ball->velocity.y*=-1;

        XPutImage(display, window, DefaultGC(display, 0), image, 0, 0, 0, 0, width, height);

        frameCount++;
        int nextFrame = ceil((float)(millis()-timeStart)/targetFrameTime);
        sleepMilliseconds(nextFrame*targetFrameTime-(float)(millis()-timeStart));
    }
}
