! 1) Settings that will be used in a main program.
Main:numberOfEvents = 10            ! number of events to generate
Main:timesAllowErrors = 100        ! abort run after this many flawed events
Random:seed = 88187982                ! initialize random generator with a seed

! 2) Settings related to output in init(), next() and stat() functions.
Init:showChangedSettings = on      ! list changed settings
Init:showAllSettings = off         ! list all settings
Init:showChangedParticleData = on  ! list changed particle data
Init:showAllParticleData = off     ! list all particle data
Next:numberCount = 10              ! print message every n events
Next:numberShowLHA = 1             ! print LHA information n times
Next:numberShowInfo = 1            ! print event information n times
Next:numberShowProcess = 1         ! print process record n times
Next:numberShowEvent = 1           ! print event record n times
Stat:showPartonLevel = off         ! additional statistics on MPI

! 3) Tell Pythia that LHEF input is used
Beams:frameType             = 4
Beams:setProductionScalesFromLHEF = off
! This assumes that the build directory is one below the
! top level directory of the repository
Beams:LHEF = ../../examples/data/events.lhe

! 4) Specify jet matching parameters for MLM
! Note: Some of these settings might be read directly from the input LHE file.
JetMatching:merge            = on
JetMatching:scheme           = 1
JetMatching:setMad           = off
JetMatching:coneRadius       = 1.0
JetMatching:etaJetMax        = 10.0
JetMatching:nJetMax          = 2
JetMatching:qCut             = 90.0
