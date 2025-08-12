#include <Windows.h>
#include <cmath>
// == COLOR IDENTIFIER ==
#define WALL_COLOR_R 60
#define WALL_COLOR_G 50
#define WALL_COLOR_B 45

// === COLORS ===
#define SKY_COLOR     RGB(0, 0, 0)        // pure black void
#define WALL_COLOR    RGB(60, 50, 45)     // cold, lifeless stone
#define FLOOR_COLOR   RGB(25, 20, 18)     // dark, worn stone

// ADD THESE NEW ONES:
#define WALL_BRICK    RGB(139, 69, 19)   // brown
#define WALL_STONE    RGB(128, 128, 128) // gray
#define WALL_METAL    RGB(192, 192, 192) // silver

const int screenWidth = 1280;
const int screenHeight = 720;  // 720p

const int mapWidth = 32;
const int mapHeight = 16;

const char map[] =
"################################"  // 32 chars
"#..............................#"
"#.####.#########.#########.####.#"
"#.#..#.#.......#.#.......#.#..#.#"
"#.#..#.#.#####.#.#.#####.#.#..#.#"
"#....#...#...#...#...#...#....#.#"
"######.###.#.#.###.#.###.######.#"
"#......#...#.#.....#.#...#......#"
"#.####.#.###.#######.###.#.####.#"
"#.#....#.............#....#....##" // Fixed this line
"#.#.####.###########.####.#.####"  // Fixed this line  
"#.#......#.........#......#.####"  // Fixed this line
"#.#######.#.#####.#.#######.####"  // Fixed this line
"#.........#.#...#.#.........####"  // Fixed this line
"#.#########.#...#.#########.####"  // Fixed this line
"################################";

float playerX = 3.5f;
float playerY = 3.5f;
float playerAngle = 0.0f;

float fovNormal = 3.14159f / 4.0f;      // 45°
float fovRun = 3.14159f / 3.0f;      // 60°
float fovWide = 3.14159f / 2.0f; // 90°, much wider

float fovCurrent = fovWide;           // this is what rendering uses

float depth = 16.0f;

float lerp(float a, float b, float t) { return a + (b - a) * t; }


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Register window
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"RaycastWindow";
    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(0, L"RaycastWindow", L"maze.",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);

    // Create backbuffer
    BITMAPINFO bmi = { };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = screenWidth;
    bmi.bmiHeader.biHeight = -screenHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* buffer = nullptr;
    HDC hdc = GetDC(hwnd);

    LARGE_INTEGER freq, prev, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);
    
    // Main loop
    MSG msg = { };
    while (true) {
        QueryPerformanceCounter(&now);
        float deltaTime = (float)(now.QuadPart - prev.QuadPart) / freq.QuadPart;
        prev = now;

        bool isRunning = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
        float targetFov = isRunning ? fovRun : fovNormal;

        float moveSpeed = 3.0f * deltaTime; // units per second

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        
            // Player Movement
            if (GetAsyncKeyState('A') & 0x8000) playerAngle -= 1.5f * deltaTime;
            if (GetAsyncKeyState('D') & 0x8000) playerAngle += 1.5f * deltaTime;

            if (GetAsyncKeyState('W') & 0x8000) {
                float newX = playerX + cosf(playerAngle) * moveSpeed;
                float newY = playerY + sinf(playerAngle) * moveSpeed;
                if (map[(int)newY * mapWidth + (int)newX] != '#') {
                    playerX = newX;
                    playerY = newY;
                }
            }
            if (GetAsyncKeyState('S') & 0x8000) {
                float newX = playerX - cosf(playerAngle) * moveSpeed;
                float newY = playerY - sinf(playerAngle) * moveSpeed;
                if (map[(int)newY * mapWidth + (int)newX] != '#') {
                    playerX = newX;
                    playerY = newY;
                }
            }

        // Allocate frame
        static DWORD pixels[screenWidth * screenHeight];

        // Raycasting
        for (int x = 0; x < screenWidth; x++) {
            float rayAngle = (playerAngle - fovCurrent / 2.0f) + ((float)x / (float)screenWidth) * fovCurrent;

            float distanceToWall = 0;
            bool hitWall = false;

            float eyeX = cosf(rayAngle);
            float eyeY = sinf(rayAngle);

            // Step through the ray
            while (!hitWall && distanceToWall < depth) {
                distanceToWall += 0.1f;
                int testX = (int)(playerX + eyeX * distanceToWall);
                int testY = (int)(playerY + eyeY * distanceToWall);

                // Check bounds first
                if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight) {
                    hitWall = true;
                    distanceToWall = depth;  // Set to max distance
                }
                // Check for wall
                else if (map[testY * mapWidth + testX] == '#') {
                    hitWall = true;
                }
            }

            // Calculate wall height
            int wallHeight = (int)(screenHeight / distanceToWall);
            int ceiling = (screenHeight - wallHeight) / 2;
            int floor = ceiling + wallHeight;

            // Clamp values
            if (ceiling < 0) ceiling = 0;
            if (floor >= screenHeight) floor = screenHeight - 1;

            // Calculate brightness
            float brightness = 1.0f - (distanceToWall / depth);
            if (brightness < 0.05f) brightness = 0.05f;

            DWORD shadedWallColor = RGB(
                (int)(60 * brightness),   // Your sad Satan colors
                (int)(50 * brightness),
                (int)(45 * brightness)
            );

            // Draw the column
            for (int y = 0; y < screenHeight; y++) {
                if (y < ceiling) {
                    pixels[y * screenWidth + x] = SKY_COLOR;
                }
                else if (y >= ceiling && y < floor) {
                    pixels[y * screenWidth + x] = shadedWallColor;
                }
                else {
                    pixels[y * screenWidth + x] = FLOOR_COLOR;
                }
            }
        }

        // Draw buffer
        StretchDIBits(hdc, 0, 0, screenWidth, screenHeight,
            0, 0, screenWidth, screenHeight,
            pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);
    }
}
