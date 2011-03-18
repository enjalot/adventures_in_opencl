//http://www.cs.sjsu.edu/~rucker/capow/paper/node3.html

float calc_wave(float4 pjm1, float4 p, float4 pjp1, float4 pn1, int choice, float k, float ymin, float ymax, float dt, float dx)
{

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

    //clamp values: still branching, too tired to do it right
    float yminm = newp.y < ymin ? ymin : newp.y;
    float ymaxm = yminm > ymax ? ymax : yminm;

    return ymaxm;
}

__kernel void wave(__global float4* pos, 
                   __global float4* color, 
                   __global float4* posn1, 
                   int ntracers, 
                   int choice,
                   int num,
                   float k, 
                   float ymin, 
                   float ymax, 
                   float dt, 
                   float dx 
                   )
{
    //get our index in the array
    unsigned int ind = get_global_id(0);
    //unsigned int imax = get_global_size(0);
    int i, imin, imax;
    int imind = 0;
    int imaxd = num-1;

    //periodic boundaries don't seem to work unless dx is small enough
    //if(i >= imax-1) return;
    //if(i == 0) return;
    //pos[i].y = i / 43.f;
    //pos[i].w = 1.f;
#if 1
    //This calculates a different wave for each set
    //instead we could simply do copying in this way (instead of calculating
    //if we want to calculate history we just need to start their initial conditions
    //staggered
    for(int j = 0; j < ntracers; j++)
    //for(int j = 0; j < 7; j++)
    {

        //i = ind + num*j;
        //i += num;
        i = ind + j * num;
        imin = imind + j * num;
        imax = imaxd + j * num;

        dt += dt/ntracers;

        int im1 = i-1;
        int ip1 = i+1;
        im1 = im1 < imin ? imax-1 : im1;
        ip1 = ip1 > imax ? imin : ip1;
        float4 pjm1 = pos[im1];     //p(i-1)
        float4 p = pos[i];          //p(i)
        float4 pjp1 = pos[ip1];     //p(i+1)
        float4 pn1 = posn1[i];      //last time step 
        //update the arrays with our newly computed values
        //barrier(CLK_GLOBAL_MEM_FENCE);
        float ymaxm = calc_wave(pjm1, p, pjp1, pn1, choice, k, ymin, ymax, dt, dx);
        pos[i].y = ymaxm;
        posn1[i] = p;
        posn1[i].w = 1.;
        //barrier(CLK_GLOBAL_MEM_FENCE);
        ymaxm = ymaxm < 0. ? -ymaxm : ymaxm;
        color[i].x = 1 - ymaxm;
        color[i].y = ymaxm;
    }
#endif

}

