! ee -> Z/gamma* -> bbbar at sqrt(s) = 91.188 GeV, with the Pythia8 per-event
! variation-weight machinery switched on. Exercises the propagation of the
! generator weight vector into EventHeader.weights and of the weight names into
! the file-level metadata frame (edm4hep::labels::EventWeightsNames).
!
! Without UncertaintyBands:doVariations / VariationFrag:list, Pythia books only
! the nominal weight and the converter writes neither the weights vector nor the
! metadata frame. With the settings below it books 53 weights per event.
!
! Realistic FCC-ee setup: the stable-particle convention is the tracking-cylinder
! one, and the variation amplitudes are calibrated so that each is a 1 sigma
! variation of its measured PDG/HFLAV anchor.
Random:setSeed = on
Random:seed = 10
Main:timesAllowErrors = 5

Init:showChangedSettings = on
Init:showChangedParticleData = off
Main:numberOfEvents = 100
Next:numberCount = 100
Next:numberShowInfo = 1
Next:numberShowProcess = 1
Next:numberShowEvent = 0

Beams:idA = 11
Beams:idB = -11
Beams:eCM = 91.188

WeakSingleBoson:ffbar2gmZ = on
23:onMode = off
23:onIfAny = 5

PartonLevel:ISR = on
PartonLevel:FSR = on

! stable-particle convention: decay only inside the tracking cylinder
ParticleDecays:limitCylinder = on
ParticleDecays:xyMax = 2250.
ParticleDecays:zMax = 2500.

! =====================================================================
! Per-event systematic-variation weights. Each amplitude is calibrated so that
! the variation shifts its measured anchor observable by exactly 1 sigma of the
! PDG/HFLAV world average:
!   frag:rFactB           0.855 +- 0.0326   anchor <x_B> 0.35%        [PDG]
!   frag:rFactC            1.32 +- 0.151    anchor <x_E(D*)> 1.2%     [PDG]
!   frag:aLund             0.68 +- 0.0895   anchor <x_B> 0.35%        [PDG]
!   frag:bLund             0.98 +- 0.0584   anchor <x_B> 0.35%        [PDG]
!   frag:ptSigma          0.335 +- 0.02     anchor none               [CONVENTIONAL: <x_B> does not respond (0.01 sigma)]
!   frag:rho              0.217 +- 0.00923  anchor <n(K0)> 1.3%       [PDG (tightest; f(Bs) 8% would allow 0.0209)]
!   frag:xi               0.081 +- 0.00151  anchor <n(Lambda)> 1.7%   [PDG (tightest; f(b bar.) 13% would allow 0.0081)]
!   frag:x                0.915 +- 0.085    anchor none               [CONVENTIONAL: <n(K0)> does not respond (0.03 sigma)]
!   frag:y               0.0275 +- 0.0035   anchor <n(Lambda)> 1.7%   [PDG]
!   fsr:G2QQ:muRfac         1.0 +- 0.3086   anchor g->cc 13%          [PDG (1 sigma in g->cc; only 0.43 sigma in g->bb)]
!   fsr:G2QQ:cNS            0.0 +- 0.9814   anchor g->bb 20%          [PDG (1 sigma in g->bb; only 0.24 sigma in g->cc)]
!
! The shower band (fsr:muRfac, fsr:cNS) is the conventional 2-point variation:
! alphaS(MZ) constrains the physical coupling, not the shower's effective one.
! =====================================================================
UncertaintyBands:doVariations = on
UncertaintyBands:List = { fsrRhi fsr:muRfac=2.0, fsrRlo fsr:muRfac=0.5, fsrNShi fsr:cNS=2.0, fsrNSlo fsr:cNS=-2.0, g2qqRhi fsr:G2QQ:muRfac=1.3086, g2qqRlo fsr:G2QQ:muRfac=0.6914, g2qqNShi fsr:G2QQ:cNS=0.9814, g2qqNSlo fsr:G2QQ:cNS=-0.9814 }

VariationFrag:list = { rFactBHi frag:rFactB=0.8876, rFactBLo frag:rFactB=0.8224, rFactCHi frag:rFactC=1.471, rFactCLo frag:rFactC=1.169, aLundHi frag:aLund=0.7695, aLundLo frag:aLund=0.5905, bLundHi frag:bLund=1.0384, bLundLo frag:bLund=0.9216, ptSigmaHi frag:ptSigma=0.355, ptSigmaLo frag:ptSigma=0.315, rhoHi frag:rho=0.22623, rhoLo frag:rho=0.20777, xiHi frag:xi=0.08251, xiLo frag:xi=0.07949, xHi frag:x=1.0, xLo frag:x=0.83, yHi frag:y=0.031, yLo frag:y=0.024 }
