! 1) Settings that will be used in a main program.
Main:numberOfEvents = 3            ! number of events to generate
Main:timesAllowErrors = 100        ! abort run after this many flawed events
#Random:seed = 1234                ! initialize random generator with a seed

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
Beams:LHEF = ../../examples/data/lhe_tester_ggHH.lhe ! the LHEF to read from

! 4) Exclusive decay with ResonanceDecayUserHook
25:onMode = off
25:onIfMatch = 5 -5
25:onIfMatch = 22 22
ResonanceDecayFilter:filter = on
ResonanceDecayFilter:exclusive = on
ResonanceDecayFilter:mothers = 25
ResonanceDecayFilter:daughters = 5,5,22,22

! 5) POWHEG parameters
! Number of outgoing particles of POWHEG Born level process
! (i.e. not counting additional POWHEG radiation)
POWHEG:nFinal = 2

! How vetoing is performed:
!  0 - No vetoing is performed (userhooks are not loaded)
!  1 - Showers are started at the kinematical limit.
!      Emissions are vetoed if pTemt > pThard.
!      See also POWHEG:vetoCount below
POWHEG:veto = 1

! After 'vetoCount' accepted emissions in a row, no more emissions
! are checked. 'vetoCount = 0' means all emissions are checked.
POWHEG:vetoCount = 3

! Selection of pThard (note, for events where there is no
! radiation, pThard is always set to be SCALUP):
!  0 - pThard = SCALUP (of the LHA/LHEF standard)
!  1 - the pT of the POWHEG emission is tested against all other
!      incoming and outgoing partons, with the minimal value chosen
!  2 - the pT of all final-state partons is tested against all other
!      incoming and outgoing partons, with the minimal value chosen
POWHEG:pThard = 0

! Selection of pTemt:
!  0 - pTemt is pT of the emitted parton w.r.t. radiating parton
!  1 - pT of the emission is checked against all incoming and outgoing
!      partons. pTemt is set to the minimum of these values
!  2 - the pT of all final-state partons is tested against all other
!      incoming and outgoing partons, with the minimal value chosen
! WARNING: the choice here can give significant variations in the final
! distributions, notably in the tail to large pT values.
POWHEG:pTemt = 0

! Selection of emitted parton for FSR
!  0 - Pythia definition of emitted
!  1 - Pythia definition of radiator
!  2 - Random selection of emitted or radiator
!  3 - Both are emitted and radiator are tried
POWHEG:emitted = 0

! pT definitions
!  0 - POWHEG ISR pT definition is used for both ISR and FSR
!  1 - POWHEG ISR pT and FSR d_ij definitions
!  2 - Pythia definitions
POWHEG:pTdef = 1

! MPI vetoing
!  0 - No MPI vetoing is done
!  1 - When there is no radiation, MPIs with a scale above pT_1 are vetoed,
!      else MPIs with a scale above (pT_1 + pT_2 + pT_3) / 2 are vetoed
POWHEG:MPIveto = 0
