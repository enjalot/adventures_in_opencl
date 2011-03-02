//http://www.cs.sjsu.edu/~rucker/capow/paper/node3.html

__kernel void wave(__global float4* pos, __global float4* color, __global float4* posn1, float k, float dt, float dx, float cll, float clu, int choice)
{
    //get our index in the array
    unsigned int i = get_global_id(0);
    unsigned int imax = get_global_size(0);

    //periodic boundaries don't seem to work unless dx is small enough
    //if(i >= imax-1) return;
    //if(i == 0) return;


    int im1 = i-1;
    int ip1 = i+1;
    im1 = im1 < 0 ? imax-1 : im1;
    ip1 = ip1 > imax ? 0 : ip1;
    float4 pjm1 = pos[im1];     //p(i-1)
    float4 p = pos[i];          //p(i)
    float4 pjp1 = pos[ip1];     //p(i+1)
    float4 pn1 = posn1[i];      //last time step 

    float invdx = 1.f/dx;
    float4 newp = (float4)(0.f,0.f,0.f,0.f);

    if(choice == 1)
    {
        //linear
        //k = c
        newp = 2.f*p - pn1 + k*k*dt*dt*invdx*invdx*(pjp1 - 2.f*p + pjm1);
    }
    else if(choice == 2)
    {
        //quadratic
        float k1 = k*invdx/2.f;
        newp = -pn1 + 2.f*p + dt*dt*invdx*invdx*((pjp1 - 2.f*p + pjm1) + k1*((pjp1 -p)*(pjp1-p) - (p-pjm1)*(p-pjm1)));
    }
    else if(choice == 3)
    {
    //cubic
    //k = gamma
        float k2 = k*invdx*invdx/4.f;
        newp = -pn1 + 2.f*p + dt*dt*invdx*invdx*((pjp1 - 2.f*p + pjm1) + k2*((pjp1 -p)*(pjp1-p)*(pjp1-p) - (p-pjm1)*(p-pjm1)*(p-pjm1)));
    }


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

    //clamp values: still branching, too tired to do it right
    float cllm = newp.y < cll ? cll : newp.y;
    float clum = cllm > clu ? clu : cllm;
    //update the arrays with our newly computed values
    pos[i].y = clum;
    posn1[i] = p;
    posn1[i].w = 1.;
    //vel[i] = v;

    color[i].x = sin(pos[i].y);
    //you can manipulate the color based on properties of the system
    //here we adjust the alpha
    //color[i].w = life;

}

