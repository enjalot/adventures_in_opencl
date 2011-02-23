//http://www.cs.sjsu.edu/~rucker/capow/paper/node3.html

__kernel void linear_wave(__global float4* pos, __global float4* color, __global float4* posn1, float c, float dt, float dx)
{
    //get our index in the array
    unsigned int i = get_global_id(0);
    unsigned int imax = get_global_size(0);

    //periodic boundaries don't seem to work
    if(i >= imax-1) return;
    if(i == 0) return;


    int im1 = i-1;
    int ip1 = i+1;
    im1 = im1 < 0 ? imax-1 : im1;
    ip1 = ip1 > imax ? 0 : ip1;
    float4 pjm1 = pos[im1];     //p(i-1)
    float4 p = pos[i];          //p(i)
    float4 pjp1 = pos[ip1];     //p(i+1)
    float4 pn1 = posn1[i];      //last time step 

    float4 newp = 2.f*p - pn1 + c*c*dt*dt*(pjp1 - 2.f*p + pjm1)/dx/dx;

/* LIFE STUFF
    //we've stored the life in the fourth component of our velocity array
    float life = vel[i].w;
    //decrease the life by the time step (this value could be adjusted to lengthen or shorten particle life
    life -= dt;
    //if the life is 0 or less we reset the particle's values back to the original values and set life to 1
    if(life <= 0)
    {
        p = pos_gen[i];
        v = vel_gen[i];
        life = 1.0;    
    }
*/

    //we use a first order euler method to integrate the velocity and position (i'll expand on this in another tutorial)
    //update the velocity to be affected by "gravity" in the z direction
    //v.z -= 9.8*dt;
    //update the position with the new velocity
    /*
    p.x += v.x*dt;
    p.y += v.y*dt;
    p.z += v.z*dt;
    */
    //store the updated life in the velocity array
    //v.w = life;

    //update the arrays with our newly computed values
    pos[i] = newp;
    pos[i].w = 1.;
    posn1[i] = p;
    posn1[i].w = 1.;
    //vel[i] = v;

    //you can manipulate the color based on properties of the system
    //here we adjust the alpha
    //color[i].w = life;

}

__kernel void quadratic_wave(__global float4* pos, __global float4* color, __global float4* posn1, float beta, float dt, float dx)
{
    //get our index in the array
    unsigned int i = get_global_id(0);
    unsigned int imax = get_global_size(0);

    //periodic boundaries don't seem to work
    if(i >= imax-1) return;
    if(i == 0) return;


    int im1 = i-1;
    int ip1 = i+1;
    im1 = im1 < 0 ? imax-1 : im1;
    ip1 = ip1 > imax ? 0 : ip1;
    float4 pjm1 = pos[im1];     //p(i-1)
    float4 p = pos[i];          //p(i)
    float4 pjp1 = pos[ip1];     //p(i+1)
    float4 pn1 = posn1[i];      //last time step 

    float invdx = 1.f/dx;
    //float4 newp = 2.f*p - pn1 + c*c*dt*dt*(pjp1 - 2.f*p + pjm1)/dx/dx;
    float k1 = beta*invdx/2.f;
    float4 newp = -pn1 + 2.f*p + dt*dt*invdx*invdx*((pjp1 - 2.f*p + pjm1) + k1*((pjp1 -p)*(pjp1-p) - (p-pjm1)*(p-pjm1)));

    //update the arrays with our newly computed values
    pos[i] = newp;
    pos[i].w = 1.;
    posn1[i] = p;
    posn1[i].w = 1.;

}


