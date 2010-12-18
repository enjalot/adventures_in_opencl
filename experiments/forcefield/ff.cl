#define STRINGIFY(A) #A

std::string kernel_source = STRINGIFY(


float4 explode(float4 p, float cx, float cy)
{
    float4 f = (float4)(0,0,0,0);
    float dsqr = (p.x-cx)*(p.x-cx) + (p.y-cy)*(p.y-cy);
    if(dsqr < .1)
    {
        f.x = -1.0f*(cx - p.x);
        f.y = -1.0f*(cy - p.y);
        float norm = sqrt(f.x*f.x + f.y*f.y);
        f /= norm;
        f *= dsqr*20;
    }
    return f;
}
float4 implode(float4 p, float cx, float cy)
{
    float4 f = (float4)(0,0,0,0);
    float dsqr = (p.x-cx)*(p.x-cx) + (p.y-cy)*(p.y-cy);
    if(dsqr < .1)
    {
        f.x = 1.0f*(cx - p.x);
        f.y = 1.0f*(cy - p.y);
        float norm = sqrt(f.x*f.x + f.y*f.y);
        f /= norm;
        f *= dsqr*20;
    }
    return f;
}

float4 predator_prey(float4 p)
{
    float4 v = (float4)(0,0,0,0);
    int a1 = 14;
    int a2 = 10;
    int b1 = 7;
    int b2 = 2;
    v.x = a1*p.x - b1*p.x*p.y;
    v.y = -a2*p.y + b2*p.y*p.x;
    return v;
}

__kernel void ff(__global float4* pos, __global float4* color, __global float4* vel, __global float4* pos_gen, __global float4* vel_gen, float cx, float cy, float dt, int num)
{
    //get our index in the array
    unsigned int i = get_global_id(0);
    if(i >= num) return;
    //copy position and velocity for this iteration to a local variable
    //note: if we were doing many more calculations we would want to have opencl
    //copy to a local memory array to speed up memory access (this will be the subject of a later tutorial)
    float4 p = pos[i];
    float4 v = vel[i];

    //we've stored the life in the fourth component of our velocity array
    float life = vel[i].w;
    //decrease the life by the time step (this value could be adjusted to lengthen or shorten particle life
    life -= dt*.1f;
    //if the life is 0 or less we reset the particle's values back to the original values and set life to 1
    if(life <= 0)
    {
        p = pos_gen[i];
        v = vel_gen[i];
        life = 1.0;    
    }

    //cx and cy are the center point of the forcefield
    //float4 f = explode(p, cx, cy);
    float4 f = implode(p, cx, cy);
    //v = predator_prey(p);

    v += f*dt;
    p.x += v.x*dt;
    p.y += v.y*dt;

    //we use a first order euler method to integrate the velocity and position (i'll expand on this in another tutorial)
    //update the velocity to be affected by "gravity" in the z direction
    //v.z -= 9.8*dt;
    //update the position with the new velocity
    //p.z += v.z*dt;
    //store the updated life in the velocity array
    v.w = life;

    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;

    //you can manipulate the color based on properties of the system
    //here we adjust the alpha
    float colx = v.x;
    float coly = v.y;
    colx = colx;
    coly = coly;
    if(colx < 0) {colx = -1.0f*colx;}
    if(colx > 1) {colx = 1.0f;}
    if(coly < 0) {coly = -1.0f*coly;}
    if(coly > 1) {coly = 1.0f;}
    color[i].x = .5*(1.0f - colx);
    color[i].y = coly;
    color[i].z = colx;
    color[i].w = life;
    //color[i].w = 1.0f;

}
);
