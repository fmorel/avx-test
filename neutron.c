#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DISTRIB_BINS 32
#define DISTRIB_BASE 0.008 /* Energy value of the first bin in eV */

/* Each bin of the Distribution struct represents the quantity of neutron at the energy level (DISTRIB_BASE * 2^bin)
 * going from DISTRIB_BASE to DISTRIB_BASE * 2^(DISTRIB_BINS-1) i.e from 8 meV to 16 MeV */
typedef struct {
    float qty[DISTRIB_BINS];
} Distribution;

typedef struct {
    float collision_prob;            /* Probability of collision */
    float capture_prob;             /* Probability of capture */
    float collision_energy_ratio;   /* Ratio of initial energy kept by neutron upon collision */
} AtomicProperties;

typedef struct {
    AtomicProperties prop;
    Distribution *layers_in;
    Distribution *layers_out;
    int n_layers;
} State;

/* Scale a distribution by a neutron quantity factor and an energy factor, both always smaller than 1 */
static void scale(Distribution *out, const Distribution *in, float qty_factor, float energy_factor)
{
    int i, bin_diff;
    float energy_log, energy_floor, qty_factor1, qty_factor2, t;
    
    /* Compute log2 of the energy factor and perform linear interpolation of qty factor across the two adjacent bins */
    energy_log = - log2(energy_factor);
    energy_floor = floor(energy_log);
    t = energy_log - energy_floor;
    qty_factor1 = qty_factor * (1.0f - t);
    qty_factor2 = qty_factor * t;
    bin_diff = (int) energy_floor;    /* Always >= 0 */
    
    for (i = 0; i < DISTRIB_BINS - bin_diff - 1; i++) {
        out->qty[i] = qty_factor1 * in->qty[i + bin_diff] + qty_factor2 * in->qty[i + bin_diff + 1];
    }
    out->qty[DISTRIB_BINS - bin_diff -1] = qty_factor1 * in->qty[DISTRIB_BINS - 1];
    for (i = DISTRIB_BINS - bin_diff; i < DISTRIB_BINS ; i++) {
        out->qty[i] = 0;
    }
}

/* Add two distributions */
static void add(Distribution *out, const Distribution *a, const Distribution *b)
{
    int i;
    for (i = 0; i < DISTRIB_BINS; i++) {
        out->qty[i] = a->qty[i] + b->qty[i];
    }
}

static float mean(const Distribution *in)
{
    int i;
    float mean = 0.0f; 
    float tot_qty = 0.0f;
    for (i = 0; i < DISTRIB_BINS; i++) {
        mean += in->qty[i] * DISTRIB_BASE * (1U << i);
        tot_qty += in->qty[i];
    }
    return mean / tot_qty;
}

static void print_dist(const Distribution *in)
{
    int i;
    for (i = 0; i < DISTRIB_BINS; i++) {
        printf(" %.2g", in->qty[i]);
    }
    printf("\n");
}

static void print_state(const State *s)
{
    int i;
    for (i = 0; i < s->n_layers; i++) {
        printf("Layer %d IN :\n", i);
        print_dist(&s->layers_in[i]);
        printf("Layer %d OUT :\n", i);
        print_dist(&s->layers_out[i]);
    }
}


void atom_encounter(const AtomicProperties *prop, const Distribution *in, Distribution *reflected, Distribution *through)
{
    /* Assume that neutron velocity is uniform after collision : half of them are reflected, half of them go through */
    scale(reflected, in, 0.5f * prop->collision_prob, prop->collision_energy_ratio);
   
    scale(through, in, (1.0f - prop->capture_prob - prop->collision_prob), 1.0f);
    add(through, through, reflected);
}

/* Returns mean of layer out */
float loop(State *s)
{
    int i;
    Distribution in_plus_one, out;
    for (i = 0; i < s->n_layers - 1; i++) {
        atom_encounter(&s->prop, &s->layers_in[i], &out, &in_plus_one);
        atom_encounter(&s->prop, &s->layers_out[i+1], &s->layers_in[i+1], &s->layers_out[i]);
        add(&s->layers_in[i+1], &s->layers_in[i+1], &in_plus_one);
        add(&s->layers_out[i], &s->layers_out[i], &out);
    }
    //print_state(s);
    return mean(&s->layers_out[0]);
}

#define EPSILON 1e-15f
#define N_ITER_MAX 100
int main(int argc, char **argv)
{
    State s;
    float out_energy = 0.0f, out_energy_init = 0.0f;
    int n_iter = 0;

    s.n_layers = atoi(argv[1]);
    s.prop.collision_prob = 1e-4f;
    s.prop.capture_prob = 1e-6f;
    s.prop.collision_energy_ratio = 0.9;
    s.layers_in = calloc(s.n_layers, sizeof(Distribution));
    s.layers_out = calloc(s.n_layers, sizeof(Distribution));

    /* Initial conditions */
    s.layers_in[0].qty[DISTRIB_BINS-2] = 1.0; /* 8 Mev */
    printf("Input energy %.3g eV\n", mean(&s.layers_in[0]));
    /* Loop */
    do {
        out_energy_init = out_energy;
        out_energy = loop(&s);
        n_iter++;
    } while (n_iter < N_ITER_MAX && fabs(out_energy - out_energy_init) > EPSILON);

    printf("Convergence after %d steps\n", n_iter);
    printf("Mean reflected energy is %.3g eV\n", out_energy);
    printf("Distribution : \n");
    print_dist(&s.layers_out[0]);

    free(s.layers_in);
    free(s.layers_out);
    return 0;
}
