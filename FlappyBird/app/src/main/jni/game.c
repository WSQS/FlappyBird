#include "android_native_app_glue.h"
#include <GLES2/gl2.h>
#include "game.h"
#include "utils.h"
#include "texture.h"
#include "audio.h"
#include "init.h"
#include "mouse.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define SIZE_SPACE_PIPE 3.3f

#define SPACE_BETWEEN_PIPES 5

//buttons
GLuint t_pause;
GLuint t_ok;
GLuint t_menu;
GLuint t_resume;
GLuint t_score;
GLuint t_share;
GLuint t_start;

//sprites
GLuint t[10];
GLuint t_small[10];

GLuint t_background_day;
GLuint t_base;
GLuint t_bronze_medal;
GLuint t_gameover;
GLuint t_gold_medal;
GLuint t_logo;
GLuint t_message;
GLuint t_new;
GLuint t_panel;
GLuint t_pipe_green;
GLuint t_platinum_medal;
GLuint t_silver_medal;
GLuint t_sparkle_sheet;

// data
int offsetBase = 0;
int gameSpeed = 0;
int score = 0;
int bestScore = 0;
bool newBestScore = false;

int alpha = 0;
bool fadeOut = false;

enum GameState {
    IDLE,
    FADE_IN,
    FADE_OUT,
    READY_GAME,
    GO_GAME,
    STOP_GAME,
    FADE_OUT_GAMEOVER,
    FALL_BIRD,
    FADE_IN_PANEL
} currentState = IDLE;

typedef struct {
    float x, y;
    float velocity;
    float angle;
    float width;
    float height;
    GLuint currentTexture;
    int frame;
    uint64_t lastFrameTime;
} Bird;

typedef struct {
    float x, y;
    float w, h;
    float offset;
} Pipe;

Bird bird;
Pipe pipes[2];

float logoY;
float birdY;
float logoVelocity;
float birdVelocity;
uint64_t timeAnimBirdForLogo;

GLuint birdTexturesForLogo[3];
int currentFrameForLogo = 0;

int fadeOutAlpha = 255;
float panelY = 0;
GLuint medalTexture = 0;

void bird_init(Bird* input_bird)
{
    input_bird->x = ScaleX(18.52f);
    input_bird->y = ScaleY(20.f);
    input_bird->velocity = 0.0f;
    input_bird->angle = 0.0f;
    input_bird->width = ScaleX(11.11f);
    input_bird->height = ScaleY(4.17f);
    input_bird->currentTexture = birdTexturesForLogo[1];
    input_bird->frame = 0;
    input_bird->lastFrameTime = 0;
}

void pipe_offset_init(Pipe* input_pipe)
{
    input_pipe->offset = Random(ScaleY(-SPACE_BETWEEN_PIPES), ScaleY(SPACE_BETWEEN_PIPES));
}

void pipe_init(Pipe* input_pipe)
{
    input_pipe[0].x = ScaleX(100.f);
    input_pipe[0].y = ScaleY(37.5f);
    input_pipe[0].w = ScaleX(15.f);
    input_pipe[0].h = ScaleY(37.5f);
    pipe_offset_init(&input_pipe[0]);
    input_pipe[1].x = ScaleX(100.f) + ScaleX(60.f);
    input_pipe[1].y = ScaleY(37.5f);
    input_pipe[1].w = ScaleX(15.f);
    input_pipe[1].h = ScaleY(37.5f);
    pipe_offset_init(&input_pipe[0]);
}

bool InitGame()
{
    //buttons
    t_pause = LoadTexture("buttons/pause.png");
    t_ok = LoadTexture("buttons/ok.png");
    t_menu = LoadTexture("buttons/menu.png");
    t_resume = LoadTexture("buttons/resume.png");
    t_score = LoadTexture("buttons/score.png");
    t_share = LoadTexture("buttons/share.png");
    t_start = LoadTexture("buttons/start.png");

    //sprites
    for (int i = 0; i < 10; ++i)
    {
        char path[21];
        if (sprintf(path, "sprites/%d.png", i) > 0)
            t[i] = LoadTexture(path);
        else
            Log("%s:%s:sprintf filed,i:%d",__FILE__,__LINE__, i);
    }
    for (int i = 0; i < 10; ++i)
    {
        char path[21];
        if (sprintf(path, "sprites/%d_small.png", i) > 0)
            t[i] = LoadTexture(path);
        else
            Log("%s:%s:sprintf filed,i:%d",__FILE__,__LINE__, i);
    }

    t_background_day = LoadTexture("sprites/background-day.png");
    t_base = LoadTexture("sprites/base.png");
    t_bronze_medal = LoadTexture("sprites/bronze-medal.png");
    t_gameover = LoadTexture("sprites/gameover.png");
    t_gold_medal = LoadTexture("sprites/gold-medal.png");
    t_logo = LoadTexture("sprites/logo.png");
    t_message = LoadTexture("sprites/message.png");
    t_new = LoadTexture("sprites/new.png");
    t_panel = LoadTexture("sprites/panel.png");
    t_pipe_green = LoadTexture("sprites/pipe-green.png");
    t_platinum_medal = LoadTexture("sprites/platinum-medal.png");
    t_silver_medal = LoadTexture("sprites/silver-medal.png");
    t_sparkle_sheet = LoadTexture("sprites/sparkle-sheet.png");
    birdTexturesForLogo[0] = LoadTexture("sprites/yellowbird-downflap.png");
    birdTexturesForLogo[1] = LoadTexture("sprites/yellowbird-midflap.png");
    birdTexturesForLogo[2] = LoadTexture("sprites/yellowbird-upflap.png");
    bird_init(&bird);
    pipe_init(pipes);

    logoY = ScaleY(20.83f);
    birdY = ScaleY(20.83f);
    logoVelocity = 1.1f;
    birdVelocity = 1.1f;

    timeAnimBirdForLogo = getTickCount();

    panelY = ScaleY(100.f);

    newBestScore = false;

    //game speed
    gameSpeed = WindowSizeX / 135; // 1080 / 135 = 8

    //load best score
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/save.txt", g_App->activity->internalDataPath);

    FILE* file = fopen(filePath, "r");
    if (file != NULL)
    {
        int loadbestscore;
        fscanf(file, "%d", &loadbestscore);
        fclose(file);

        bestScore = loadbestscore;
    }

    return true;
}

float MoveTowards(float current, float target, float maxDelta)
{
    if (fabsf(target - current) <= maxDelta)
    {
        return target;
    }
    return current + (target > current ? maxDelta : -maxDelta);
}

void AnimateBird()
{
    uint64_t currentTime = getTickCount();
    if (currentTime - bird.lastFrameTime > 100)
    {
        bird.lastFrameTime = currentTime;
        bird.frame = (bird.frame + 1) % 3;
        bird.currentTexture = birdTexturesForLogo[bird.frame];
    }
}

void ApplyGravity()
{
    bird.velocity += 0.65f;
    bird.y += bird.velocity;

    float targetAngle = bird.velocity > 0 ? 90.0f : -30.0f;
    bird.angle = MoveTowards(bird.angle, targetAngle, 2.0f);

    if (bird.angle > 90.0f) bird.angle = 90.0f;
}

void Jump()
{
    bird.velocity = -13.5f;
    bird.angle = -30.0f;
}

bool CheckCollision()
{
    // detect collision with pipes
    for (int i = 0; i < 2; i++)
    {
        // upper pipe
        float topPipeX = pipes[i].x;
        float topPipeY = pipes[i].y + pipes[i].offset - (bird.height * SIZE_SPACE_PIPE);
        float topPipeWidth = pipes[i].w;
        float topPipeHeight = -(pipes[i].h + pipes[i].offset - (bird.height * SIZE_SPACE_PIPE));

        // lower pipe
        float bottomPipeX = pipes[i].x;
        float bottomPipeY = pipes[i].y + pipes[i].offset;
        float bottomPipeWidth = pipes[i].w;
        float bottomPipeHeight = pipes[i].h - pipes[i].offset;

        // checking collision for upper pipe
        if (bird.x < topPipeX + topPipeWidth &&
            bird.x + bird.width > topPipeX &&
            bird.y < topPipeY &&
            bird.y + bird.height > topPipeY + topPipeHeight)
        {
            return true;
        }

        // checking collision for lower pipe
        if (bird.x < bottomPipeX + bottomPipeWidth &&
            bird.x + bird.width > bottomPipeX &&
            bird.y < bottomPipeY + bottomPipeHeight &&
            bird.y + bird.height > bottomPipeY)
        {
            return true;
        }

        // checking space between pipes
        float gapStartY = topPipeY + topPipeHeight;
        float gapEndY = bottomPipeY;

        if (bird.x < bottomPipeX + bottomPipeWidth &&
            bird.x + bird.width > bottomPipeX &&
            bird.y + bird.height > gapStartY &&
            bird.y < gapEndY)
        {
            // collision not detect for bird
            continue;
        }
    }

    // ground collision
    float baseHeight = ScaleY(75.f);
    if (bird.y + bird.width > baseHeight)
    {
        return true;
    }

    // sky collision
    if (bird.y <= 0)
    {
        return true;
    }

    return false;
}

void RenderBird()
{
    RenderTexturePro(bird.currentTexture, bird.x, bird.y, bird.width, bird.height, bird.angle);
}

void RenderPipes()
{
    for (int i = 0; i < 2; i++)
    {
        RenderTexture(t_pipe_green, pipes[i].x, pipes[i].y + pipes[i].offset - (bird.height * SIZE_SPACE_PIPE),
            pipes[i].w, -(pipes[i].h + pipes[i].offset - (bird.height * SIZE_SPACE_PIPE)));

        RenderTexture(t_pipe_green, pipes[i].x, pipes[i].y + pipes[i].offset, pipes[i].w, pipes[i].h - pipes[i].offset);
    }
}

void RenderBirdTextureForLogo()
{
    uint64_t currentTime = getTickCount();
    if (currentTime - timeAnimBirdForLogo > 100)
    {
        timeAnimBirdForLogo = currentTime;
        currentFrameForLogo = (currentFrameForLogo + 1) % 3;
    }
    RenderTexture(birdTexturesForLogo[currentFrameForLogo], ScaleX(75), birdY, bird.width, bird.height);
}

void RenderLeft(const GLuint* textures, int score, float x, float y, float digitWidth, float digitHeight)
{
    char scoreStr[10];
    sprintf(scoreStr, "%d", score);
    int len = strlen(scoreStr);

    for (int i = 0; i < len; i++)
    {
        int digit = scoreStr[i] - '0';
        GLuint texture = textures[digit];

        RenderTexture(texture, x + i * digitWidth, y, digitWidth, digitHeight);
    }
}

int len(int score)
{
    if (score == 0)return 1;
    return log10(score) + 1;
}
void RenderScoreLeft(int score, float x, float y, float digitWidth, float digitHeight)
{
    RenderLeft(t, score, x, y, digitWidth, digitHeight);
}

void RenderScoreCenter(int score, float x, float y, float digitWidth, float digitHeight)
{
    RenderLeft(t, score, x - (len(score) - 1) / 2.f * digitWidth, y, digitWidth, digitHeight);
}

void RenderSmallScoreLeft(int score, float x, float y, float digitWidth, float digitHeight)
{
    RenderLeft(t_small, score, x, y, digitWidth, digitHeight);
}

void RenderSmallScoreRight(int score, float x, float y, float digitWidth, float digitHeight)
{
    RenderLeft(t_small, score, x - len(score) * digitWidth, y, digitWidth, digitHeight);
}

void Render()
{
    //background
    RenderTexture(t_background_day, 0, 0, ScaleX(100.f), ScaleY(95.83f));

    //cycle base texture
    if (currentState != STOP_GAME && currentState != FADE_OUT_GAMEOVER && currentState != FALL_BIRD && currentState != FADE_IN_PANEL)
    {
        offsetBase -= gameSpeed;
    }

    RenderTexture(t_base, offsetBase, ScaleY(75.f), ScaleX(100.f), ScaleY(25.f));

    if (offsetBase < 0)
    {
        RenderTexture(t_base, ScaleX(100.f) + offsetBase, ScaleY(75.f), ScaleX(100.f), ScaleY(25.f));
    }

    if (offsetBase <= -ScaleX(100.f))
    {
        offsetBase = 0;
    }

    logoY += logoVelocity;
    birdY += birdVelocity;

    if (logoY > ScaleY(20.83f) + 25 || logoY < ScaleY(20.83f) - 25) {
        logoVelocity = -logoVelocity;
    }

    if (birdY > ScaleY(20.83f) + 25 || birdY < ScaleY(20.83f) - 25) {
        birdVelocity = -birdVelocity;
    }

    if (currentState == IDLE || currentState == FADE_IN)
    {
        RenderTexture(t_logo, ScaleX(15.f), logoY, ScaleX(55.56f), ScaleY(5.21f));

        RenderBirdTextureForLogo();


        if (ButtonBump(t_start, ScaleX(10.f), ScaleY(65.f), ScaleX(35.f), ScaleY(6.f)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");
            currentState = FADE_IN;
        }
        
        // button SCORE
        if (ButtonBump(t_score, ScaleX(55.f), ScaleY(65.f), ScaleX(35.f), ScaleY(6.f)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");
        }
    }
    else if (currentState == FADE_OUT || currentState == READY_GAME) //Ready?
    {
        RenderBird();

        RenderTexture(t_message, ScaleX(10.f), ScaleY(9.f), ScaleX(80.f), ScaleY(50.f));
        if (Button(0, 0, ScaleX(100.f), ScaleY(100.f)))
        {
            currentState = GO_GAME;
        }
    }
    else if (currentState == GO_GAME)
    {
        ApplyGravity();
        AnimateBird();

        for (int i = 0; i < 2; i++)
        {
            pipes[i].x -= gameSpeed;
            if (pipes[i].x < -ScaleX(15.f))
            {
                pipes[i].x = ScaleX(115.f);
                pipe_offset_init(&pipes[i]);
            }

            if (bird.x + (bird.width / 2) >= pipes[i].x + pipes[i].w &&
                bird.x + (bird.width / 2) <= pipes[i].x + pipes[i].w + gameSpeed)
            {
                score++;
                PlayAudio("audio/point.mp3");
            }
        }

        if (CheckCollision())
        {
            currentState = STOP_GAME;
            PlayAudio("audio/hit.mp3");
        }

        if (IsClick(0, 0, ScaleX(100.f), ScaleY(100.f)))
        {
            Jump();
            PlayAudio("audio/wing.mp3");
        }

        RenderPipes();
        RenderBird();

        if (score > 0)
            RenderScoreCenter(score, ScaleX(45.f), ScaleY(7.f), ScaleX(8.f), ScaleY(5.f));
    }
    else if (currentState == STOP_GAME)
    {
        if (score > bestScore)
        {
            bestScore = score;
            newBestScore = true;

            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/save.txt", g_App->activity->internalDataPath);

            FILE* file = fopen(filePath, "w");
            if (file != NULL)
            {
                fprintf(file, "%d", bestScore);
                fclose(file);
            }
        }
        currentState = FADE_OUT_GAMEOVER;
    }
    else if (currentState == FADE_OUT_GAMEOVER)
    {
        fadeOutAlpha -= 5;
        if (fadeOutAlpha <= 0)
        {
            fadeOutAlpha = 0;
            currentState = FALL_BIRD;
            PlayAudio("audio/die.mp3");
        }

        RenderPipes();
        RenderBird();

        uint32_t color = 0x00FFFFFF | (fadeOutAlpha << 24);
        CreateBox(color, 0, 0, ScaleX(100.f), ScaleY(100.f));
    }
    else if (currentState == FALL_BIRD)
    {
        ApplyGravity();
        RenderPipes();
        RenderBird();

        if (bird.y + bird.height >= ScaleY(75.f) - bird.height)
        {
            bird.y = ScaleY(75.f) - bird.height;
            currentState = FADE_IN_PANEL;
        }
    }
    else if (currentState == FADE_IN_PANEL)
    {
        RenderPipes();
        RenderBird();

        panelY = MoveTowards(panelY, ScaleY(30), 20.0f);
        RenderTexture(t_panel, ScaleX(15.f), panelY, ScaleX(70.f), ScaleY(17.5f));

        // Render default score
        RenderSmallScoreRight(score, ScaleX(78), panelY + ScaleY(5), ScaleX(4), ScaleY(3));

        // Render best score
        RenderSmallScoreRight(bestScore, ScaleX(78), panelY + ScaleY(11.5f), ScaleX(4), ScaleY(3));

        if (newBestScore)
        {
            RenderTexture(t_new, ScaleX(56.f), panelY + ScaleY(9.f), ScaleX(10.f), ScaleY(1.8f));
        }


        RenderTexture(t_gameover, ScaleX(17.5f), ScaleY(18.f), ScaleX(65.f), ScaleY(6.f));

        // Render medal
        if (score >= 40) medalTexture = t_platinum_medal;
        else if (score >= 30) medalTexture = t_gold_medal;
        else if (score >= 20) medalTexture = t_silver_medal;
        else if (score >= 10) medalTexture = t_bronze_medal;
        else medalTexture = 0;

        if (medalTexture)
        {
            RenderTexture(medalTexture, ScaleX(22.f), panelY + ScaleY(6.f), ScaleX(15.f), ScaleY(7.f));
        }

        // button OK
        if (ButtonBump(t_ok, ScaleX(10.f), ScaleY(65.f), ScaleX(35.f), ScaleY(6.f)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");

            //Reset
            currentState = IDLE;
            score = 0;

            bird_init(&bird);
            pipe_init(pipes);


            panelY = ScaleY(100);

            fadeOutAlpha = 255;

            newBestScore = false;
        }

        // button SHARE
        if (ButtonBump(t_share, ScaleX(55), ScaleY(65), ScaleX(35), ScaleY(6)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");
        }
    }

    if (currentState == FADE_IN)
    {
        alpha += 5;
        if (alpha >= 255)
        {
            alpha = 255;
            currentState = FADE_OUT;
        }
    }
    else if (currentState == FADE_OUT)
    {
        alpha -= 5;
        if (alpha <= 0)
        {
            alpha = 0;
            currentState = READY_GAME;
        }
    }

    // render black screen
    if (currentState == FADE_IN || currentState == FADE_OUT)
    {
        uint32_t color = 0x00000000 | (alpha << 24);
        CreateBox(color, 0, 0, ScaleX(100), ScaleY(100));
    }
}

bool ButtonBump(GLuint textureid, float posX, float posY, float width, float height)
{
    bool released = false;

    if (mouse.isReleased)
    {
        if (IsMouseInSquare(mouse.x, mouse.y, posX, posY, width, height))
        {
            released = true;
        }
    }

    if (released) { posY += ScaleY(1); }

    RenderTexture(textureid, posX, posY, width, height);

    return released;
}

bool Button(float posX, float posY, float width, float height)
{
    bool released = false;

    if (mouse.isReleased)
    {
        if (IsMouseInSquare(mouse.x, mouse.y, posX, posY, width, height))
        {
            released = true;
        }
    }

    return released;
}

bool IsClick(float posX, float posY, float width, float height)
{
    bool down = false;

    if (mouse.isDown)
    {
        if (IsMouseInSquare(mouse.x, mouse.y, posX, posY, width, height))
        {
            down = true;
        }
    }

    return down;
}

void ShutdownGame()
{
    DestroyAudioPlayer();
    DestroyAudioEngine();

    // Delete textures
    glDeleteTextures(1, &t_pause);
    glDeleteTextures(1, &t_ok);
    glDeleteTextures(1, &t_menu);
    glDeleteTextures(1, &t_resume);
    glDeleteTextures(1, &t_score);
    glDeleteTextures(1, &t_share);
    glDeleteTextures(1, &t_start);
    glDeleteTextures(1, &t_ok);
    glDeleteTextures(10, t);
    glDeleteTextures(10, t_small);

    glDeleteTextures(1, &t_background_day);
    glDeleteTextures(1, &t_base);
    glDeleteTextures(1, &t_bronze_medal);
    glDeleteTextures(1, &t_gameover);
    glDeleteTextures(1, &t_gold_medal);
    glDeleteTextures(1, &t_logo);
    glDeleteTextures(1, &t_message);
    glDeleteTextures(1, &t_new);
    glDeleteTextures(1, &t_panel);
    glDeleteTextures(1, &t_pipe_green);
    glDeleteTextures(1, &t_platinum_medal);
    glDeleteTextures(1, &t_silver_medal);
    glDeleteTextures(1, &t_sparkle_sheet);
    glDeleteTextures(3, birdTexturesForLogo);
}