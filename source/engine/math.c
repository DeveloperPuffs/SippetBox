#include <math.h>
#include <string.h>
#include <engine/math.h>

static const size_t FLOAT_SIZE = sizeof(float);
static const size_t MATRIX_SIZE_4x4 = 16 * FLOAT_SIZE;

void matrixIdentity4x4(float matrix[4][4]) {
  memset(matrix, 0, MATRIX_SIZE_4x4);

  matrix[0][0] = 1.0f;
  matrix[1][1] = 1.0f;
  matrix[2][2] = 1.0f;
  matrix[3][3] = 1.0f;
}

void matrixRotate4x4(float matrix[4][4], float angle, const float axis[3]) {
  const float x = axis[0], y = axis[1], z = axis[2];
  const float c = cosf(angle);
  const float s = sinf(angle);
  const float t = 1.0f - c;

  matrixIdentity4x4(matrix);

  matrix[0][0] = t * x * x + c;
  matrix[0][1] = t * x * y - s * z;
  matrix[0][2] = t * x * z + s * y;

  matrix[1][0] = t * y * x + s * z;
  matrix[1][1] = t * y * y + c;
  matrix[1][2] = t * y * z - s * x;

  matrix[2][0] = t * z * x - s * y;
  matrix[2][1] = t * z * y + s * x;
  matrix[2][2] = t * z * z + c;
}

void matrixView4x4(float matrix[4][4], const float pov[3], const float center[3], const float up[3]) {
  float f[3], s[3], u[3];
  float normalized;

  f[0] = center[0] - pov[0];
  f[1] = center[1] - pov[1];
  f[2] = center[2] - pov[2];

  normalized = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
  f[0] /= normalized; f[1] /= normalized; f[2] /= normalized;

  s[0] = f[1] * up[2] - f[2] * up[1];
  s[1] = f[2] * up[0] - f[0] * up[2];
  s[2] = f[0] * up[1] - f[1] * up[0];

  normalized = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
  s[0] /= normalized; s[1] /= normalized; s[2] /= normalized;

  u[0] = s[1] * f[2] - s[2] * f[1];
  u[1] = s[2] * f[0] - s[0] * f[2];
  u[2] = s[0] * f[1] - s[1] * f[0];

  matrixIdentity4x4(matrix);

  matrix[0][0] = s[0];
  matrix[1][0] = s[1];
  matrix[2][0] = s[2];

  matrix[0][1] = u[0];
  matrix[1][1] = u[1];
  matrix[2][1] = u[2];

  matrix[0][2] = -f[0];
  matrix[1][2] = -f[1];
  matrix[2][2] = -f[2];

  matrix[3][0] = -(s[0] * pov[0] + s[1] * pov[1] + s[2] * pov[2]);
  matrix[3][1] = -(u[0] * pov[0] + u[1] * pov[1] + u[2] * pov[2]);
  matrix[3][2] = f[0] * pov[0] + f[1] * pov[1] + f[2] * pov[2];
}

void matrixPerspective4x4(float matrix[4][4], float fov, float aspect, float near, float far) {
  const float tangentHalfFov = tanf(fov / 2.0f);
  memset(matrix, 0, MATRIX_SIZE_4x4);

  matrix[0][0] = 1.0f / (aspect * tangentHalfFov);
  matrix[1][1] = 1.0f / tangentHalfFov;
  matrix[2][2] = -(far + near) / (far - near);
  matrix[2][3] = -1.0f;
  matrix[3][2] = -(2.0f * far * near) / (far - near);
}

void matrixMultiply4x4(float output[4][4], float a[4][4], float b[4][4]) {
  float result[4][4] = {};
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      for (int k = 0; k < 4; ++k) result[i][j] += a[i][k] * b[k][j];
    }
  }

  memcpy(output, result, MATRIX_SIZE_4x4);
}