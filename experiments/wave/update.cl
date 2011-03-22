__kernel void update(__global float4* pos, 
                     __global float4* color, 
                     __global float4* posn1, 
                     __global float4* posn2, 
                     int ntracers, 
//                     int choice,
                     int num,
//                     float k, 
//                     float ymin, 
//                     float ymax, 
                     float dt,
                     float dz
                     )
{
    unsigned int ind = get_global_id(0);
    int i;
    float life;

    //handle the ping pong for actual wave equation
    posn2[ind].xyz = posn1[ind].xyz;   //make sure we don't overwrite the life which is stored in posn2.w
    posn1[ind] = pos[ind];

    for(int j = 0; j < ntracers; j++)
    {

        i = ind + (j+1) * num;
#if 1
        life = posn2[i].w - dt/ntracers;
        if(life <= 0)
        {
            //start this tracer over
            pos[i] = posn1[ind]; 
            color[i] = color[ind];
            life = ntracers*dt;

        }
        pos[i].z += dz;
#endif
        pos[i].w = 1.;
        posn2[i].w = life;
    }

}
