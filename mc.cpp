/* 
This code runs Monte Carlo simulations for Lennard Jones particles of a single type, in the canonical (NVT) ensemble.

The simulation cell is assumed to be cubic.

R. K. Lindsey (2023)
*/

#include<iostream>
#include<iomanip>
#include<fstream>
#include<string>
#include<vector>
#include<cmath>

#include "MersenneTwiser.h"

using namespace std;

const double  nav     = 6.02*pow(10.0,23);        // Avagadro's number
const double  cm2m    = 1.0/100.0;                // x cm * cm2m = m
const double  A2m     = 1.0/pow(10.0,10);         // x Angstrom * A to m = m    
const double  kB      = 1.380649*pow(10,-23.0);   // Units: J⋅K^−1 

struct xyz
{
    double x;
    double y;
    double z;
};

double get_dist(const xyz & a1, const xyz & a2, const xyz & boxdim, xyz & rij_vec)
{
    /* Write code to calculate the distance vector and scalar distance between two atoms, using the minimum image convention.
    The distance vector is rij_vec - it is passed by reference so it can be modified directly.
    The convention is that the vector should point from atom 1 to atom 2
    
    The function should return the scalar distance.
    */
    rij_vec.x = a2.x - a1.x;
    rij_vec.y = a2.y - a1.y;
    rij_vec.z = a2.z - a1.z;
    rij_vec.x -= std::round(rij_vec.x / boxdim.x) * boxdim.x;
    rij_vec.y -= std::round(rij_vec.y / boxdim.y) * boxdim.y;
    rij_vec.z -= std::round(rij_vec.z / boxdim.z) * boxdim.z;
    
    // Calculate the scalar distance using the Euclidean norm
    double dist = std::sqrt(rij_vec.x * rij_vec.x + rij_vec.y * rij_vec.y + rij_vec.z * rij_vec.z);

    return dist;
    
}

double update_max_displacement(double fraction_accepted, double boxdim, double max_displacement)
{
    /* Write code to update the maximum displacement based on current acceptance crtieria. 
    The function should return the the new maximum displacement.
    */
    double new_max_displacement;

    if (fraction_accepted > 0.5) {
        new_max_displacement = std::min(max_displacement * 1.1, boxdim / 2.0);
    } else {
        new_max_displacement = std::max(max_displacement / 1.2, boxdim / 100.0);
    }
    return new_max_displacement;
}

void write_frame(ofstream & trajstream, vector<xyz> & coords, string atmtyp, xyz & boxdim, int mcstep)
{
    trajstream << "ITEM: TIMESTEP" << endl;
    trajstream << mcstep << endl;
    trajstream << "ITEM: NUMBER OF ATOMS" << endl;
    trajstream << coords.size() << endl;
    trajstream << "ITEM: BOX BOUNDS pp pp pp" << endl;
    trajstream << "0 " << boxdim.x << endl;
    trajstream << "0 " << boxdim.y << endl;
    trajstream << "0 " << boxdim.z << endl;
    trajstream << "ITEM: ATOMS id type element xu yu zu" << endl;
    
    for (int i=0; i<coords.size(); i++)
        trajstream << i << " 1 " << atmtyp << " " << coords[i].x << " "<< coords[i].y << " "  << coords[i].z << endl;
}

double get_LJ_eij(double sigma, double epsilon, double rcut, double rij)
{
    /* Write code to compute the Lennard Jones energy between the two atoms.
    It should account for the user-specified cutoff distance.
    The function should return the computed energy.
    */
    if (rij < rcut) {
        return 4 * epsilon * ((pow(sigma, 12)/ pow(rij, 12)) - (pow(sigma, 6)/ pow(rij, 6)));
    } else {
        return 0.0;  
    }
}

void get_LJ_fij(double sigma, double epsilon, double rcut, double rij, const xyz & rij_vec, xyz & fij)
{
    /* Write code to compute the Lennard Jones force between the two atoms.
    It should account for the user-specified cutoff distance.
    The function should update the the force and distance vectors directly
    The function should not return anything.
    */
    if (rij < rcut) {
        double f = 24 * epsilon * (2 * (pow(sigma, 12)/ pow(rij, 13)) - (pow(sigma, 6)/ pow(rij, 7))) ;
        fij.x = f * rij_vec.x / rij;
        fij.y = f * rij_vec.y / rij;
        fij.z = f * rij_vec.z / rij;
    } else {
        fij.x = 0.0;
        fij.y = 0.0;
        fij.z = 0.0;
    }
}

void get_single_particle_LJ_contributions(double sigma, double epsilon, double rcut, const vector<xyz> & coords, int selected_atom, const xyz & selected_atom_coords, const xyz & boxdim, double & energy_selected, xyz & stress_selected)
{
    energy_selected   = 0;
    stress_selected.x = 0;
    stress_selected.y = 0;
    stress_selected.z = 0;
    xyz force;
    force.x = 0;
    force.y = 0;
    force.z = 0;
    
    static double rij;
    static xyz    rij_vec;
    
    /* Write code to determine the contributions to the total system energy, forces, and stresses due to the selected atom.
    Self interactions should not be included.

    // Loop over all atoms. Within the loop:
        
        // Get the scalar distance and distance vector between atoms, using MIC


        // Determine pair energy, but only if interaction is within cutoff idstance
    
    
        // Determine the atom pair's contribution to the total system pressure - again, only perform 
        // if within the model cutoff - we'll use this if the move is accepted
    
    */
    for (int i = 0; i < coords.size(); ++i) {
        if (i == selected_atom)
            continue;

        rij = get_dist(selected_atom_coords, coords[i], boxdim, rij_vec);
        if (rij < rcut) {
            energy_selected += get_LJ_eij(sigma, epsilon, rcut, rij);
            get_LJ_fij(sigma, epsilon, rcut, rij, rij_vec, force);
            stress_selected.x += rij_vec.x * force.x;
            stress_selected.y += rij_vec.y * force.y;
            stress_selected.z += rij_vec.z * force.z;
        }
    }
}


int main(int argc, char* argv[])
{
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Set user-defined variables (Read in from input file at some point)
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    int     seed    = stoi(argv[1]);// Seed for random number generator - read from commandline 
    double  redden  = stod(argv[2]);// Reduced density - read from commandline
    
    //////// Static variables 
    
    MTRand  mtrand(seed); // Seed the (psuedo) random number generator
    
    double  sigma   = 3.4;          // Units: Angstrom
    double  epsilon = 120;          // Units: K/k_B
    double  rcut    = 4*sigma;      // Model outer cutoff
    
    int     natoms  = 500;          // Number of atoms in the system
    string  atmtyp  = "Ar";         // Atom type (atomic symbol)
    double  molmass = 39.948;       // Mass of an particle of atmtyp (Atomic mass in the case of an argon atom)
    double  density = redden / pow(A2m,3.0) * pow(cm2m,3.0) / nav * molmass / pow(sigma,3.0);     // System density; Units: g/cm^3
    double  numden;                 // System number density; Units: atoms/Ang^3 - will be calculated later on
    
    double  temp    = 1.2*epsilon;  // Temperature, in K
    double  nsteps  = 5e6;          // Number of MC steps
    int     iofrq   = 2e3;          // Frequency to output statistics and trajectory
    int     nequil  = 2e6;          // Equilibration period (chemical potential and heat capacity only collected after this many steps)
    
    xyz         boxdim;             // Simulation box x, y, and z lengths - will be determined later on
    vector<xyz> coords;             // Coordinates for all atoms in our system; coords[0].x gives the x coordinate of the 0th atom - will be generated later on
    
    //////// Variables used for file i/o
    
    ofstream    trajstream; // Trajectory file - uses the LAMMPS lammpstrj format
        
    //////// Print information for user
    
    cout << "# Number of atoms:       " << natoms     << endl;
    cout << "# Atom type:             " << atmtyp     << endl;
    cout << "# Molar Mass (g/mol):    " << molmass    << endl;
    cout << "# Density (g/cm^3):      " << density    << endl;
    cout << "# LJ sigma (Angstrom):   " << sigma      << endl;
    cout << "# LJ epsilon/kB (K):     " << epsilon    << endl;
    cout << "# LJ cutoff (Angstrom):  " << rcut       << endl;
    cout << "# LJ cutoff (sigma):     " << rcut/sigma << endl;
    cout << "# Temperature (K):       " << temp       << endl;
    cout << "# Number MC steps:       " << nsteps     << endl;
    cout << "# Output frequency:      " << iofrq      << endl;
    

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Generate the system coordinates
    // Generates an initial configuation of atom on a 3D lattice  with a user-specified number of 
    // atoms, at the user-specified density. 
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    // Determine the box length that will yield the user-specified density. Start by computing the target number density.

    numden = density / molmass * nav / pow(cm2m,3.0) * pow(A2m,3.0); // Density in atoms per Ang^3
    
    cout << "# Num. den. (atoms/Ang): " << numden << endl;
    
    boxdim.x = pow(natoms/numden, 1.0/3.0); 
    boxdim.y = boxdim.x;
    boxdim.z = boxdim.x;
    
    cout << "# Box length (x):        " << boxdim.x << endl;
    cout << "# Box length (y):        " << boxdim.y << endl;
    cout << "# Box length (z):        " << boxdim.z << endl;

    // Compute the number of gridpoints to use in each direction (ceiling of the cube root of number of atoms).
    // Set the spacing in each dimension based on the box length and the number of gridpoints+1 (to prevent overlap
    // across the periodic boundary. 
    
    int     ngridpoints = ceil(pow(natoms,1.0/3.0));
    double  spacing     = boxdim.x / (ngridpoints+1);
    
    cout << "# Init. spacing (Ang):   " << spacing << endl;

    xyz     tmp_atom;
    int     added_atoms = 0;
    
    for (int x=0; x<ngridpoints; x++)
    {
        for(int y=0; y<ngridpoints; y++)
        {
            for(int z=0; z<ngridpoints; z++)  
            {
                if(added_atoms >= natoms)
                    continue;
            
                tmp_atom.x = x * spacing;
                tmp_atom.y = y * spacing;
                tmp_atom.z = z * spacing;

                coords.push_back(tmp_atom);
            
                added_atoms++;
            }
        }
    }
            
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Print information on the initial configuration 
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    // Print this initial configuration to a file
    
    trajstream.open("MC_traj.lammpstrj");
    write_frame(trajstream, coords, atmtyp, boxdim, -1);

    // Print some information for the user

    /* Complete these lines to compute the reduced density and temperature*/
    
    cout << "# reduc. density (rho*): " << redden << endl;
    cout << "# reduc. temp (T*):      " << temp/epsilon << endl;

    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Determine initial system energy and pressure
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    double  rij;                // Scalar distance between atoms
    xyz     rij_vec;            // Distance vector between atoms
    
    double  energy = 0;         // Energy for the configuration
   
    double widom_factor = 0;    // The exponential term in the Widom chemical potential expression
    double widom_trials = 0;    // Number of Widom insertion attempts
    double widom_avg    = 0;    // Average value of the wWidom factor
        
    double Eavg = 0;            // Average energy
    double Esqavg = 0;          // Square of average energy

   
    xyz     force;              // Force on a given atom
    xyz     stensor;            // System stress tensor (diagonal components only, i.e., xx, yy, zz)
    stensor.x = 0;
    stensor.y = 0;
    stensor.z = 0;
    
    for (int i=0; i<natoms; i++)                                                                         
    {
        for (int j=i+1; j<natoms; j++)
        {
            // Get the scalar distance and distance vector between atoms, using MIC

            rij = get_dist(coords[i], coords[j], boxdim, rij_vec);
                      
            // Determine atom pair's contirbution to total system energy - remember to only perform the 
            // calculation if the pair distance is within the model cutoff
            
            if (rij < rcut)
            {
                energy += get_LJ_eij(sigma, epsilon, rcut, rij);
            }
            
            // Determine the atom pair's contribution to the total system pressure - again, only perform 
            // if within the model cutoff
                        
            if (rij < rcut)
            {
                get_LJ_fij(sigma, epsilon, rcut, rij, rij_vec, force);
                stensor.x += rij_vec.x * force.x;
                stensor.y += rij_vec.y * force.y;
                stensor.z += rij_vec.z * force.z;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Begin the simulation
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    
    double max_displacement = 0.5*boxdim.x;  // Start trial displacements at one angstrom

    int     selected_atom;
    
    double  eold_selected;  // Pre-trial-move energy of the selected atom
    double  enew_selected;  // Post-trial-move energy of the selected atom
    double  delta_energy;   // Difference between old and new (trial) energy
    xyz     sold_selected;  // Pre-trial-move stress tensor diagonal of the selected atom
    xyz     snew_selected;  // Post-trial-move stress tensor diagonal of the selected atom
    
    xyz     trial_displacement;    
    xyz     trial_position;
    
    int     naccepted_moves = 0;    // Number of accepted moves
    double  fraction_accepted;      // Fraction of attempted moves that have been accepted
    int     nrunningav_moves = 0;   // Move index for running averages (doesn't start until equilibration period has ended)
    
    double pressure;
    double Cv;

    double stat_avgE   = 0;
    double stat_avgEsq = 0;
    double stat_avgP   = 0;

    for (int i=0; i<nsteps; i++)
    {
        // Select a random particle. The syntax below shows how to use the random number generator. This generate a random integer between 0 and natoms-1
        
        selected_atom = int(mtrand()*natoms);
        
        // Determine contributions to the system's energy, force, and stress due to the selected atom 
        
        get_single_particle_LJ_contributions(sigma, epsilon, rcut, coords, selected_atom, coords[selected_atom], boxdim, eold_selected, sold_selected);
        

        // Attempt to randomly move a particle - this is a multistep process
        
        // 1. Generate the trial **displacement** in x, y, and z - the particle should be able to move in positive
        // and negative directions, i.e., +/- the maximum displacement

        /*write this*/
        trial_displacement.x = (2.0 * mtrand() - 1.0) * max_displacement;
        trial_displacement.y = (2.0 * mtrand() - 1.0) * max_displacement;
        trial_displacement.z = (2.0 * mtrand() - 1.0) * max_displacement;
        
        // 2. Generate the trial **position** in x, y, and z based on the displacement
        
        trial_position.x = coords[selected_atom].x + trial_displacement.x;
        trial_position.y = coords[selected_atom].y + trial_displacement.y;
        trial_position.z = coords[selected_atom].z + trial_displacement.z;
        
        // 3. Apply PBC if the particle has moved outside the box
        
        if (trial_position.x < 0) trial_position.x += boxdim.x;
        if (trial_position.x >= boxdim.x) trial_position.x -= boxdim.x;

        if (trial_position.y < 0) trial_position.y += boxdim.y;
        if (trial_position.y >= boxdim.y) trial_position.y -= boxdim.y;

        if (trial_position.z < 0) trial_position.z += boxdim.z;
        if (trial_position.z >= boxdim.z) trial_position.z -= boxdim.z;        
        
        // 4. Determine the energy contribution of that particle with the system **in it's trial position**
    
        get_single_particle_LJ_contributions(sigma, epsilon, rcut, coords, selected_atom, trial_position, boxdim, enew_selected, snew_selected);
        
        if (i >= nequil) // Only do Widom tests for the equilibrated portion of the simulation
        {
            // 5. Do a widom insertion test
        
            double ewidom;
            xyz    swidom;
            xyz    widom_position;
        
            // 5.a Generate another position for a new ghost atom
        
            widom_position.x = mtrand() * boxdim.x;
            widom_position.y = mtrand() * boxdim.y;
            widom_position.z = mtrand() * boxdim.z;
        

            // 5.b Calculate change in system energy due to insertion of new ghost atom 
        
            get_single_particle_LJ_contributions(sigma, epsilon, rcut, coords, -1, widom_position, boxdim, ewidom, swidom);
        
            // 5.c Update the Widom factor
        
            widom_factor = exp(-ewidom/ (temp));/*write this*/
            widom_avg   += widom_factor;
            widom_trials++;
        }
        else
        {
            // Needed to avoid a divide-by-zero during equilibration phase when these values aren't collected
            widom_avg    = 1;
            widom_trials = 1;
        }
        
        // 6. Accept or reject the move
        // If E_old is the energy of the original system and E_new is the system energy when the 
        // particle is at it's trial position, E_old - eold_selected + enew_selected = E_new
        // Therefore delta E, which = E_new - E_old is just enew_selected - eold_selected
        
        delta_energy = enew_selected - eold_selected;

        if ( (mtrand() < exp(-delta_energy/ (temp)) )|| delta_energy < 0) // Then accept
        {
            // Then the system energy has decreased **or** our random number is less than our probability to accept
            // Update the system position, energy, and stress tensor, and number of accepted moves
            
            coords[selected_atom] = trial_position;
            energy = energy - eold_selected + enew_selected;
            stensor.x = stensor.x - sold_selected.x + snew_selected.x;
            stensor.y = stensor.y - sold_selected.y + snew_selected.y;
            stensor.z = stensor.z - sold_selected.z + snew_selected.z;
            naccepted_moves++;
             
        }
        
        // Update maximum diplacement to target a 50% acceptance rate
        
        fraction_accepted = float(naccepted_moves)/float(i+1);
        
        max_displacement = update_max_displacement(fraction_accepted, boxdim.x, max_displacement);
       
       
        // print statistics if ineeded - don't forget to convert the stress tensor to pressure 
        // Compute instantaneous properties
        
        pressure = (stensor.x + stensor.y + stensor.z)/(3.0 * pow(boxdim.x,3)) + (natoms* temp) / pow(boxdim.x,3);/*write this - this is the full pressure, i.e., kinetic + Virial*/;
        Cv       = 0;

        if (i >= nequil) // Compute values for running averages, only using the equilibrated portion of the trajectory
        {
            stat_avgE   += energy;
            stat_avgEsq += energy*energy;        
            stat_avgP   += pressure/epsilon*pow(sigma,3.0);
            nrunningav_moves++;
            
            double avgE   = stat_avgE /float(nrunningav_moves);
            double avgEsq = stat_avgEsq / float(nrunningav_moves);
            
            Cv =(avgEsq - avgE * avgE)/( temp * temp)/*write this - this should only be the dE/dT portion*/;
        }
 
        
        if ( (i+1) % iofrq == 0)
        {
            write_frame(trajstream, coords, atmtyp, boxdim, i);
            
            cout << "Step:  " << setw(10) << left << i;
            cout << " NAcc:  " << setw(10) << left << setprecision(3) <<  naccepted_moves;
            cout << " fAcc:  " << setw(10) << left << fixed << setprecision(3) << fraction_accepted;
            cout << " Maxd:  " << setw(10) << left << fixed << setprecision(5) << max_displacement;
            cout << " E*/N:  " << setw(10) << left << fixed << setprecision(5) << energy/natoms/epsilon;
            cout << " P*:     " << setw(10) << left << fixed << setprecision(5) << pressure/epsilon*pow(sigma,3.0) ;
            cout << " P*cold: " << setw(10) << left << fixed << setprecision(5) << (stensor.x + stensor.y + stensor.z)/(3.0 * pow(boxdim.x,3.0)) /epsilon*pow(sigma,3.0); /*write this - this is the reduced Virial component of pressure*/;
            cout << " Mu*_xs: " << setw(10) << left << fixed << setprecision(5) << - temp * log(widom_avg / widom_trials);/*write this - this is excess chemical potential*/; 
            cout << " Cv*/N_xs:  " << setw(15) << left << fixed << setprecision(5) << Cv/natoms;/*write this - this is excess heat capacity per atom*/
            cout << " E(kJ/mol): " << setw(10) << left << fixed << setprecision(3) << energy * 0.008314; // KJ/mol per K 
            cout << " P(bar):    " << setw(10) << left << fixed << setprecision(3) << pressure * 0.008314 * 10.0e30 * 1000/(6.02*10.0e23)*1.0e-5; // KJ/mol/A^3 to bar
            cout << endl;
 
        }
    }
    
    trajstream.close();
    
    stat_avgE    /= float(nrunningav_moves);
    stat_avgEsq  /= float(nrunningav_moves);
    Cv            =  (stat_avgEsq - pow(stat_avgE,2))/(temp * temp); /*write this, based on average energies - this should only be the dE/dT portion*/
    stat_avgE    *= 1.0/natoms/epsilon;

    cout << "# Computed average properties: " << endl;
    cout << " # E*/N:  "      << setw(10) << left << fixed << setprecision(5) << stat_avgE << endl;
    cout << " # P*:     "     << setw(10) << left << fixed << setprecision(5) << stat_avgP / float(nrunningav_moves) << endl;
    cout << " # Cv*/N_xs:   " << setw(15) << left << fixed << setprecision(5) << Cv/natoms << endl;
    cout << " # Mu_xs:  "     << setw(10) << left << fixed << setprecision(5) << -temp * log(widom_avg / widom_trials)<< endl;       
    
}


















