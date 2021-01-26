/*
 * Készítette: Riskutia Balázs, 2020
 * BME MOGI Tanszék
 */

#include "mechanics.h"

mechanics e; // end-effector mechanics
mechanics b; // ball mechanics

// wall parameters
float wx, wy; // variables denoting wall boundaries
float kwe, kwb, bwe; // parameters used for collision
// Karnopp model parameters - when moving along the wall
float Dp, Dn, Cp, Cn, bp, bn, delta_v;

// pantograph dimensions
float a1, a2, a3, a4, a5;

// auxiliary variables for forward kinematics
float theta1, theta5, x2, y2, x4, y4, x3, y3;
float P2P4, P2Ph, Ph_x, Ph_y, P3Ph;
float ex_prev, ey_prev, evx_raw, evy_raw;

// auxiliary variables for the Jacobian
float del1_y4, del1_x4, del5_y2, del5_x2, d, b_j, h;
float del1_x2, del1_y2, del5_x4, del5_y4, del1_d, del1_b, del1_h;
float del1_yh, del1_xh, del1_y3, del1_x3, del5_y3, del5_x3;
float del5_d, del5_b, del5_h, del5_yh, del5_xh;
float torque_left, torque_right;

// auxiliary variables for end-effector calculations
float ew1, ew2, ewx, ewy, ewx_abs, ewy_abs;

// auxiliary variables for ball calculations
float bw1, bw2, bwx, bwy, bwx_abs, bwy_abs;
float ebx, eby, d, feb_abs;
float ball_force_scaling_factor;

// collision booleans
BOOL effector_wall_collision;
BOOL ball_wall_collision;
BOOL effector_ball_collision;

float g; // gravitational acceleration
float t, delta_t; // time

// function for initializing variables
void init_mechanics() {
    
    // initialize pantograph dimensions
    a1 = 0.180;
    a2 = 0.240;
    a3 = 0.240;
    a4 = 0.180;
    a5 = 0.120;

    // initial values of end-effector properties
    e.k = 0.0f; // won't be used
    e.m = 0.0f; // won't be used
    e.r = 0.01f; // d=20 mm
    e.b = 0.0f; // won't be used
    e.fx = 0.0f;
    e.fy = 0.0f;
    e.fx_active = 0.0f;
    e.fy_active = 0.0f;
    e.ax = 0.0f;
    e.ay = 0.0f;
    e.vx = 0.0f;
    e.vy = 0.0f;

    e.x = 0.0f;
    e.y = 0.0f;

    // initial values of ball properties
    b.k = 3000.0f; // 2000 N/m
    b.m = 0.6f; // 0.3 kg
    b.r = 0.02f; // d=40 mm
    b.b = 0.4f; // damping
    b.fx_active = 0.0f; // won't be used
    b.fy_active = 0.0f; // won't be used
    reset_simulation();
    ball_force_scaling_factor = 0.7f;

    // wall boundaries (absolute values)
    wx = 0.125f;
    wy = 0.080f;
    kwe = 3000.0f; // stiffnes between wall and end-effector
    kwb = 20000.0f; // stiffnes between wall and ball
    bwe = 0.4f; // damping of the wall for the end-effector
    
    // initializing the Karnopp model
    // aluminium on brass
    Dp = 2.1302f;
    Dn = -1.8623f;
    Cp = 1.7714f;
    Cn = 1.729f;
    bp = 0.457f; 
    bn = 0.6196f;
    delta_v = 0.007;

    delta_t = 0.005; // 200 ms
}

/*
 * functions for collision detection (with force calculations)
*/

void collision_detection() {
    effector_wall_cd();
    ball_wall_cd();
    effector_ball_cd();
}

void effector_wall_cd() {
    e.fx = 0.0f;
    e.fy = 0.0f;

    ew1 = e.x - wx;
    ew2 = e.x + wx;
    ewx = fabsf(ew1) < fabsf(ew2) ? ew1 : ew2;

    ew1 = e.y - wy;
    ew2 = e.y + wy;
    ewy = fabsf(ew1) < fabsf(ew2) ? ew1 : ew2;

    ewx_abs = fabsf(ewx);
    ewy_abs = fabsf(ewy);

    if (ewx_abs < e.r) {
        e.fx = kwe * (e.r - ewx_abs);
        e.fx *= ewx > 0 ? 1 : -1;
        
        // add damping to force when moving towards the wall
        if(ewx > 0 && e.vx < 0) e.fx += bwe*fabsf(e.vx);
        if(ewx < 0 && e.vx > 0) e.fx += bwe*fabsf(e.vx);
    }

    if (ewy_abs < e.r) {
        e.fy = kwe * (e.r - ewy_abs);
        e.fy *= ewy > 0 ? 1 : -1;
        
        // add damping to force when moving towards the wall
        if(ewy > 0 && e.vy < 0) e.fy += bwe*fabsf(e.vy);
        if(ewy < 0 && e.vy > 0) e.fy += bwe*fabsf(e.vy);
    }
    
    apply_karnopp_model();
}

void ball_wall_cd() {
    b.fx = 0.0f;
    b.fy = 0.0f;

    bw1 = b.x - wx;
    bw2 = b.x + wx;
    bwx = fabsf(bw1) < fabsf(bw2) ? bw1 : bw2;

    bw1 = b.y - wy;
    bw2 = b.y + wy;
    bwy = fabsf(bw1) < fabsf(bw2) ? bw1 : bw2;

    bwx_abs = fabsf(bwx);
    bwy_abs = fabsf(bwy);

    if (bwx_abs < b.r) {
        b.fx = kwb * (b.r - bwx_abs);
        b.fx *= bwx > 0 ? 1 : -1;
    }

    if (bwy_abs < b.r) {
        b.fy = kwb * (b.r - bwy_abs);
        b.fy *= bwy > 0 ? 1 : -1;
    }
}

void effector_ball_cd() {

    ebx = e.x - b.x;
    eby = e.y - b.y;
    d = sqrt(ebx * ebx + eby * eby);

    if (d < e.r + b.r) {
        feb_abs = b.k * (e.r + b.r - d);
        e.fx += feb_abs * ebx / (e.r + b.r - d);
        e.fy += feb_abs * eby / (e.r + b.r - d);
        b.fx += -1 * e.fx * ball_force_scaling_factor;
        b.fy += -1 * e.fy * ball_force_scaling_factor;
    }
}

/*
 * functions for applying forces on objects
*/

void apply_forces() {
    //apply_passivity_controller();
    apply_forces_on_effector();
    apply_forces_on_ball();
}

void apply_forces_on_effector() {
    compute_jacobian();
    compute_torques();
}

void apply_forces_on_ball() {
    b.ax = (b.fx - b.b*b.vx) / b.m;
    b.ay = (b.fy - b.b*b.vy) / b.m;
}

/*
 * functions for simulating motion of virtual objects
*/

void simulate_motion() {
    move_ball();
}

void move_ball() {
    // calculate velocity from acceleartion and previous velocity
    b.vx_prev = b.vx;
    b.vy_prev = b.vy;
    b.vx = b.ax * delta_t + b.vx;
    b.vy = b.ay * delta_t + b.vy;

    // calculate position from velocity and previous position
    b.x = b.vx * delta_t + b.x;
    b.y = b.vy * delta_t + b.y;

    // if the ball is out of the box
    if (b.x < (-1 * wx - b.r) || b.x > (wx + b.r) || b.y < (-1 * wy - b.r) || b.y > (wx + b.r)) {
        reset_simulation();
    }
}

void reset_simulation() {
    b.fx = 0.0f;
    b.fy = 0.0f;
    b.ax = 0.0f;
    b.ay = 0.0f;
    b.vx = 0.0f;
    b.vy = 0.0f;
    b.vx_prev = 0.0f;
    b.vy_prev = 0.0f;
    b.x = 0.065f;
    b.y = 0.02f;
}

/*
 * advanced haptics techniques
*/

void apply_karnopp_model() {
    if (ewx_abs < e.r) {
        // applying the Karnopp model - when moving along the wall
        if(e.vy < -1*delta_v) e.fy = Cn*sgn(e.vy) + bn*e.vy;
        else if(e.vy >= -1*delta_v && e.vy < 0) e.fy = max(Dn, e.fy);
        else if(e.vy >= 0 && e.vy < delta_v) e.fy = min(Dp, e.fy);
        else if(e.vy >= delta_v) e.fy = Cp*sgn(e.vy) + bp*e.vy;
    }

    if (ewy_abs < e.r) {
        // applying the Karnopp model - when moving along the wall
        if(e.vx < -1*delta_v) e.fx = Cn*sgn(e.vx) + bn*e.vx;
        else if(e.vx >= -1*delta_v && e.vx < 0) e.fx = max(Dn, e.fx);
        else if(e.vx >= 0 && e.vx < delta_v) e.fx = min(Dp, e.fx);
        else if(e.vx >= delta_v) e.fx = Cp*sgn(e.vx) + bp*e.vx;
    }
}

void apply_passivity_controller() {
    // compute for ball
    b.Eobv_x = b.Eobv_x + (b.fx*b.vx+b.alpha_x*b.vx_prev*b.vx_prev)*delta_t;
    b.alpha_x = b.Eobv_x < 0 ? (-1*b.Eobv_x/(delta_t*b.vx*b.vx)) : 0;
    b.fx = b.fx + b.alpha_x*b.vx;
    
    b.Eobv_y = b.Eobv_y + (b.fy*b.vy+b.alpha_y*b.vy_prev*b.vy_prev)*delta_t;
    b.alpha_y = b.Eobv_y < 0 ? (-1*b.Eobv_y/(delta_t*b.vy*b.vy)) : 0;
    b.fy = b.fy + b.alpha_y*b.vy;
    
    // compute for end-effector
    e.Eobv_x = e.Eobv_x + (e.fx*e.vx+e.alpha_x*e.vx_prev*e.vx_prev)*delta_t;
    e.alpha_x = e.Eobv_x < 0 ? (-1*e.Eobv_x/(delta_t*e.vx*e.vx)) : 0;
    e.fx = e.fx + e.alpha_x*e.vx;
    
    e.Eobv_y = e.Eobv_y + (e.fy*e.vy+e.alpha_y*e.vy_prev*e.vy_prev)*delta_t;
    e.alpha_y = e.Eobv_y < 0 ? (-1*e.Eobv_y/(delta_t*e.vy*e.vy)) : 0;
    e.fy = e.fy + e.alpha_y*e.vy;
}

/*
 * functions for calculation of the pantograph mechanism
*/

void fwd_kinematics(float theta1_measured, float theta5_measured) {
    theta1 = theta1_measured + 0.7536f; // theta1_measured + theta1_ref
    theta5 = theta5_measured + 2.3880f; // theta5_measured + theta5_ref

    // Compute position of P2
    x2 = a1*cos(theta1);
    y2 = a1*sin(theta1);

    // Compute position of P4
    x4 = a4*cos(theta5)-a5;
    y4 = a4*sin(theta5);

    // Get distance between P2 and P4
    P2P4 = norm(x4,y4,x2,y2);

    // Compute position of handle
    P2Ph = (a2*a2-a3*a3 + P2P4*P2P4) / (2.0 * P2P4);
    Ph_x   = x2 + (P2Ph/P2P4) * (x4 - x2);
    Ph_y   = y2 + (P2Ph/P2P4) * (y4 - y2);
    P3Ph = sqrt(a2*a2 - P2Ph*P2Ph);
    x3 = Ph_x + (P3Ph/norm(x2,y2,x4,y4)) * (y4 - y2);
    y3 = Ph_y - (P3Ph/norm(x2,y2,x4,y4)) * (x4 - x2);

    ex_prev = e.x;
    ey_prev = e.y;
    e.x = x3+0.06f;
    e.y = y3-0.27f;
    
    // Compute velocity of handle
    evx_raw = (e.x - ex_prev) / delta_t;
    evy_raw = (e.y - ey_prev) / delta_t;

    // Filter velocity (low pass filter)
    e.vx_prev = e.vx;
    e.vy_prev = e.vy;
    e.vx = 0.9f * evx_raw + (1 - 0.9f) * e.vx;
    e.vy = 0.9f * evy_raw + (1 - 0.9f) * e.vy;
}

void compute_jacobian() {
    del1_y4 = 0.0f;
    del1_x4 = 0.0f;
    del5_y2 = 0.0f;
    del5_x2 = 0.0f;

    d = norm(x2,y2,x4,y4);
    b_j = norm(x2,y2,Ph_x,Ph_y);
    h = norm(x3,y3,Ph_x,Ph_y);

    del1_x2 = -a1*sin(theta1);  //NOTE: THE AUTHOR FORGOT NEGATIVE SIGN IN THE PAPER
    del1_y2 =  a1*cos(theta1);
    del5_x4 = -a4*sin(theta5);  //NOTE: THE AUTHOR FORGOT NEGATIVE SIGN IN THE PAPER
    del5_y4 =  a4*cos(theta5);

    //joint 1
    del1_d = ( (x4-x2)*(del1_x4-del1_x2) + (y4-y2)*(del1_y4-del1_y2) ) / d;
    del1_b = del1_d - (del1_d*(a2*a2-a3*a3+d*d))/(2.0*d*d);
    del1_h = -b_j*del1_b / h;

    del1_yh = del1_y2 + (del1_b*d-del1_d*b_j)/(d*d) * (y4-y2) + b_j/d * (del1_y4 - del1_y2);
    del1_xh = del1_x2 + (del1_b*d-del1_d*b_j)/(d*d) * (x4-x2) + b_j/d * (del1_x4 - del1_x2);

    del1_y3 = del1_yh - h/d * (del1_x4-del1_x2) - (del1_h*d - del1_d*h)/(d*d) *(x4 - x2);
    del1_x3 = del1_xh + h/d * (del1_y4-del1_y2) + (del1_h*d - del1_d*h)/(d*d) *(y4 - y2);

    //joint 2
    del5_d = ( (x4-x2)*(del5_x4-del5_x2)+(y4-y2)*(del5_y4-del5_y2) ) / d;
    del5_b = del5_d - (del5_d*(a2*a2-a3*a3+d*d))/(2.0*d*d);
    del5_h = -b_j*del5_b / h;

    del5_yh = del5_y2 + (del5_b*d-del5_d*b_j)/(d*d) * (y4-y2) + b_j/d * (del5_y4 - del5_y2);
    del5_xh = del5_x2 + (del5_b*d-del5_d*b_j)/(d*d) * (x4-x2) + b_j/d * (del5_x4 - del5_x2);

    del5_y3 = del5_yh - h/d * (del5_x4-del5_x2) - (del5_h*d - del5_d*h)/(d*d) * (x4 - x2);
    del5_x3 = del5_xh + h/d * (del5_y4-del5_y2) + (del5_h*d - del5_d*h)/(d*d) * (y4 - y2);
}

void compute_torques() {
    torque_left  = del1_x3 * e.fx + del1_y3 * e.fy;
    torque_right = del5_x3 * e.fx + del5_y3 * e.fy;
}

/*
 * helper functions
 */

float norm(float v1_x, float v1_y, float v2_x, float v2_y) {
    return (float)sqrt((v1_x-v2_x)*(v1_x-v2_x) + (v1_y-v2_y)*(v1_y-v2_y));
}

int sgn(float val) {
    return (0 < val) - (val < 0);
}

float max(float a, float b) {
    if(a > b) return a;
    return b;
}

float min(float a, float b) {
    if(a < b) return a;
    return b;
}