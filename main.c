#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;
const int WIN_H_PADDING = 70;

const int PADDLE_WIDTH_2 = 12;
const int PADDLE_WIDTH = 2 * PADDLE_WIDTH_2 + 1;
const int PADDLE_HEIGHT = 150;
const double PADDLE_SPEED = 0.5;

const int BALL_RADIUS = 20;
const int BALL_WIDTH = 2 * BALL_RADIUS + 1;
const int BALL_HEIGHT = BALL_WIDTH;

typedef struct Ball
{
  double x; // center
  double y;
  double vx;
  double vy;
} Ball;

typedef struct Paddle
{
  double x; // bounding box top-left
  double y;
} Paddle;

typedef struct Game
{
  int active_paddle;
  Paddle paddle[2];
  Ball ball;

  SDL_Texture* texture_ball;
  SDL_Texture* texture_active_paddle;
  SDL_Texture* texture_inactive_paddle;
} Game;

void
RenderCircle(SDL_Renderer* renderer, int cx, int cy, int radius)
{
  int x = radius;
  int y = 0;
  int error = 3 - 2 * radius;

  while (x >= y) {
    SDL_RenderDrawLine(renderer, cx - y, cy + x, cx + y, cy + x);
    SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
    SDL_RenderDrawLine(renderer, cx - y, cy - x, cx + y, cy - x);
    SDL_RenderDrawLine(renderer, cx - x, cy - y, cx + x, cy - y);
    if (error > 0) {
      error -= 4 * (--x);
    }
    error += 4 * (++y) + 2;
  }
}

SDL_Texture*
CreateBallTexture(SDL_Renderer* renderer)
{
  SDL_Texture* texture = SDL_CreateTexture(renderer,
                                           SDL_PIXELFORMAT_UNKNOWN,
                                           SDL_TEXTUREACCESS_TARGET,
                                           BALL_WIDTH,
                                           BALL_HEIGHT);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // transparent
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
  RenderCircle(renderer, BALL_RADIUS, BALL_RADIUS, BALL_RADIUS);
  SDL_SetRenderTarget(renderer, NULL);
  return texture;
}

SDL_Texture*
CreatePaddleTexture(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b)
{
  SDL_Texture* texture = SDL_CreateTexture(renderer,
                                           SDL_PIXELFORMAT_UNKNOWN,
                                           SDL_TEXTUREACCESS_TARGET,
                                           PADDLE_WIDTH,
                                           PADDLE_HEIGHT);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // transparent
  SDL_RenderClear(renderer);

  SDL_Rect body = {
    .x = 0,
    .y = PADDLE_WIDTH_2,
    .w = PADDLE_WIDTH,
    .h = PADDLE_HEIGHT - 2 * PADDLE_WIDTH_2,
  };
  SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(renderer, &body);
  RenderCircle(renderer, PADDLE_WIDTH_2, PADDLE_WIDTH_2, PADDLE_WIDTH_2);
  RenderCircle(renderer,
               PADDLE_WIDTH_2,
               PADDLE_HEIGHT - PADDLE_WIDTH_2 - 1,
               PADDLE_WIDTH_2);

  SDL_SetRenderTarget(renderer, NULL);
  return texture;
}

Game*
CreateGame(SDL_Renderer* renderer)
{
  Game* game = (Game*)malloc(sizeof(Game));

  game->active_paddle = 1;

  game->paddle[0].x = WIN_H_PADDING;
  game->paddle[0].y = (WIN_HEIGHT - PADDLE_HEIGHT) / 2.0;

  game->paddle[1].x = WIN_WIDTH - WIN_H_PADDING - PADDLE_WIDTH;
  game->paddle[1].y = (WIN_HEIGHT - PADDLE_HEIGHT) / 2.0;

  game->ball.x = WIN_WIDTH / 2.0;
  game->ball.y = WIN_HEIGHT / 2.0;
  game->ball.vx = 0.3;
  game->ball.vy = 0.4;

  game->texture_ball = CreateBallTexture(renderer);
  game->texture_active_paddle = CreatePaddleTexture(renderer, 0, 0, 255);
  game->texture_inactive_paddle = CreatePaddleTexture(renderer, 50, 50, 50);

  return game;
}

void
DestroyGame(Game* game)
{
  SDL_DestroyTexture(game->texture_ball);
  SDL_DestroyTexture(game->texture_active_paddle);
  SDL_DestroyTexture(game->texture_inactive_paddle);
  free(game);
}

double
Clamp(double x, double a, double b)
{
  return fmin(fmax(x, a), b);
}

double
SqDist(double x1, double y1, double x2, double y2)
{
  return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

bool
EvaluateCollisionWithPaddle(Ball* ball, const Paddle* paddle)
{

  // collision with left boundary
  if ((ball->x >= paddle->x - BALL_RADIUS) && (ball->x <= paddle->x) &&
      (ball->y >= paddle->y + PADDLE_WIDTH_2) &&
      (ball->y <= paddle->y + PADDLE_HEIGHT - PADDLE_WIDTH_2)) {
    ball->vx *= -1.0;
    ball->x = 2.0 * (paddle->x - BALL_RADIUS) - ball->x;
    return true;
  }

  // collision with right boundary
  if ((ball->x <= paddle->x + PADDLE_WIDTH + BALL_RADIUS) &&
      (ball->x >= paddle->x + PADDLE_WIDTH) &&
      (ball->y >= paddle->y + PADDLE_WIDTH_2) &&
      (ball->y <= paddle->y + PADDLE_HEIGHT - PADDLE_WIDTH_2)) {
    ball->vx *= -1.0;
    ball->x = 2.0 * (paddle->x + PADDLE_WIDTH + BALL_RADIUS) - ball->x;
    return true;
  }

  double collision_radius_sq = pow(BALL_RADIUS + PADDLE_WIDTH_2, 2);

  // collision with top
  if (SqDist(paddle->x + PADDLE_WIDTH_2,
             paddle->y + PADDLE_WIDTH_2,
             ball->x,
             ball->y) <= collision_radius_sq) {
    double paddle_to_ball_x = ball->x - (paddle->x + PADDLE_WIDTH_2);
    double paddle_to_ball_y = ball->y - (paddle->y + PADDLE_WIDTH_2);
    double norm = hypot(paddle_to_ball_x, paddle_to_ball_y);
    paddle_to_ball_x /= norm;
    paddle_to_ball_y /= norm;
    double vel_along_normal =
      paddle_to_ball_x * ball->vx + paddle_to_ball_y * ball->vy;
    ball->vx -= 2 * vel_along_normal * paddle_to_ball_x;
    ball->vy -= 2 * vel_along_normal * paddle_to_ball_y;
    ball->x = paddle->x + PADDLE_WIDTH_2 +
              paddle_to_ball_x * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    ball->y = paddle->y + PADDLE_WIDTH_2 +
              paddle_to_ball_y * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    return true;
  }

  // collision with bottom
  if (SqDist(paddle->x + PADDLE_WIDTH_2,
             paddle->y + PADDLE_HEIGHT - PADDLE_WIDTH_2,
             ball->x,
             ball->y) <= collision_radius_sq) {
    double paddle_to_ball_x = ball->x - (paddle->x + PADDLE_WIDTH_2);
    double paddle_to_ball_y =
      ball->y - (paddle->y + PADDLE_HEIGHT - PADDLE_WIDTH_2);
    double norm = hypot(paddle_to_ball_x, paddle_to_ball_y);
    paddle_to_ball_x /= norm;
    paddle_to_ball_y /= norm;
    double vel_along_normal =
      paddle_to_ball_x * ball->vx + paddle_to_ball_y * ball->vy;
    ball->vx -= 2 * vel_along_normal * paddle_to_ball_x;
    ball->vy -= 2 * vel_along_normal * paddle_to_ball_y;
    ball->x = paddle->x + PADDLE_WIDTH_2 +
              paddle_to_ball_x * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    ball->y = paddle->y + PADDLE_HEIGHT - PADDLE_WIDTH_2 +
              paddle_to_ball_y * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    return true;
  }

  return false;
}

void
RenderGame(SDL_Renderer* renderer, Game* game)
{
  // Background
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  // Center Line
  SDL_Rect line = {
    .x = WIN_WIDTH / 2 - 1,
    .y = 0,
    .w = 2 + (WIN_WIDTH % 2),
    .h = WIN_HEIGHT,
  };
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(renderer, &line);

  // Active Paddle
  SDL_Rect bb_paddle0 = {
    .x = game->paddle[game->active_paddle].x,
    .y = game->paddle[game->active_paddle].y,
    .w = PADDLE_WIDTH,
    .h = PADDLE_HEIGHT,
  };
  SDL_RenderCopy(renderer, game->texture_active_paddle, NULL, &bb_paddle0);

  // Inactive Paddle
  SDL_Rect bb_paddle1 = {
    .x = game->paddle[1 - game->active_paddle].x,
    .y = game->paddle[1 - game->active_paddle].y,
    .w = PADDLE_WIDTH,
    .h = PADDLE_HEIGHT,
  };
  SDL_RenderCopy(renderer, game->texture_inactive_paddle, NULL, &bb_paddle1);

  // Ball
  SDL_Rect bb_ball = {
    .x = game->ball.x - BALL_RADIUS,
    .y = game->ball.y - BALL_RADIUS,
    .w = BALL_WIDTH,
    .h = BALL_HEIGHT,
  };
  SDL_RenderCopy(renderer, game->texture_ball, NULL, &bb_ball);
}

void
AdvanceState(Game* game, Uint64 time, int push)
{
  const double max_step_time_ms = 5.0;
  const double n_steps = ceil(time / max_step_time_ms);
  const double step_time_ms = time / n_steps;

  for (int i = 0; i < n_steps; ++i) {
    game->ball.x += step_time_ms * game->ball.vx;
    game->ball.y += step_time_ms * game->ball.vy;
    if (game->ball.x >= WIN_WIDTH - BALL_RADIUS) {
      game->ball.x = 2.0 * (WIN_WIDTH - BALL_RADIUS) - game->ball.x;
      game->ball.vx *= -1.0;
    }
    if (game->ball.x <= BALL_RADIUS) {
      game->ball.x = 2.0 * BALL_RADIUS - game->ball.x;
      game->ball.vx *= -1.0;
    }
    if (game->ball.y >= WIN_HEIGHT - BALL_RADIUS) {
      game->ball.y = 2.0 * (WIN_HEIGHT - BALL_RADIUS) - game->ball.y;
      game->ball.vy *= -1.0;
    }
    if (game->ball.y <= BALL_RADIUS) {
      game->ball.y = 2.0 * BALL_RADIUS - game->ball.y;
      game->ball.vy *= -1.0;
    }

    game->paddle[game->active_paddle].y += push * step_time_ms * PADDLE_SPEED;
    game->paddle[game->active_paddle].y =
      Clamp(game->paddle[game->active_paddle].y, 0, WIN_HEIGHT - PADDLE_HEIGHT);

    EvaluateCollisionWithPaddle(&game->ball, &game->paddle[0]);
    EvaluateCollisionWithPaddle(&game->ball, &game->paddle[1]);

    if (game->ball.x <= game->paddle[0].x + PADDLE_WIDTH + BALL_RADIUS) {
      game->active_paddle = 0;
    } else if (game->ball.x >= game->paddle[1].x - BALL_RADIUS) {
      game->active_paddle = 1;
    } else {
      game->active_paddle = (game->ball.vx > 0.0);
    }
  }
}

void
GameLoop(SDL_Renderer* renderer, Game* game)
{
  bool isRunning = true;
  SDL_Event event;
  Uint64 loop_time_ms = 20;
  int push = 0; // -1 => up, 1 => down

  while (isRunning) {
    Uint64 now = SDL_GetTicks64();

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          isRunning = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              push = -1;
              break;
            case SDLK_DOWN:
              push = 1;
              break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_DOWN:
            case SDLK_UP:
              push = 0;
          }
          break;
      }
    }

    AdvanceState(game, loop_time_ms, push);
    RenderGame(renderer, game);
    SDL_RenderPresent(renderer);
    loop_time_ms = SDL_GetTicks64() - now;
  }
}

int
main(void)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "%s\n", SDL_GetError());
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Window* window = SDL_CreateWindow("Pong",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        WIN_WIDTH,
                                        WIN_HEIGHT,
                                        0);

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  Game* game = CreateGame(renderer);

  GameLoop(renderer, game);

  DestroyGame(game);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return EXIT_SUCCESS;
}
