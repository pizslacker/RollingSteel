#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h> 

#define MAX_CRACKS 10

// --- SHADERS ---

const char* terrainVS =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "out vec2 fragTexCoord;\n"
    "out vec3 fragNormal;\n"
    "out vec3 fragPos;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform vec2 worldOffset;\n"
    "float GetHeight(vec2 p) {\n"
    "    return sin((p.x + worldOffset.x) * 0.08) * 2.2 + cos((p.y + worldOffset.y) * 0.08) * 2.2;\n"
    "}\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord + worldOffset * 0.02;\n"
    "    vec3 pos = vertexPosition;\n"
    "    pos.y = GetHeight(pos.xz);\n"
    "    float delta = 0.1;\n"
    "    float hL = GetHeight(pos.xz - vec2(delta, 0.0));\n"
    "    float hR = GetHeight(pos.xz + vec2(delta, 0.0));\n"
    "    float hD = GetHeight(pos.xz - vec2(0.0, delta));\n"
    "    float hU = GetHeight(pos.xz + vec2(0.0, delta));\n"
    "    vec3 N = normalize(vec3(hL - hR, 2.0 * delta, hD - hU));\n"
    "    fragNormal = normalize(vec3(matModel * vec4(N, 0.0)));\n"
    "    fragPos = vec3(matModel * vec4(pos, 1.0));\n"
    "    gl_Position = mvp * vec4(pos, 1.0);\n"
    "}\n";

const char* terrainFS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec3 fragNormal;\n"
    "in vec3 fragPos;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec3 lightDir;\n"
    "uniform float ballY;\n"
    "uniform vec2 worldOffset;\n"
    
    "// --- Cracks Data ---\n"
    "uniform vec2 crackPos[10];\n"
    "uniform float crackOp[10];\n"

    "void main() {\n"
    "    vec3 normal = normalize(fragNormal);\n"
    "    vec3 light = normalize(lightDir);\n"
    "    vec4 texel = texture(texture0, fragTexCoord);\n"
    "    float diffuse = max(dot(normal, light), 0.0) * 0.7 + 0.3;\n"
    
    "    // --- Soft Drop Shadow ---\n"
    "    float dist = length(fragPos.xz);\n"
    "    float verticalDist = max(ballY - fragPos.y - 1.0, 0.0);\n"
    "    float shadowRadius = 1.0 + (verticalDist * 0.15);\n"
    "    float shadowOpacity = 0.7 * clamp(1.0 - (verticalDist * 0.25), 0.0, 1.0);\n"
    "    float shadow = smoothstep(0.0, shadowRadius, dist);\n"
    "    diffuse *= mix(1.0 - shadowOpacity, 1.0, shadow);\n"
    
    "    // --- Impact Cracks ---\n"
    "    for(int i = 0; i < 10; i++) {\n"
    "        if(crackOp[i] > 0.0) {\n"
    "            vec2 cPos = crackPos[i] - worldOffset;\n" 
    "            vec2 delta = fragPos.xz - cPos;\n"
    "            float cDist = length(delta);\n"
    "            if(cDist < 2.5) {\n"
    "                float angle = atan(delta.y, delta.x);\n"
    "                float radius = 0.3 + 1.2 * abs(sin(angle * 4.0)) * abs(cos(angle * 13.0));\n"
    "                float crackAlpha = smoothstep(radius - 0.1, radius + 0.1, cDist);\n"
    "                diffuse *= mix(1.0 - crackOp[i], 1.0, crackAlpha);\n"
    "            }\n"
    "        }\n"
    "    }\n"
    
    "    finalColor = texel * vec4(vec3(diffuse), 1.0);\n"
    "}\n";

const char* ballVS = 
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec3 vertexNormal;\n"
    "in vec2 vertexTexCoord;\n"
    "out vec3 fragNormal;\n"
    "out vec3 fragPos;\n"
    "out vec2 fragTexCoord;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "void main() {\n"
    "    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    fragNormal = normalize(vec3(matModel * vec4(vertexNormal, 0.0)));\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

const char* ballFS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec3 fragNormal;\n"
    "in vec3 fragPos;\n"
    "out vec4 finalColor;\n"
    "uniform vec3 cameraPos;\n"
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec2 worldOffset;\n"
    "uniform sampler2D texture0;\n"
    "void main() {\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 V = normalize(cameraPos - fragPos);\n"
    "    vec3 L = normalize(lightDir);\n"
    "    float diffuse = max(dot(N, L), 0.0);\n"
    "    vec3 R = reflect(-L, N);\n"
    "    float spec = pow(max(dot(V, R), 0.0), 64.0);\n"
    "    vec3 envReflect = reflect(-V, N);\n"
    "    vec3 skyColor = vec3(0.5, 0.65, 0.8);\n"
    "    vec3 groundColor;\n"
    "    if (envReflect.y < 0.0) {\n"
    "        float t = -fragPos.y / envReflect.y;\n"
    "        vec3 hitPos = fragPos + envReflect * t;\n"
    "        vec2 uv = (hitPos.xz / 150.0) + vec2(0.5) + worldOffset * 0.02;\n"
    "        groundColor = texture(texture0, uv, 4.0).rgb;\n"
    "        float fog = clamp(t * 0.02, 0.0, 1.0);\n"
    "        groundColor = mix(groundColor, skyColor, fog);\n"
    "    } else {\n"
    "        groundColor = skyColor;\n"
    "    }\n"
    "    float envGrnd = smoothstep(-0.05, 0.05, envReflect.y);\n"
    "    vec3 reflectColor = mix(groundColor, skyColor, envGrnd);\n"
    "    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);\n"
    "    vec3 baseMetal = vec3(0.6, 0.62, 0.65);\n"
    "    vec3 finalRGB = baseMetal * (diffuse * lightColor * 0.8 + 0.1) + reflectColor * (fresnel * 0.5 + 0.2) + lightColor * spec;\n"
    "    finalColor = vec4(finalRGB, 1.0);\n"
    "}\n";

const char *fisheyeFS = 
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "void main() {\n"
    "    vec2 p = fragTexCoord - vec2(0.5);\n"
    "    float r2 = dot(p, p);\n"
    "    float distortion = 1.0 + r2 * 1.5;\n"
    "    vec2 uv = p * distortion + vec2(0.5);\n"
    "    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)\n"
    "        finalColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "    else\n"
    "        finalColor = texture(texture0, uv);\n"
    "}\n";

float GetTerrainHeight(float x, float y) {
    return sin(x * 0.08f) * 2.2f + cos(y * 0.08f) * 2.2f;
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, """--^`` RollingSteel ``^--""");

    // --- AUDIO SETUP ---
    InitAudioDevice(); 
    int sampleRate = 44100;
    
    // Jump Sound
    int jumpSamples = sampleRate / 4; 
    short *jumpData = (short *)MemAlloc(jumpSamples * sizeof(short));
    for (int i = 0; i < jumpSamples; i++) {
        float time = (float)i / sampleRate;
        float freq = 300.0f + (time * 1500.0f);
        jumpData[i] = (short)(sinf(2.0f * PI * freq * time) * 16000.0f);
    }
    Wave jumpWave = { .frameCount = jumpSamples, .sampleRate = sampleRate, .sampleSize = 16, .channels = 1, .data = jumpData };
    Sound jumpSound = LoadSoundFromWave(jumpWave);
    UnloadWave(jumpWave);
    
    // Land Sound
    int landSamples = sampleRate / 10; 
    short *landData = (short *)MemAlloc(landSamples * sizeof(short));
    for (int i = 0; i < landSamples; i++) {
        float time = (float)i / sampleRate;
        float freq = 200.0f - (time * 1500.0f); 
        if (freq < 40.0f) freq = 40.0f;
        float env = 1.0f - ((float)i / landSamples); 
        float noise = (float)GetRandomValue(-100, 100) / 100.0f; 
        landData[i] = (short)(((sinf(2.0f * PI * freq * time) * 0.7f) + (noise * 0.3f)) * env * 24000.0f);
    }
    Wave landWave = { .frameCount = landSamples, .sampleRate = sampleRate, .sampleSize = 16, .channels = 1, .data = landData };
    Sound landSound = LoadSoundFromWave(landWave);
    UnloadWave(landWave);

    // Background Music
    Music bgm = LoadMusicStream("drone.mp3");
    PlayMusicStream(bgm);
    // -------------------

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 6.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 55.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader terrainShader = LoadShaderFromMemory(terrainVS, terrainFS);
    Shader ballShader = LoadShaderFromMemory(ballVS, ballFS);
    Shader fisheyeShader = LoadShaderFromMemory(0, fisheyeFS);

    int terrainOffsetLoc = GetShaderLocation(terrainShader, "worldOffset");
    int terrainLightLoc  = GetShaderLocation(terrainShader, "lightDir");
    int terrainBallYLoc  = GetShaderLocation(terrainShader, "ballY"); 
    int crackPosLoc      = GetShaderLocation(terrainShader, "crackPos");
    int crackOpLoc       = GetShaderLocation(terrainShader, "crackOp");
    
    int ballCamPosLoc    = GetShaderLocation(ballShader, "cameraPos");
    int ballLightLoc     = GetShaderLocation(ballShader, "lightDir");
    int ballLightColLoc  = GetShaderLocation(ballShader, "lightColor");
    int ballOffsetLoc    = GetShaderLocation(ballShader, "worldOffset");

    Vector3 lightDir = { 1.0f, 1.0f, 0.5f };
    SetShaderValue(terrainShader, terrainLightLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(ballShader, ballLightLoc, &lightDir, SHADER_UNIFORM_VEC3);

    Vector3 steelBlue = { 0.27f, 0.51f, 0.71f }; 
    SetShaderValue(ballShader, ballLightColLoc, &steelBlue, SHADER_UNIFORM_VEC3);

    Image checkedImg = GenImageChecked(256, 256, 16, 16, RAYWHITE, DARKGRAY);
    Texture2D terrainTex = LoadTextureFromImage(checkedImg);
    GenTextureMipmaps(&terrainTex);
    SetTextureFilter(terrainTex, TEXTURE_FILTER_TRILINEAR); 
    UnloadImage(checkedImg);
    
    Image bgImg = GenImageCellular(512, 512, 32);
    Texture2D bgTex = LoadTextureFromImage(bgImg);
    SetTextureWrap(bgTex, TEXTURE_WRAP_REPEAT); 
    UnloadImage(bgImg);

    Model terrain = LoadModelFromMesh(GenMeshPlane(150.0f, 150.0f, 128, 128));
    terrain.materials[0].shader = terrainShader;
    terrain.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = terrainTex;

    float ballRadius = 1.0f;
    Model ball = LoadModelFromMesh(GenMeshSphere(ballRadius, 64, 64));
    ball.materials[0].shader = ballShader;
    ball.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = terrainTex; 

    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);

    Vector2 terrainOffset = { 0.0f, 0.0f };
    float speed = 12.0f;
    
    Vector3 ballPos = { 0.0f, 0.0f, 0.0f };
    float ballVelocityY = 0.0f;
    float gravity = -25.0f;
    float jumpForce = 12.0f;
    bool isGrounded = false;
    
    Vector2 crackPos[MAX_CRACKS] = { 0 };
    float crackOp[MAX_CRACKS] = { 0 };
    int currentCrack = 0;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateMusicStream(bgm);
        float dt = GetFrameTime();
        
        // --- INPUT ---
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    terrainOffset.y -= speed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  terrainOffset.y += speed * dt;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  terrainOffset.x -= speed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) terrainOffset.x += speed * dt;

        // --- PHYSICS UPDATE ---
        float currentHeight = GetTerrainHeight(terrainOffset.x, terrainOffset.y);
        bool wasGrounded = isGrounded; 
        
        ballVelocityY += gravity * dt; 
        ballPos.y += ballVelocityY * dt; 
        
        if (ballPos.y <= currentHeight + ballRadius) {
            ballPos.y = currentHeight + ballRadius; 
            
            if (!wasGrounded && ballVelocityY < -2.0f) {
                PlaySound(landSound);
                crackPos[currentCrack] = terrainOffset; 
                crackOp[currentCrack] = Clamp(-ballVelocityY / 25.0f, 0.4f, 0.85f); 
                currentCrack = (currentCrack + 1) % MAX_CRACKS; 
            }
            
            ballVelocityY = 0.0f;
            isGrounded = true;
        } else {
            isGrounded = false;
        }
        
        if (IsKeyPressed(KEY_SPACE) && isGrounded) {
            ballVelocityY = jumpForce;
            PlaySound(jumpSound);
        }

        // --- VISUAL UPDATES ---
        for (int i = 0; i < MAX_CRACKS; i++) {
            if (crackOp[i] > 0.0f) {
                crackOp[i] -= 0.15f * dt; 
                if (crackOp[i] < 0.0f) crackOp[i] = 0.0f;
            }
        }

        camera.target = ballPos;
        camera.position.y = ballPos.y + 6.0f;

        SetShaderValue(terrainShader, terrainOffsetLoc, &terrainOffset, SHADER_UNIFORM_VEC2);
        SetShaderValue(terrainShader, terrainBallYLoc, &ballPos.y, SHADER_UNIFORM_FLOAT);
        SetShaderValueV(terrainShader, crackPosLoc, crackPos, SHADER_UNIFORM_VEC2, MAX_CRACKS);
        SetShaderValueV(terrainShader, crackOpLoc, crackOp, SHADER_UNIFORM_FLOAT, MAX_CRACKS);
        
        SetShaderValue(ballShader, ballOffsetLoc, &terrainOffset, SHADER_UNIFORM_VEC2);
        SetShaderValue(ballShader, ballCamPosLoc, &camera.position, SHADER_UNIFORM_VEC3);

        // --- RENDERING ---
        BeginTextureMode(target);
            ClearBackground((Color){ 10, 20, 40, 255 }); 
            
            Rectangle bgSrc1 = { terrainOffset.x * 2.0f, terrainOffset.y * 2.0f, screenWidth / 1.5f, screenHeight / 1.5f };
            DrawTexturePro(bgTex, bgSrc1, (Rectangle){0, 0, screenWidth, screenHeight}, (Vector2){0,0}, 0.0f, Fade(SKYBLUE, 0.3f));
            
            Rectangle bgSrc2 = { terrainOffset.x * 5.0f, terrainOffset.y * 5.0f, screenWidth / 3.0f, screenHeight / 3.0f };
            DrawTexturePro(bgTex, bgSrc2, (Rectangle){0, 0, screenWidth, screenHeight}, (Vector2){0,0}, 0.0f, Fade(WHITE, 0.15f));
            
            BeginMode3D(camera);
                DrawModel(terrain, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                DrawModel(ball, ballPos, 1.0f, WHITE);
            EndMode3D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            
            BeginShaderMode(fisheyeShader);
                Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
                DrawTextureRec(target.texture, sourceRec, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            
            DrawFPS(10, 10);
            DrawText("Use WASD to roll, SPACE to jump", 10, 40, 20, RAYWHITE);
        EndDrawing();
    }

    // --- CLEANUP ---
    UnloadSound(jumpSound);
    UnloadSound(landSound);  
    UnloadMusicStream(bgm);
    CloseAudioDevice();      
    
    UnloadShader(terrainShader);
    UnloadShader(ballShader);
    UnloadShader(fisheyeShader);
    UnloadRenderTexture(target);
    UnloadTexture(terrainTex);
    UnloadTexture(bgTex);
    UnloadModel(terrain);
    UnloadModel(ball);
    CloseWindow();

    return 0;
}