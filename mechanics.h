/*
 * Készítette: Riskutia Balázs, 2020
 * BME MOGI Tanszék
 */

#ifndef MECHANICS_H
#define	MECHANICS_H

#include <math.h>

#define BYTE unsigned char
#define BOOL unsigned char
#define FALSE 0
#define TRUE 1

typedef struct {
    float k, m, r, b; // material properties
    float fx, fy, fx_active, fy_active; // forces
    float ax, ay; // accelerations
    float vx, vy, vx_prev, vy_prev; // velocities
    float x, y; // positions
    float Eobv_x, alpha_x, Eobv_y, alpha_y; // passivity observer
} mechanics;

extern mechanics e; // end-effector mechanics
extern mechanics b; // ball mechanics

// wall parameters
extern float wx, wy; // variables denoting wall boundaries
extern float kwe, kwb, bwe; // parameters used for collision
// Karnopp model parameters - when moving along the wall
extern float Dp, Dn, Cp, Cn, bp, bn, delta_v;

// pantograph dimensions
extern float a1, a2, a3, a4, a5;

// auxiliary variables for forward kinematics
extern float theta1, theta5, x2, y2, x4, y4, x3, y3;
extern float P2P4, P2Ph, Ph_x, Ph_y, P3Ph;
extern float ex_prev, ey_prev, evx_raw, evy_raw;

// auxiliary variables for the Jacobian
extern float del1_y4, del1_x4, del5_y2, del5_x2, d, b_j, h;
extern float del1_x2, del1_y2, del5_x4, del5_y4, del1_d, del1_b, del1_h;
extern float del1_yh, del1_xh, del1_y3, del1_x3, del5_y3, del5_x3;
extern float del5_d, del5_b, del5_h, del5_yh, del5_xh;
extern float torque_left, torque_right;

// auxiliary variables for end-effector calculations
extern float ew1, ew2, ewx, ewy, ewx_abs, ewy_abs;

// auxiliary variables for ball calculations
extern float bw1, bw2, bwx, bwy, bwx_abs, bwy_abs;
extern float ebx, eby, d, feb_abs;
extern float ball_force_scaling_factor;

// collision booleans
extern BOOL effector_wall_collision;
extern BOOL ball_wall_collision;
extern BOOL effector_ball_collision;

extern float g; // gravitational acceleration
extern float t, delta_t; // time

// function for initializing variables
void init_mechanics();

// functions for collision detection (with force calculations)
void collision_detection();
void effector_wall_cd();
void ball_wall_cd();
void effector_ball_cd();

// functions for applying forces on objects
void apply_forces();
void apply_forces_on_effector();
void apply_forces_on_ball();

// functions for simulating motion of virtual objects
void simulate_motion();
void move_ball();
void reset_simulation();

// advanced haptics techniques
void apply_karnopp_model();
void apply_passivity_controller();

// functions for calculation of the pantograph mechanism
void fwd_kinematics(float theta1_measured, float theta5_measured);
void compute_jacobian();
void compute_torques();

// helper functions
float norm(float v1_x, float v1_y, float v2_x, float v2_y);
int sgn(float val);
float max(float a, float b);
float min(float a, float b);

#endif

