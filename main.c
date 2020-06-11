#include <stdio.h>
#include <SDL.h>
#include "constants.h"

const int map[MAP_NUM_ROWS][MAP_NUM_COLS] = {
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,1},
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
	normalizeAngle(rayAngle);
	int isRayFacingDown = rayAngle > 0 && rayAngle < PI;
	int isRayFacingUp = !isRayFacingDown;

	int isRayFacingRight = rayAngle <  0.5 * PI || 1.5 * PI;
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
	float nextHorizontalTouxhY = yIntercept;

	//Increment xStep and yStep until we find a wall
	while (nextHorizontalTouchX >= 0 && nextHorizontalTouchX <= WINDOW_WIDTH && nextHorizontalTouxhY >= 0 && nextHorizontalTouxhY <= WINDOW_HEIGHT) {
		float xToCheck = nextHorizontalTouchX;
		float yToCheck = nextHorizontalTouxhY + (isRayFacingUp ? -1 : 0);

		if (mapHasWallAt(xToCheck, yToCheck)) {
			horizontalWallHitX = nextHorizontalTouchX;
			horizontalWallHitY = nextHorizontalTouxhY;
			horizontalWallContent = map[(int)floor(yToCheck / TILE_SIZE)][(int)floor(xToCheck / TILE_SIZE)];
			foundHorizontalWallHit = TRUE;
			break;
		}
		else {
			nextHorizontalTouchX += xStep;
			nextHorizontalTouxhY += yStep;
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
	yIntercept = player.y + (xIntercept - player.x) / tanf(rayAngle);

	//calculate the increment xstep and ystep
	xStep = TILE_SIZE;
	xStep *= isRayFacingLeft ? -1 : 1;

	yStep = TILE_SIZE / tanf(rayAngle);
	yStep *= (isRayFacingUp && xStep > 0) ? -1 : 1;
	yStep *= (isRayFacingDown && xStep < 0) ? -1 : 1;

	float nextVerticalTouchX = xIntercept;
	float nextVerticalTouxhY = yIntercept;

	//Increment xStep and yStep until we find a wall
	while (nextVerticalTouchX >= 0 && nextVerticalTouchX <= WINDOW_WIDTH && nextVerticalTouxhY >= 0 && nextVerticalTouxhY <= WINDOW_HEIGHT) {
		float xToCheck = nextVerticalTouchX + (isRayFacingLeft ? -1 : 0);
		float yToCheck = nextVerticalTouxhY;

		if (mapHasWallAt(xToCheck, yToCheck)) {
			verticalWallHitX = nextVerticalTouchX ;
			verticalWallHitY = nextVerticalTouxhY;
			verticalWallContent = map[(int)floor(yToCheck / TILE_SIZE)][(int)floor(xToCheck / TILE_SIZE)];
			foundVerticalWallHit = TRUE;
			break;
		}
		else {
			nextVerticalTouchX += xStep;
			nextVerticalTouxhY += yStep;
		}
	}

	// Calculate both horizontal and vertica hit distances and chose the smallest one;
	float horizontalHitDistance = foundHorizontalWallHit 
		? distanceBetweenPoints(player.x, player.y, horizontalWallHitX, horizontalWallHitY) 
		: _CRT_INT_MAX;
	float verticalHitDistance = foundVerticalWallHit
		? distanceBetweenPoints(player.x, player.y, verticalWallHitX, verticalWallHitY)
		: _CRT_INT_MAX;

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

void render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// TODO:
	// render all game objects for the current frame
	renderMap();
	//renderRays();
	//renderPlayer();

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