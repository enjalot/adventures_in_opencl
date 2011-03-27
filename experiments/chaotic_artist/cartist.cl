typedef struct Params
{
    //self.params = struct.pack('ffffff', self.dt, self.maxlife, self.dlife, beta, A, B, numpy.pi)
    float dt;
    float maxlife;
    float dlife;
    float beta;
    float A;
    float B;
    float PI;

} Params;


float calc_alpha(float life, float oomph,  __constant struct Params* params)
{
    float oml = 1.f/(params->maxlife - life);
    //float oml = 1.f/(life);
    float alpha = oomph*oml + params->A * sin(8.0f*params->PI*life - params->beta) + params->B;
    return alpha;
}

float4 calc_color(float life)
{
    float4 color;
    color.x = life;
    color.y = 0.f;
    color.z = 0.f;
    return color;
}

__kernel void cartist(__global float4* pos, 
                   __global float4* col, 
                   __global float4* time, 
                   __global int4* props,
                   //int ntracers, 
                   int count,
                   float newt,
                   float4 newp,
                   __constant struct Params* params
                   )
{
    //get our index in the array
    uint i = get_global_id(0);
    uint tid = get_local_id(0);

    float t = time[i].x; //this particles current time
    float dt = params->dt;
    float life = time[i].z;
    int lifeordeath;
    
    if(i == count)
    {
        //initialize new particle
        t = newt; //shes a witch, burn her!
        pos[i] = newp;
        life = params->maxlife;
    }

    t += dt;

    life -= params->dlife;
    
    time[i].x = t;
    time[i].y = dt;
    lifeordeath = life < 1E-6 ? 0.f : 1.f; //should this go in tid==0 loop?
    time[i].z = life * lifeordeath; //a dead particle will have 0 life

    float4 color = calc_color(life);
    //oomph based on what kind of note?
    float alpha = calc_alpha(life, .01, params);
    color.w = alpha * lifeordeath;
    //color.x = 1.f;
    //olor.w = 1.f;

    col[i] = color;


}

