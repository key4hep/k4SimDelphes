#
#######################################
# Order of execution of various modules
#######################################

set RandomSeed 31415
set MaxEvents 1000

set ExecutionPath {
  ParticlePropagator

  TreeWriter
}


#################################
# Propagate particles in cylinder
#################################

module ParticlePropagator ParticlePropagator {
  set InputArray Delphes/stableParticles

  set OutputArray stableParticles
  set ChargedHadronOutputArray chargedHadrons
  set ElectronOutputArray electrons
  set MuonOutputArray muons

  # inner radius of the solenoid, in m
  set Radius 2.25

  # half-length: z of the solenoid, in m
  set HalfLength 2.5

  # magnetic field, in T
  set Bz 2.0
}

##################
# ROOT tree writer
##################

# Tracks, towers and eflow objects are not stored by default in the output.
# If needed (for jet constituent or other studies), uncomment the relevant
# "add Branch ..." lines.

module TreeWriter TreeWriter {
    # add Branch InputArray BranchName BranchClass

    add Branch Delphes/allParticles Particle GenParticle
    add Branch ParticlePropagator/electrons Electron Electron
}

