#pragma once

/**
 * Calculates the cross product of two 3D vectors.
 */
void sippetCross3D(const float vector1[3], const float vector2[3], float result[3]);

/**
 * Normalizes a 3D vector.
 */
void sippetNormalize3D(float vector[3]);

/**
 * Initializes a 4x4 matrix as an identity matrix.
 */
void sippetMatrixIdentity4x4(float matrix[4][4]);

/**
 * Rotates a 4x4 matrix by an angle around an axis.
 */
void sippetMatrixRotate4x4(float matrix[4][4], float angle, const float axis[3]);

/**
 * Creates a view matrix from a point of view, a center, and an up vector.
 */
void sippetMatrixView4x4(float matrix[4][4], const float pov[3], const float center[3], const float up[3]);

/**
 * Creates a perspective projection matrix from a field of view, an aspect ratio, and near and far
 * planes.
 */
void sippetMatrixPerspective4x4(float matrix[4][4], float fov, float aspect, float near, float far);

/**
 * Multiplies two 4x4 matrices together.
 */
void sippetMatrixMultiply4x4(float output[4][4], float a[4][4], float b[4][4]);