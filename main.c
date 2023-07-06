#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

const double WIN_H_PADDING = 70.0;
const double WIN_WIDTH = 800.0;
const double WIN_HEIGHT = 600.0;
const double WIN_WIDTH_2 = WIN_WIDTH / 2.0;

const double PADDLE_WIDTH = 25.0;
const double PADDLE_HEIGHT = 125.0;
const double PADDLE_SPEED = 0.5;
const double PADDLE_WIDTH_2 = PADDLE_WIDTH / 2.0;
const double PADDLE_HEIGHT_2 = PADDLE_HEIGHT / 2.0;

const double BALL_RADIUS = 20.0;

typedef struct Ball
{
  double x; // center
  double y;
  double vx;
  double vy;
} Ball;

typedef struct Paddle
{
  double x;
  double y; // top-left
} Paddle;

typedef struct GameState
{
  int active_paddle;
  Paddle paddle[2];
  Ball ball;
} GameState;

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
      (ball->y >= paddle->y) && (ball->y <= paddle->y + PADDLE_HEIGHT)) {
    ball->vx *= -1.0;
    ball->x = 2.0 * (paddle->x - BALL_RADIUS) - ball->x;
    return true;
  }

  // collision with right boundary
  if ((ball->x <= paddle->x + PADDLE_WIDTH + BALL_RADIUS) &&
      (ball->x >= paddle->x + PADDLE_WIDTH) && (ball->y >= paddle->y) &&
      (ball->y <= paddle->y + PADDLE_HEIGHT)) {
    ball->vx *= -1.0;
    ball->x = 2.0 * (paddle->x + PADDLE_WIDTH + BALL_RADIUS) - ball->x;
    return true;
  }

  double collision_radius_sq =
    (BALL_RADIUS + PADDLE_WIDTH_2) * (BALL_RADIUS + PADDLE_WIDTH_2);

  // collision with top
  if (SqDist(paddle->x + PADDLE_WIDTH_2, paddle->y, ball->x, ball->y) <=
      collision_radius_sq) {
    double paddle_to_ball_x = ball->x - (paddle->x + PADDLE_WIDTH_2);
    double paddle_to_ball_y = ball->y - paddle->y;
    double norm = hypot(paddle_to_ball_x, paddle_to_ball_y);
    paddle_to_ball_x /= norm;
    paddle_to_ball_y /= norm;
    double vel_along_normal =
      paddle_to_ball_x * ball->vx + paddle_to_ball_y * ball->vy;
    ball->vx -= 2 * vel_along_normal * paddle_to_ball_x;
    ball->vy -= 2 * vel_along_normal * paddle_to_ball_y;
    ball->x = paddle->x + PADDLE_WIDTH_2 +
              paddle_to_ball_x * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    ball->y = paddle->y +
              paddle_to_ball_y * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    return true;
  }

  // collision with bottom
  if (SqDist(paddle->x + PADDLE_WIDTH_2,
             paddle->y + PADDLE_HEIGHT,
             ball->x,
             ball->y) <= collision_radius_sq) {
    double paddle_to_ball_x = ball->x - (paddle->x + PADDLE_WIDTH_2);
    double paddle_to_ball_y = ball->y - (paddle->y + PADDLE_HEIGHT);
    double norm = hypot(paddle_to_ball_x, paddle_to_ball_y);
    paddle_to_ball_x /= norm;
    paddle_to_ball_y /= norm;
    double vel_along_normal =
      paddle_to_ball_x * ball->vx + paddle_to_ball_y * ball->vy;
    ball->vx -= 2 * vel_along_normal * paddle_to_ball_x;
    ball->vy -= 2 * vel_along_normal * paddle_to_ball_y;
    ball->x = paddle->x + PADDLE_WIDTH_2 +
              paddle_to_ball_x * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    ball->y = paddle->y + PADDLE_HEIGHT +
              paddle_to_ball_y * (2 * (PADDLE_WIDTH_2 + BALL_RADIUS) - norm);
    return true;
  }

  return false;
}

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

void
RenderPaddle(SDL_Renderer* renderer, Paddle* paddle)
{
  SDL_Rect rect = {
    .x = paddle->x,
    .y = paddle->y,
    .w = PADDLE_WIDTH,
    .h = PADDLE_HEIGHT,
  };
  SDL_RenderFillRect(renderer, &rect);
  RenderCircle(renderer, rect.x + PADDLE_WIDTH_2, rect.y, PADDLE_WIDTH_2);
  RenderCircle(
    renderer, rect.x + PADDLE_WIDTH_2, rect.y + PADDLE_HEIGHT, PADDLE_WIDTH_2);
}

void
RenderGame(SDL_Renderer* renderer, GameState* state)
{
  // Background
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  // Active Paddle
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
  RenderPaddle(renderer, &state->paddle[state->active_paddle]);

  // Inactive Paddle
  SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE);
  RenderPaddle(renderer, &state->paddle[1 - state->active_paddle]);

  // Center Line
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, SDL_ALPHA_OPAQUE);
  SDL_RenderDrawLine(renderer, WIN_WIDTH_2, 0, WIN_WIDTH_2, WIN_HEIGHT);

  // Ball
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
  RenderCircle(renderer, state->ball.x, state->ball.y, BALL_RADIUS);
}

void
AdvanceState(GameState* state, Uint64 time, int push)
{
  const double max_step_time_ms = 5.0;
  const double n_steps = ceil(time / max_step_time_ms);
  const double step_time_ms = time / n_steps;

  for (int i = 0; i < n_steps; ++i) {
    state->ball.x += step_time_ms * state->ball.vx;
    state->ball.y += step_time_ms * state->ball.vy;
    if (state->ball.x >= WIN_WIDTH - BALL_RADIUS) {
      state->ball.x = 2.0 * (WIN_WIDTH - BALL_RADIUS) - state->ball.x;
      state->ball.vx *= -1.0;
    }
    if (state->ball.x <= BALL_RADIUS) {
      state->ball.x = 2.0 * BALL_RADIUS - state->ball.x;
      state->ball.vx *= -1.0;
    }
    if (state->ball.y >= WIN_HEIGHT - BALL_RADIUS) {
      state->ball.y = 2.0 * (WIN_HEIGHT - BALL_RADIUS) - state->ball.y;
      state->ball.vy *= -1.0;
    }
    if (state->ball.y <= BALL_RADIUS) {
      state->ball.y = 2.0 * BALL_RADIUS - state->ball.y;
      state->ball.vy *= -1.0;
    }

    state->paddle[state->active_paddle].y += push * step_time_ms * PADDLE_SPEED;
    state->paddle[state->active_paddle].y =
      Clamp(state->paddle[state->active_paddle].y,
            PADDLE_WIDTH_2,
            WIN_HEIGHT - PADDLE_HEIGHT - PADDLE_WIDTH_2);

    EvaluateCollisionWithPaddle(&state->ball, &state->paddle[0]);
    EvaluateCollisionWithPaddle(&state->ball, &state->paddle[1]);

    if (state->ball.x <= state->paddle[0].x + PADDLE_WIDTH + BALL_RADIUS) {
      state->active_paddle = 0;
    } else if (state->ball.x >= state->paddle[1].x - BALL_RADIUS) {
      state->active_paddle = 1;
    } else {
      state->active_paddle = (state->ball.vx > 0.0);
    }
  }
}

int
main(void)
{
  GameState state = {
    .active_paddle = 1,
    .paddle = {
      [0] = {
        .x = WIN_H_PADDING,
        .y = (WIN_HEIGHT - PADDLE_HEIGHT) / 2.0,
      },
      [1] = {
        .x = WIN_WIDTH - PADDLE_WIDTH - WIN_H_PADDING,
        .y = (WIN_HEIGHT - PADDLE_HEIGHT) / 2.0,
      },
    },
    .ball = {
      .x = WIN_WIDTH / 2.0,
      .y = WIN_HEIGHT / 2.0,
      .vx = 0.3,
      .vy = 0.4,
    },
  };
  int push = 0; // -1 => up, 1 => down

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "Error: %s\n", SDL_GetError());
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

  SDL_Event event;
  bool isRunning = true;

  Uint64 loop_time_ms = 20;
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

    AdvanceState(&state, loop_time_ms, push);
    RenderGame(renderer, &state);
    SDL_RenderPresent(renderer);
    loop_time_ms = SDL_GetTicks64() - now;
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
