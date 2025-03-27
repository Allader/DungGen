#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define GRID_WIDTH 80
#define GRID_HEIGHT 50
#define CELL_SIZE 16
#define ROOM_MIN_SIZE 4
#define ROOM_MAX_SIZE 10
#define MIN_ROOMS 5
#define MAX_ROOMS 20

typedef enum {
    TILE_EMPTY,
    TILE_WALL,
    TILE_FLOOR,
    TILE_DOOR,
    TILE_STAIRS_DOWN,
    TILE_STAIRS_UP
} TileType;

typedef struct {
    TileType type;
    bool explored;
} Tile;

typedef struct {
    int x, y;
    int width, height;
    int centerX, centerY;
} Room;

Tile dungeon[GRID_WIDTH][GRID_HEIGHT];
Room rooms[MAX_ROOMS];
int targetRoomCount = 10;
int actualRoomCount = 0;
Vector2 startPosition;
Vector2 exitPosition;

void InitDungeon() {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            dungeon[x][y].type = TILE_EMPTY;
            dungeon[x][y].explored = true;
        }
    }
    actualRoomCount = 0;
}

bool CanPlaceRoom(Room newRoom) {
    if (newRoom.x < 1 || newRoom.y < 1 ||
        newRoom.x + newRoom.width >= GRID_WIDTH-1 ||
        newRoom.y + newRoom.height >= GRID_HEIGHT-1) {
        return false;
    }

    for (int i = 0; i < actualRoomCount; i++) {
        if (newRoom.x < rooms[i].x + rooms[i].width + 1 &&
            newRoom.x + newRoom.width > rooms[i].x - 1 &&
            newRoom.y < rooms[i].y + rooms[i].height + 1 &&
            newRoom.y + newRoom.height > rooms[i].y - 1) {
            return false;
        }
    }

    return true;
}

bool IsWallPosition(int x, int y) {
    if (x <= 0 || x >= GRID_WIDTH-1 || y <= 0 || y >= GRID_HEIGHT-1)
        return false;

    //Check for wall in room
    if (dungeon[x][y].type == TILE_WALL) {
        int floorCount = 0;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dungeon[x+dx][y+dy].type == TILE_FLOOR) floorCount++;
            }
        }
        return floorCount > 0;
    }
    return false;
}

void CreateRoom(Room room, bool isStart) {
    for (int x = room.x; x < room.x + room.width; x++) {
        for (int y = room.y; y < room.y + room.height; y++) {
            if (x == room.x || y == room.y || x == room.x + room.width - 1 || y == room.y + room.height - 1) {
                dungeon[x][y].type = TILE_WALL;
            } else {
                dungeon[x][y].type = TILE_FLOOR;
            }
        }
    }

    room.centerX = room.x + room.width / 2;
    room.centerY = room.y + room.height / 2;

    if (isStart) {
        dungeon[room.centerX][room.centerY].type = TILE_STAIRS_UP;
        startPosition.x = (float)room.centerX;
        startPosition.y = (float)room.centerY;
    }

    rooms[actualRoomCount++] = room;
}

void CreateHorizontalTunnel(int x1, int x2, int y) {
    int minX = x1 < x2 ? x1 : x2;
    int maxX = x1 > x2 ? x1 : x2;

    for (int x = minX; x <= maxX; x++) {
        if (dungeon[x][y].type == TILE_EMPTY || dungeon[x][y].type == TILE_WALL) {
            dungeon[x][y].type = TILE_FLOOR;
        }
        if (dungeon[x][y-1].type == TILE_EMPTY) dungeon[x][y-1].type = TILE_WALL;
        if (dungeon[x][y+1].type == TILE_EMPTY) dungeon[x][y+1].type = TILE_WALL;
    }
}

void CreateVerticalTunnel(int y1, int y2, int x) {
    int minY = y1 < y2 ? y1 : y2;
    int maxY = y1 > y2 ? y1 : y2;

    for (int y = minY; y <= maxY; y++) {
        if (dungeon[x][y].type == TILE_EMPTY || dungeon[x][y].type == TILE_WALL) {
            dungeon[x][y].type = TILE_FLOOR;
        }
        if (dungeon[x-1][y].type == TILE_EMPTY) dungeon[x-1][y].type = TILE_WALL;
        if (dungeon[x+1][y].type == TILE_EMPTY) dungeon[x+1][y].type = TILE_WALL;
    }

}

void ConnectRooms(Room room1, Room room2) {
    if (GetRandomValue(0, 1) == 0) {
        CreateHorizontalTunnel(room1.centerX, room2.centerX, room1.centerY);
        CreateVerticalTunnel(room1.centerY, room2.centerY, room2.centerX);
    } else {
        CreateVerticalTunnel(room1.centerY, room2.centerY, room1.centerX);
        CreateHorizontalTunnel(room1.centerX, room2.centerX, room2.centerY);
    }
}

void GenerateDungeon() {
    InitDungeon();

    //starting room
    Room startRoom;
    startRoom.width = GetRandomValue(ROOM_MIN_SIZE, ROOM_MAX_SIZE);
    startRoom.height = GetRandomValue(ROOM_MIN_SIZE, ROOM_MAX_SIZE);
    startRoom.x = GRID_WIDTH/2 - startRoom.width/2;
    startRoom.y = GRID_HEIGHT/2 - startRoom.height/2;
    CreateRoom(startRoom, true);

    //makes the rooms
    for (int i = 0; i < targetRoomCount-1; i++) {
        Room newRoom;
        int attempts = 0;

        do {
            newRoom.width = GetRandomValue(ROOM_MIN_SIZE, ROOM_MAX_SIZE);
            newRoom.height = GetRandomValue(ROOM_MIN_SIZE, ROOM_MAX_SIZE);
            newRoom.x = GetRandomValue(1, GRID_WIDTH - newRoom.width - 2);
            newRoom.y = GetRandomValue(1, GRID_HEIGHT - newRoom.height - 2);
            attempts++;
        } while (!CanPlaceRoom(newRoom) && attempts < 100);

        if (attempts < 100) {
            CreateRoom(newRoom, false);

            if (actualRoomCount > 1) {
                int roomToConnect = GetRandomValue(0, actualRoomCount-2);
                ConnectRooms(rooms[roomToConnect], rooms[actualRoomCount-1]);
            }
        }
    }
    // Furthest room exit
    int farthestRoom = 0;
    float maxDist = 0;

    for (int i = 1; i < actualRoomCount; i++) {
        float dist = sqrtf(powf(rooms[i].centerX - startPosition.x, 2) +
                          powf(rooms[i].centerY - startPosition.y, 2));
        if (dist > maxDist) {
            maxDist = dist;
            farthestRoom = i;
        }
    }

    exitPosition.x = (float)rooms[farthestRoom].centerX;
    exitPosition.y = (float)rooms[farthestRoom].centerY;
    dungeon[rooms[farthestRoom].centerX][rooms[farthestRoom].centerY].type = TILE_STAIRS_DOWN;
}
void DrawDungeon() {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            Color color;
            switch(dungeon[x][y].type) {
                case TILE_WALL: color = DARKGRAY; break;
                case TILE_FLOOR: color = LIGHTGRAY; break;
                case TILE_DOOR: color = BROWN; break;
                case TILE_STAIRS_UP: color = RED; break;
                case TILE_STAIRS_DOWN: color = GREEN; break;
                default: color = BLACK;
            }

            DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
        }
    }

    DrawText("ENTRANCE", (int)startPosition.x * CELL_SIZE - 40, (int)startPosition.y * CELL_SIZE - 20, 12, RED);
    DrawText("EXIT", (int)exitPosition.x * CELL_SIZE - 20, (int)exitPosition.y * CELL_SIZE - 20, 12, GREEN);
    DrawText(TextFormat("Rooms: %d (UP/DOWN to change)", targetRoomCount), 10, 90, 20, WHITE);
}
int main(void) {
    InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Advanced Dungeon Generator");
    SetTargetFPS(60);
    srand(time(NULL));
    GenerateDungeon();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_UP) && targetRoomCount < MAX_ROOMS) {
            targetRoomCount++;
            GenerateDungeon();
        }
        if (IsKeyPressed(KEY_DOWN) && targetRoomCount > MIN_ROOMS) {
            targetRoomCount--;
            GenerateDungeon();
        }
        if (IsKeyPressed(KEY_SPACE)) {
            GenerateDungeon();
        }
        //infos
        BeginDrawing();
        ClearBackground(BLACK);
        DrawDungeon();
        DrawText("SPACE: Generate new dungeon", 10, 10, 20, WHITE);
        DrawText("UP/DOWN: Change room count", 10, 30, 20, WHITE);
        DrawText("Red: Entrance", 10, 50, 20, RED);
        DrawText("Green: Exit", 10, 70, 20, GREEN);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}