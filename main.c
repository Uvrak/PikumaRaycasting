#include <stdio.h>
#include <limits.h>
#include <SDL.h>
#include "constants.h"

const int map[MAP_NUM_ROWS][MAP_NUM_COLS] = {
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,1,0,2,0,3,0,4,0,5,0,6,0,7,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,2,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

struct Player {
	float x;
	float y;
	float width;
	float height;
	int turnDirection; // -1 for left, +1 for right
	int walkDirection; // -1 for back, +1 for forward
	float rotationAngle;
	float walkSpeed;
	float turnSpeed;
} player;

struct Ray {
	float rayAngle;
	float wallHitX;
	float wallHitY;
	float distance;
	int isRayFacingUp;
	int isRayFacingDown;
	int isRayFacingLeft;
	int isRayFacingRight;
	int wallHitContent;
	int wasHitVertical;
} rays[NUM_RAYS];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL; 
int isGameRunning = FALSE;
int ticksLastFrame;

Uint32* colorBuffer = NULL;
SDL_Texture* colorBufferTexture;

int initializeWindow() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initializing SDL.\n");
		return FALSE;
	}
	window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_BORDERLESS
	);

	if (!window) {
		fprintf(stderr, "Error creating SDL window .\n");
		return FALSE;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Error creating SDL renderer.\n");
		return FALSE;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	return TRUE;
}

void destroyWindow() {
	free(colorBuffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void setup() {
	// TODO:
	// initialize and setup game objects
	player.x = WINDOW_WIDTH / 2;
	player.y = WINDOW_HEIGHT / 2;
	player.width = 5;
	player.height = 5;
	player.turnDirection = 0;
	player.walkDirection = 0;
	player.rotationAngle = PI / 2;
	player.walkSpeed = 100;
	player.turnSpeed = 45 * (PI / 180);

	//allocate the total amount of bytes in memory to hold our colorbuffer
	colorBuffer = malloc(sizeof(Uint32) * (Uint32)WINDOW_WIDTH * (Uint32)WINDOW_HEIGHT);

	// create an SDL_Texture to display the colorBuffer
	colorBufferTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH,
		WINDOW_HEIGHT
	);
}

void renderPlayer() {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_Rect playerRect = {
		(int)(MINIMAP_SCALE_FACTOR * player.x),
		(int)(MINIMAP_SCALE_FACTOR * player.y),
		(int)(MINIMAP_SCALE_FACTOR * player.width),
		(int)(MINIMAP_SCALE_FACTOR * player.height)
	};

	SDL_RenderFillRect(renderer, &playerRect);

	SDL_RenderDrawLine(
		renderer,
		(int)(MINIMAP_SCALE_FACTOR * player.x),
		(int)(MINIMAP_SCALE_FACTOR * player.y),
		(int)(MINIMAP_SCALE_FACTOR * player.x + cosf(player.rotationAngle) * 40),
		(int)(MINIMAP_SCALE_FACTOR * player.y + sinf(player.rotationAngle) * 40)
		);

}

int mapHasWallAt(float x, float y) {
	if (x < 0 || x > WINDOW_WIDTH || y < 0 || y > WINDOW_HEIGHT) {
		return TRUE;
	}
	int mapGridIndexX = (int)(floor(x / TILE_SIZE));
	int mapGridIndexY = (int)(floor(y / TILE_SIZE));

	return map[mapGridIndexY][mapGridIndexX] != 0;
}

void movePlayer(float perSecond) {
	player.rotationAngle += player.turnDirection * player.turnSpeed * perSecond;
	float moveStep = player.walkDirection * player.walkSpeed * perSecond;

	float newPlayerX = player.x + cosf(player.rotationAngle) * moveStep;
	float newPlayerY = player.y + sinf(player.rotationAngle) * moveStep;
	//TODO:
	//perform wall collision
	if (!mapHasWallAt(newPlayerX, newPlayerY)) {
		player.x = newPlayerX;
		player.y = newPlayerY;
	}
}

float normalizeAngle(float angle) {
	angle = remainderf(angle, TWO_PI);
	if (angle < 0) {
		angle = TWO_PI + angle;
	}
	return angle;
}

float distanceBetweenPoints(float x1, float y1, float x2, float y2) {
	return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

void castRay(float rayAngle, int stripId) {
	//TODO: All that crazy logic for horz, vert, ...
	rayAngle = normalizeAngle(rayAngle);
	int isRayFacingDown = (rayAngle > 0) && (rayAngle < PI);
	int isRayFacingUp = !isRayFacingDown;

	int isRayFacingRight = (rayAngle <  0.5 * PI) || (rayAngle > 1.5 * PI);
	int isRayFacingLeft = !isRayFacingRight;

	float xIntercept, yIntercept;
	float xStep, yStep;

	////////////////////////////////////////////////////////////
	// Horizontal RAY-GRID Intersection CODE
	////////////////////////////////////////////////////////////

	int foundHorizontalWallHit = FALSE;
	float horizontalWallHitX = 0;
	float horizontalWallHitY = 0;
	int horizontalWallContent = 0;

	//Find the y-coordinate of the closest horizontal grid intersection
	yIntercept = floorf(player.y / TILE_SIZE) * TILE_SIZE;
	yIntercept += isRayFacingDown ? TILE_SIZE : 0;

	//Find the x- coordinate of the closest horizontal grid intersection
	xIntercept = player.x + (yIntercept - player.y) / tanf(rayAngle);

	//calculate the increment xstep and ystep
	yStep = TILE_SIZE;
	yStep *= isRayFacingUp ? -1 : 1;

	xStep = TILE_SIZE / tanf(rayAngle);
	xStep *= (isRayFacingLeft && xStep > 0) ? -1 : 1;
	xStep *= (isRayFacingRight && xStep < 0) ? -1 : 1;

	float nextHorizontalTouchX = xIntercept;
	float nextHorizontalTouchY = yIntercept;

	//Increment xStep and yStep until we find a wall
	while (nextHorizontalTouchX >= 0 && nextHorizontalTouchX <= WINDOW_WIDTH && nextHorizontalTouchY >= 0 && nextHorizontalTouchY <= WINDOW_HEIGHT) {
		float xToCheck = nextHorizontalTouchX;
		float yToCheck = nextHorizontalTouchY + (isRayFacingUp ? -1 : 0);

		if (mapHasWallAt(xToCheck, yToCheck)) {
			horizontalWallHitX = nextHorizontalTouchX;
			horizontalWallHitY = nextHorizontalTouchY;
			horizontalWallContent = map[(int)floorf(yToCheck / TILE_SIZE)][(int)floorf(xToCheck / TILE_SIZE)];
			foundHorizontalWallHit = TRUE;
			break;
		}
		else {
			nextHorizontalTouchX += xStep;
			nextHorizontalTouchY += yStep;
		}
	}	

	////////////////////////////////////////////////////////////
	// Vertical RAY-GRID Intersection CODE
	////////////////////////////////////////////////////////////
    
	int foundVerticalWallHit = FALSE;
	float verticalWallHitX = 0;
	float verticalWallHitY = 0;
	int verticalWallContent = 0;

	//Find the x-coordinate of the closest horizontal grid intersection
	xIntercept = floorf(player.x / TILE_SIZE) * TILE_SIZE;
	xIntercept += isRayFacingRight ? TILE_SIZE : 0;

	//Find the y- coordinate of the closest horizontal grid intersection
	yIntercept = player.y + (xIntercept - player.x) * tanf(rayAngle);

	//calculate the increment xstep and ystep
	xStep = TILE_SIZE;
	xStep *= isRayFacingLeft ? -1 : 1;

	yStep = TILE_SIZE * tanf(rayAngle);
	yStep *= (isRayFacingUp && yStep > 0) ? -1 : 1;
	yStep *= (isRayFacingDown && yStep < 0) ? -1 : 1;

	float nextVerticalTouchX = xIntercept;
	float nextVerticalTouchY = yIntercept;

	//Increment xStep and yStep until we find a wall
	while (nextVerticalTouchX >= 0 && nextVerticalTouchX <= WINDOW_WIDTH && nextVerticalTouchY >= 0 && nextVerticalTouchY <= WINDOW_HEIGHT) {
		float xToCheck = nextVerticalTouchX + (isRayFacingLeft ? -1 : 0);
		float yToCheck = nextVerticalTouchY;

		if (mapHasWallAt(xToCheck, yToCheck)) {
			verticalWallHitX = nextVerticalTouchX ;
			verticalWallHitY = nextVerticalTouchY;
			verticalWallContent = map[(int)floor(yToCheck / TILE_SIZE)][(int)floor(xToCheck / TILE_SIZE)];
			foundVerticalWallHit = TRUE;
			break;
		}
		else {
			nextVerticalTouchX += xStep;
			nextVerticalTouchY += yStep;
		}
	}

	// Calculate both horizontal and vertica hit distances and chose the smallest one;
	float horizontalHitDistance = foundHorizontalWallHit 
		? distanceBetweenPoints(player.x, player.y, horizontalWallHitX, horizontalWallHitY) 
		: INT_MAX;
	float verticalHitDistance = foundVerticalWallHit
		? distanceBetweenPoints(player.x, player.y, verticalWallHitX, verticalWallHitY)
		: INT_MAX;

	if (verticalHitDistance < horizontalHitDistance) {
		rays[stripId].distance = verticalHitDistance;
		rays[stripId].wallHitX = verticalWallHitX;
		rays[stripId].wallHitY = verticalWallHitY;
		rays[stripId].wallHitContent = verticalWallContent;
		rays[stripId].wasHitVertical = TRUE;
	}
	else {
		rays[stripId].distance = horizontalHitDistance;
		rays[stripId].wallHitX = horizontalWallHitX;
		rays[stripId].wallHitY = horizontalWallHitY;
		rays[stripId].wallHitContent = horizontalWallContent;
		rays[stripId].wasHitVertical = FALSE;
	}

	rays[stripId].rayAngle = rayAngle;
	rays[stripId].isRayFacingDown = isRayFacingDown;
	rays[stripId].isRayFacingUp = isRayFacingUp;
	rays[stripId].isRayFacingLeft = isRayFacingLeft;
	rays[stripId].isRayFacingRight = isRayFacingRight;

}

void castAllRays() {
	// start first ray substracting half of our FOV
	float rayAngle = player.rotationAngle - (FOV_ANGLE / 2);

	for (int stripId = 0; stripId < NUM_RAYS; stripId++) {
		castRay(rayAngle, stripId);
		rayAngle += FOV_ANGLE / NUM_RAYS;
	}
}

void renderMap() {
	for (int r = 0; r < MAP_NUM_ROWS; r++) {
		for (int c = 0; c < MAP_NUM_COLS; c++) {
			int tileX = c * TILE_SIZE;
			int tileY = r * TILE_SIZE;
			int tileColor = map[r][c] != 0 ? 255 : 0;

			SDL_SetRenderDrawColor(renderer, tileColor, tileColor, tileColor, 255);
			SDL_Rect mapTileRect = {
				(int)(tileX * MINIMAP_SCALE_FACTOR),
				(int)(tileY * MINIMAP_SCALE_FACTOR),
				(int)(TILE_SIZE * MINIMAP_SCALE_FACTOR),
				(int)(TILE_SIZE * MINIMAP_SCALE_FACTOR)
			};
			SDL_RenderFillRect(renderer, &mapTileRect);
		}
	}
}

void renderRays() {
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for (int r = 0; r < NUM_RAYS; r++) {
		SDL_RenderDrawLine(
			renderer,
			(int)(MINIMAP_SCALE_FACTOR * player.x),
			(int)(MINIMAP_SCALE_FACTOR * player.y),
			(int)(MINIMAP_SCALE_FACTOR * rays[r].wallHitX),
			(int)(MINIMAP_SCALE_FACTOR * rays[r].wallHitY)
		);
	};
}

void processInput() {
	SDL_Event event;
	SDL_PollEvent(&event);
	switch (event.type) {
		case SDL_QUIT: {
			isGameRunning = FALSE;
			break;
		}
		case SDL_KEYDOWN: {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				isGameRunning = FALSE;
			}
			if (event.key.keysym.sym == SDLK_UP) {
				player.walkDirection = +1;
			}
			if (event.key.keysym.sym == SDLK_DOWN) {
				player.walkDirection = -1;
			}
			if (event.key.keysym.sym == SDLK_LEFT) {
				player.turnDirection = -1;
			}
			if (event.key.keysym.sym == SDLK_RIGHT) {
				player.turnDirection = +1;
			}

			break;
		}
		case SDL_KEYUP: {
			if (event.key.keysym.sym == SDLK_UP) {
				player.walkDirection = 0;
			}
			if (event.key.keysym.sym == SDLK_DOWN) {
				player.walkDirection = 0;
			}
			if (event.key.keysym.sym == SDLK_LEFT) {
				player.turnDirection = 0;
			}
			if (event.key.keysym.sym == SDLK_RIGHT) {
				player.turnDirection = 0;
			}
			
			break;
		}
	}
}

void update() {
	// wate some time until we reach the target frame time left
	while (!SDL_TICKS_PASSED(SDL_GetTicks(), ticksLastFrame + FRAME_TIME_LENGTH));
	float perSecond = (SDL_GetTicks() - ticksLastFrame) / 1000.0f;
	ticksLastFrame = SDL_GetTicks();
	
	//TODO: remember to update game objject as a function of perSecond
	movePlayer(perSecond);
	castAllRays();
}

void generate3DProjection() {
	for (int i = 0; i < NUM_RAYS; i++) {
		float perpDistance = rays[i].distance * cosf(rays[i].rayAngle - player.rotationAngle);
		float distanceProjectionPlane = (WINDOW_WIDTH / 2) / tanf(FOV_ANGLE / 2);
		float projectedWallHeight = (TILE_SIZE / perpDistance) * distanceProjectionPlane;

		int wallStripHeight = (int)projectedWallHeight;
		int wallTopPixel = (WINDOW_HEIGHT / 2) - (wallStripHeight / 2);
		wallTopPixel = wallTopPixel < 0 ? 0 : wallTopPixel;
		int wallBottomPixel = (WINDOW_HEIGHT / 2) + (wallStripHeight / 2);
		wallBottomPixel = wallBottomPixel > WINDOW_HEIGHT ? WINDOW_HEIGHT : wallBottomPixel;


		//TODO: render the wall from wallTopPixel to WallBottomPixel
		for (int y = 0; y < wallTopPixel; y++) {
		
			colorBuffer[(WINDOW_WIDTH * y) + i] = 0xFF333333;
		}
		for (int y = wallTopPixel; y < wallBottomPixel; y++) {
			colorBuffer[(WINDOW_WIDTH * y) + i] = rays[i].wasHitVertical ? 0xFFFFFFFF : 0xFFCCCCCC ;
		}

		for (int y = wallBottomPixel; y < WINDOW_HEIGHT; y++) {

			colorBuffer[(WINDOW_WIDTH * y) + i] = 0xFF777777;
		}


	}
}

void clearColorBuffer(Uint32 color) {
	for (int x = 0; x < WINDOW_WIDTH; x++) {
		for (int y = 0; y < WINDOW_HEIGHT; y++) {
			colorBuffer[(WINDOW_WIDTH * y) + x] = color;
		}
	}
}

void renderColorBuffer() {
	SDL_UpdateTexture(
		colorBufferTexture,
		NULL,
		colorBuffer,
		(int)((Uint32)WINDOW_WIDTH * sizeof(Uint32))
	);

	SDL_RenderCopy(
		renderer,
		colorBufferTexture,
		NULL,
		NULL
	);
};

void render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	// clear the color buffer

	generate3DProjection();
	renderColorBuffer();
	clearColorBuffer(0xFF000000);
	// TODO:
	// render all game objects for the current frame
	renderMap();
	renderRays();
	renderPlayer();

	SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
	isGameRunning = initializeWindow();

	setup();

	while (isGameRunning) {
		processInput();
		update();
		render();
	}
	destroyWindow();
	return 0;
}